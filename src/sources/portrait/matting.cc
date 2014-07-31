//portrait/matting.cc

#include "portrait/matting.hh"

#include <functional>
#include <map>
#include <unordered_map>

#include "sybie/common/Graphics/Structs.hh"
#include "sybie/common/Graphics/CVCast.hh"
#include "sybie/common/RichAssert.hh"
#include "sybie/common/Time.hh"

#include "portrait/math.hh"
#include "portrait/graphics.hh"

namespace portrait {

//本模块内所有图形运算不使用OpenCV，而使用这个库，以简化编程
using namespace sybie::common::Graphics;

namespace {

//前景色采样中心与边缘距离
enum { FrontSamplingDistance = 5 };
//背景色采样中心与边缘距离
enum { BackSamplingDistance = 30 };
//前景色采样半径
enum { FrontSamplingRange = 5 };
//背景色采样半径
enum { BackSamplingRange = 3 };
//混合范围，边缘向内（前景方向）的距离
enum { FrontSamplingStep = 2 };
//混合范围，边缘向外（背景方向）的距离
enum { BackSamplingStep = 5 };
//混合范围，边缘向内（前景方向）的距离
enum { FrontMattingRange = 10 };
//混合范围，边缘向外（背景方向）的距离
enum { BackMattingRange = 30 };
//前景色分类数，越大越准确、越慢。
enum {KFront = 4};
//球体映射的球体半径，足够大即可
enum {SphereRadius = 0xfff};
//Matting前景色和背景色最小距离（欧氏距离），太小易被噪声干扰，太大则精确度下降
enum {MinFrontBackDiff = 5};
//前景分类，每个分类的最小距离
//enum {MinSphereDiff = 5 * SphereRadius / 255 / 4};

const Size FrontSamplingSize(FrontSamplingRange * 2 + 1,
                             FrontSamplingRange * 2 + 1);
const Size BackSamplingSize(BackSamplingRange * 2 + 1,
                            BackSamplingRange * 2 + 1);
const Size FrontSamplingStepArea(FrontSamplingStep * 2 - 1,
                                 FrontSamplingStep * 2 - 1);
const Size BackSamplingStepArea(BackSamplingStep * 2 - 1,
                                BackSamplingStep * 2 - 1);

template<class T, int n>
cv::Vec<T,n> Normalize(const cv::Vec<T,n>& vec, T modulus)
{
    return vec * modulus / std::max(1, (T)ModulusOf(vec));
}

//球面上的向量
template<int radius>
class MeanOnSphere
{
public:
    MeanOnSphere()
        : _mean()
    { }

    void Push(const cv::Vec3i& val)
    {
        _mean.Push(val);
    }

    cv::Vec3i Get() const
    {
        return Normalize(_mean.Get(), radius);
    }

    int Count() const
    {
        return _mean.Count();
    }
private:
    Mean<cv::Vec3i> _mean;
}; //template<class T> class Mean


template<class T, int n, int reduce>
struct MattingDistance
{
    double operator()(const cv::Vec<T,n>& vec1, const cv::Vec<T,n>& vec2)
    {
        return std::max<double>(ModulusOf<T,n>(vec1 - vec2) - reduce, 1.0);
    }
};

/* 在GrabCut结果的mask中获取边缘像素点集。
 * 边缘像素点是前景点，且上下左右四个像素至少有一个是背景点。
 */
std::vector<Point> _GetBorderPoints(
    const MatBase<uint8_t>& mask)
{
    std::vector<Point> border_points;
    Rect area(Point(1,1), mask.GetSize() - Size(2,2));
    for (auto& point : PointsIn(area))
    {
        if (IsFront(mask[point]) &&
            (IsBack(mask[point + Point(-1,0)]) ||
             IsBack(mask[point + Point(1,0)]) ||
             IsBack(mask[point + Point(0,-1)]) ||
             IsBack(mask[point + Point(0,1)])
            ))
            border_points.push_back(point);
    }
    return border_points;
}

struct ExpandingPoint
{
    ExpandingPoint(const Point& point,
                   const Point& source)
        : point(point), source(source), distance()
    {
        Point diff = point - source;
        distance = Squeue(diff.x) + Squeue(diff.y);
    }

    Point point;
    Point source;
    int distance;
};

/*
std::pair<int, ExpandingPoint>
ExpandingPair(const Point& point,
              const Point& source)
{
    Point diff = point - source;
    int dist = Squeue(diff.x) + Squeue(diff.y);
    return std::make_pair(dist, ExpandingPoint(point, source));
}
*/

struct ComparePoints
{
    bool operator()(const Point& p1,
                    const Point& p2) const
    {
        return (p1.y * (1<<16) + p1.x) < (p2.y * (1<<16) + p2.x);
    }
};

struct HashPoint
{
    size_t operator()(const Point& point) const
    {
        static std::hash<int> hash;
        return hash(point.y * (1<<16) + point.x);
    }
};

class ExpandingSet
{
public:
    void Update(const Point& point, const Point& src)
    {
        ExpandingPoint point_to_expand(point, src);

        auto found = _expanding_index.find(point);
        if (found != _expanding_index.end())
        {   //如果更新点在扩展点集中
            ExpandingBlock& block = *(found->second.first);
            ExpandingPoint& block_point = *(found->second.second);
            if(point_to_expand.distance < block_point.distance)
            {   //新距离较小，需要更新
                block.erase(found->second.second);
                _expanding_index.erase(found);
            }
            else
            {   //不需要更新
                return;
            }
        }

        int block_index = (int)(sqrt(point_to_expand.distance) * 3);
        ExpandingBlock& block = _expanding_map[block_index];
        block.push_front(point_to_expand);
        _expanding_index.insert(std::make_pair(point,
            std::make_pair(&block, block.begin())));
    }

    ExpandingPoint PopFirst()
    {
        ExpandingMap::iterator block_it;
        for (block_it = _expanding_map.begin();
             block_it->second.empty();
             block_it++)
        { }
        _expanding_map.erase(_expanding_map.begin(), block_it);

        ExpandingBlock& first_block = block_it->second;
        ExpandingPoint result = first_block.back();
        first_block.pop_back();
        _expanding_index.erase(result.point);
        return result;
    }

    bool Empty() const
    {
        return _expanding_index.empty();
    }
private:
    typedef std::list<ExpandingPoint> ExpandingBlock;
    typedef std::map<int, ExpandingBlock> ExpandingMap;

    ExpandingMap _expanding_map;
    std::unordered_map<Point,
                       std::pair<ExpandingBlock*,ExpandingBlock::iterator>,
                       HashPoint> _expanding_index;
};


/* 在一个以size指定的空间内，计算所有像素点离点集points的最近距离。
 * 返回一个矩阵，每个单元包含距离平方（(x1-x2)^2+(y1-y2)^2）和最近点
 * range指定最大距离，超过这个距离的点不再计算，并返回-1
 * 时间复杂度：O(N * log(N))  N = size.width*size.height
 */
template<class PointsCollection>
MatBase<std::pair<int, Point> > _GetDistMap(
    const Size& size, const PointsCollection& points,
    int range = std::numeric_limits<int>::max(),
    const MatBase<uint8_t>& mask = MatBase<uint8_t>())
{
    //结果网格
    MatBase<std::pair<int, Point> > dist_map(size);
    dist_map.Set(std::make_pair(-1, Point(-1,-1)));

    //扩展点集
    ExpandingSet expanding;

    //初始化，将目标点集加入到扩展点集
    for (auto& point : points)
    {
        assert(point.x >= 0 && point.x < size.width &&
               point.y >= 0 && point.y < size.height);
        expanding.Update(point, point);
    }

    while (!expanding.Empty())
    {
        ExpandingPoint expanding_point = expanding.PopFirst();

        //将这个点的最小距离和最近点更新到结果
        int dist_result = (int)sqrt(expanding_point.distance);
        dist_map[expanding_point.point] =
            std::make_pair(dist_result, expanding_point.source);
        if (dist_result >= range)
            continue;

        /*
        //更新范围：扩展点expanding_point为中心的3*3范围
        Rect update_area(expanding_point.point + Point(-1,-1), Size(3,3));
        //边缘防止越界
        update_area = OverlapArea(update_area, dist_map.WholeArea());
        //遍历更新范围
        for (auto& point : PointsIn(update_area))
        {
            if (dist_map[point].first < 0 &&
                (!mask.IsValid() || mask[point] > 0))
                expanding.Update(point, expanding_point.source);
        }
        */

        auto _Update = [&](int offx, int offy){
            Point point = expanding_point.point + Point(offx, offy);
            if (dist_map[point].first < 0 &&
                (!mask.IsValid() || mask[point] > 0))
                expanding.Update(point, expanding_point.source);
        };
        if (expanding_point.point.x > 0) _Update(-1,0);
        if (expanding_point.point.x < size.width - 1) _Update(1,0);
        if (expanding_point.point.y > 0) _Update(0,-1);
        if (expanding_point.point.y < size.height - 1) _Update(0,1);
    }

    return dist_map;
}

template<class PointCollection>
const Point GetNearestPoint(const Point& src_point, const PointCollection& dst_points)
{
    int min_dist = std::numeric_limits<int>::max();
    Point nearest_point;
    for (auto& point : dst_points)
    {
        Point diff = (point - src_point);
        int dist = Squeue(diff.x) + Squeue(diff.y);
        if (dist < min_dist)
        {
            min_dist = dist;
            nearest_point = point;
        }
    }
    return nearest_point;
}

struct BackSample
{
    explicit BackSample(const Point& center)
        : center(center)
    { }

    Point center;
    cv::Vec3i mean_back_color;
}; //struct BackSample

void _StatBackSample(BackSample& sample,
                     const MatBase<cv::Vec3b>& img)
{
    Rect sampling_area(
        sample.center - Point(BackSamplingRange,
                              BackSamplingRange),
        BackSamplingSize);
    sampling_area = OverlapArea(sampling_area, img.WholeArea());

    Median<uint8_t,3> median;
    median.Reserve(sampling_area.size.Total());
    for (auto& point : PointsIn(sampling_area))
        median.Push(img[point]);
    sample.mean_back_color = median.Get();
}

struct FrontSample
{
    explicit FrontSample(const Point& center)
        : center(center), back_sample(nullptr), kmeans(KFront)
    { }

    Point center;
    const BackSample* back_sample;
    KMeans<cv::Vec3i, DistanceOfVector<int,3>,
           MeanOnSphere<SphereRadius> > kmeans;
    cv::Vec3i mean_color[KFront];
    int mean_color_squeue[KFront];
    int mean_color_modulus[KFront];
}; //struct FrontSample

void _StatFrontSample(FrontSample& front_sample,
                      const BackSample* nearest_back_sample,
                      const MatBase<cv::Vec3b>& img)
{
    front_sample.back_sample = nearest_back_sample;
    cv::Vec3i back_color = nearest_back_sample->mean_back_color;

    //以平均背景色为球心，将像素颜色映射到一个球面上，并对前景像素执行k-means聚类。
    Rect sampling_area(front_sample.center -
                       Point(FrontSamplingRange,
                             FrontSamplingRange),
                       FrontSamplingSize);
    sampling_area = OverlapArea(sampling_area, img.WholeArea());
    MatBase<cv::Vec3i> sphere_map(sampling_area.size);
    std::vector<cv::Vec3i> pixels_diff_vec;
    for (auto& point : PointsIn(sampling_area))
    {
        const Point point_sub = point - sampling_area.point;
        cv::Vec3i sphere_vec =
            Normalize<int, 3>((cv::Vec3i)img[point] - back_color,
                              SphereRadius);
        sphere_map[point_sub] = sphere_vec;
        if (Squeue(point_sub.x) + Squeue(point_sub.y)
                <= Squeue<int>(FrontSamplingRange))
            pixels_diff_vec.push_back(sphere_vec);
    }
    for (int k = 0 ; k < KFront ; k++) //随便初始化kmeans聚类中心
        front_sample.kmeans.InitCenter(k, cv::Vec3i(k,0,0));
    front_sample.kmeans.Train(pixels_diff_vec.cbegin(),
                              pixels_diff_vec.cend());

    //统计每个分类的颜色中位数
    Median<int, 3> median[KFront];
    for (auto& point : PointsIn(sampling_area))
    {
        const Point point_sub = point - sampling_area.point;
        if (Squeue(point_sub.x) + Squeue(point_sub.y)
                <= Squeue<int>(FrontSamplingRange))
        {
            //前景分类
            int tag = front_sample.kmeans.GetTag(sphere_map[point_sub]);
            median[tag].Push((cv::Vec3i)img[point] - back_color);
        }
    }

    /*
    Mean<cv::Vec3i> meaning[KFront];
    for (auto& point : PointsIn(sampling_area))
    {
        const Point point_sub = point - sampling_area.point;
        if (Squeue(point_sub.x) + Squeue(point_sub.y)
                <= Squeue<int>(FrontSamplingRange))
        {
            //前景分类
            int tag = front_sample.kmeans.GetTag(sphere_map[point_sub]);
            meaning[tag].Push((cv::Vec3i)img[point] - back_color);
        }
    }
    */

    //将结果保存到sample
    for (int k = 0 ; k < KFront ; k++)
    {
        front_sample.mean_color[k] = median[k].Count() > 0 ?
            median[k].Get() : Normalize(front_sample.kmeans.GetCenter(k),100);
        front_sample.mean_color_squeue[k] =
            std::max(Squeue<int>(MinFrontBackDiff),
                     SqueueVec(front_sample.mean_color[k]));
        front_sample.mean_color_modulus[k] =
            sqrt(front_sample.mean_color_squeue[k]);
    }
}

template<class T>
bool Found(const MatBase<T>& mat, const Rect& area, const T& val)
{
    using sybie::common::Graphics::OverlapArea;
    for (auto& point : PointsIn(OverlapArea(area,mat.WholeArea())))
        if (mat[point] == val)
            return true;
    return false;
}

}  //namespace

cv::Mat MatBorder(const cv::Mat& image, const cv::Mat& mask)
{
/* 1）找出所有边缘像素
 * 2）计算图像上每一点与最近边缘像素的距离（一维距离）
 * 3）边缘像素指定距离（FrontSamplingDistance）的前景点为点集F（front_sampling_points）。
 *    边缘像素指定距离（BackSamplingDistance）的背景点为点集B（back_sampling_points）。
 * 4）B每点作为中心，一个指定大小（BackSamplingRange）为半边长的正方形范围内采样，
 *    采样点求平均值，作为该点背景色。
 * 5）F每点作为中心，一个指定大小（FrontSamplingRange）为半边长的正方形范围内采样，
 *    分别找到B中最近点的背景色，
 *    采样点减去背景色后映射到颜色空间的球面上，用k-means聚类，k=KFront，每个分类分别计算平均颜色。
 * 6）FrontSamplingPoints和BackSamplingPoints之间的像素为混合区域，
 *    对区域内每一点找到在F中的最近（欧几里德距离）点f，
 *    在f的采样分类中找到接近分类，分类的平均颜色为前景色，f的最近背景采样背景色作为alpha计算依据。
 */

    const MatBase<cv::Vec3b> _img =
        MakeConstWrapper<cv::Vec3b>(image);
    const MatBase<uint8_t> _mask =
        MakeConstWrapper<uint8_t>(mask);
    const Size _size = _img.GetSize();
    cv::Mat matte(image.rows, image.cols, CV_8UC4);
    MatBase<cv::Vec4b> _matte =
        MakeWrapper<cv::Vec4b>(matte);

    //1)
    std::vector<Point> border_points;
    {
        sybie::common::StatingTestTimer timer("_MatBorder:1");
        border_points = _GetBorderPoints(_mask);
    }

    //2)
    MatBase<std::pair<int, Point> > border_dist_map;
    {
        sybie::common::StatingTestTimer timer("_MatBorder:2");
        border_dist_map = _GetDistMap(_size, border_points,
           std::max<int>(FrontSamplingDistance, BackSamplingDistance) + 2);
        for (auto& point : PointsIn(_size))
        {
            int& distance = border_dist_map[point].first;
            if (distance == -1)
                distance = std::numeric_limits<int>::max();
            if (IsFront(_mask[point]))
                distance = -distance;
        }
    }

    //3)
    std::vector<Point> front_sampling_points,
                       back_sampling_points;
    std::map<Point,FrontSample,ComparePoints> front_samples;
    std::map<Point,BackSample,ComparePoints> back_samples;
    MatBase<uint8_t> border_mask(_size);
    MatBase<std::pair<int, Point> > front_dist_map;
    MatBase<std::pair<int, Point> > back_dist_map;

    {
        sybie::common::StatingTestTimer timer("_MatBorder:3");
        MatBase<uint8_t> sampling_mask(_size);
        for (auto& point : PointsIn(_size))
        {
            uint8_t& sampling_mask_point = sampling_mask[point];
            int dist = border_dist_map[point].first;
            if (dist == -FrontSamplingDistance &&
                !Found<uint8_t>(sampling_mask,
                    Rect::FromCenterSize(point, FrontSamplingStepArea), 1))
            {
                front_sampling_points.push_back(point);
                sampling_mask_point = 1;
            }
            else if (dist == BackSamplingDistance &&
                     !Found<uint8_t>(sampling_mask,
                         Rect::FromCenterSize(point, BackSamplingStepArea), 2))
            {
                back_sampling_points.push_back(point);
                sampling_mask_point = 2;
            }
            else
            {
                sampling_mask_point = 0;
            }
            border_mask[point] = (dist <= BackSamplingDistance+1 &&
                                  dist <= BackMattingRange+1 &&
                                  dist >= -FrontSamplingDistance-1 &&
                                  dist >= -FrontMattingRange-1);
        }

        for (auto& point : front_sampling_points)
            front_samples.insert(std::make_pair(point, FrontSample(point)));

        for (auto& point : back_sampling_points)
            back_samples.insert(std::make_pair(point, BackSample(point)));

        front_dist_map = _GetDistMap(
            _size, front_sampling_points,
            std::numeric_limits<int>::max(),
            border_mask);
        back_dist_map = _GetDistMap(
            _size, back_sampling_points,
            std::numeric_limits<int>::max(),
            border_mask);
    }

    //4)
    {
        sybie::common::StatingTestTimer timer("_MatBorder:4");
        for (auto& back_sample : back_samples)
            _StatBackSample(back_sample.second, _img);
    } //timer

    //5)
    {
        sybie::common::StatingTestTimer timer("_MatBorder:5");
        for (auto& front_sample_pair : front_samples)
        {
            Point nearest_back_point =
                back_dist_map[front_sample_pair.first].second;
            if (nearest_back_point.x == -1)
            {
                nearest_back_point = GetNearestPoint(
                    front_sample_pair.first, back_sampling_points);
            }
            const BackSample* nearest_back_sample =
                &back_samples.at(nearest_back_point);
            _StatFrontSample(front_sample_pair.second,
                             nearest_back_sample,
                             _img);
        }
    } //timer

    //6)
    {
        sybie::common::StatingTestTimer timer("_MatBorder:6");
        for (auto& point : PointsIn(_size))
        {
            const cv::Vec3i pixel = (cv::Vec3i)_img[point];
            cv::Vec4b& raw_pixel = _matte[point];
            uint8_t& alpha_pixel = raw_pixel[3];
            cv::Vec3b& back_pixel = *(cv::Vec3b*)&raw_pixel;

            int dist = border_dist_map[point].first;
            if (dist > -FrontMattingRange &&
                dist <= BackMattingRange &&
                front_dist_map[point].first >= 0)
            {
                //最近的前景样本点
                const FrontSample& front_sample =
                    front_samples.at(front_dist_map[point].second);
                //采样背景颜色
                const cv::Vec3i& back_color =
                    front_sample.back_sample->mean_back_color;

                const cv::Vec3i pixel_sphere =
                    Normalize<int>(pixel, SphereRadius);
                //std::cout<<SHOW(pixel_sphere)
                //         <<SHOW(pixel);
                Mean<double> mean_modulus;
                for (int k = 0 ; k < KFront ; k++)
                {
                    double distance =
                        front_sample.kmeans.DistanceOf(pixel_sphere, k);
                    double count =
                        front_sample.kmeans.Count(k);
                    mean_modulus.Push((double)front_sample.mean_color_modulus[k],
                                      count/(distance+1));

                    //std::cout<<SHOW(distance)
                    //         <<SHOW(count)
                    //         <<SHOW(front_sample.mean_color_modulus[k]);
                }
                const double front_modulus = mean_modulus.Get();
                const double modulus = ModulusOf(pixel - back_color);
                int alpha = (int)(255 * modulus / front_modulus);
                //std::cout<<modulus<<"/"<<front_modulus<<"="<<alpha<<std::endl;

                /*
                //前景分类
                int tag = front_sample.kmeans.GetTag(
                    Normalize(pixel - back_color, (int)SphereRadius));
                //采样前景颜色(相对背景色)
                const cv::Vec3i& front_color =
                    front_sample.mean_color[tag];

                //Alpha
                int alpha = 255
                    * front_color.dot(pixel - back_color)
                    / front_sample.mean_color_squeue[tag];
                */
                alpha_pixel = TruncByte(alpha);
                //背景色结果
                back_pixel =
                    TruncIntVec((back_color * alpha +
                                pixel * (255 - alpha))
                                / 255);
            }
            else
            {
                alpha_pixel = IsFront(_mask[point]) ? 255 : 0;
                back_pixel = _img[point];
            }
        }
    } //timer

    return matte;
}

cv::Mat MakeTrimap(const cv::Mat& image, const cv::Mat& mask)
{
    const MatBase<cv::Vec3b> _img =
        MakeConstWrapper<cv::Vec3b>(image);
    const MatBase<uint8_t> _mask =
        MakeConstWrapper<uint8_t>(mask);
    const Size _size = _img.GetSize();
    cv::Mat trimap(image.rows, image.cols, CV_8UC1);
    MatBase<uint8_t> _trimap =
        MakeWrapper<uint8_t>(trimap);

    std::vector<Point> border_points = _GetBorderPoints(_mask);
    MatBase<std::pair<int, Point> > border_dist_map =
        _GetDistMap(_size, border_points,
                    std::max<int>(FrontSamplingDistance, BackSamplingDistance) + 2);
    for (auto& point : PointsIn(_size))
    {
        int& distance = border_dist_map[point].first;
        if (IsFront(_mask[point]))
        {
            if (distance < FrontMattingRange && distance >=0 )
                _trimap[point] = 127;
            else
                _trimap[point] = 255;
        }
        else
        {
            if (distance <= BackMattingRange && distance >=0 )
                _trimap[point] = 127;
            else
                _trimap[point] = 0;
        }
    }

    return trimap;
}

} //namespace portrait
