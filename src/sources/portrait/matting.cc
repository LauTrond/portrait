//portrait/matting.cc

#include "portrait/matting.hh"

#include <functional>

#include "sybie/common/Graphics/Structs.hh"
#include "sybie/common/Graphics/CVCast.hh"
#include "sybie/common/RichAssert.hh"
#include "sybie/common/Time.hh"

#include "portrait/math.hh"
#include "portrait/graphics.hh"

namespace portrait {

using namespace sybie::common::Graphics;

namespace {

enum { FrontSamplingDistance = 7 };
enum { BackSamplingDistance = 30 };
enum { FrontSamplingRange = 5 };
enum { BackSamplingRange = 3 };
enum { FrontMattingRange = 10 };
enum { BackMattingRange = 30 };

//在边缘处理时用到的前景分类数，越大越准确、越慢。
enum {KFront = 4};

//球体映射的球体半径，足够大即可
enum {SphereRadius = 0xfff};

//Matting前景色和背景色最小距离（欧氏距离），太小易被噪声干扰，太大则精确度下降
enum {MinFrontBackDiff = 5};

//边缘大小
//enum {BorderSize = 1};

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

struct ExpendingPoint
{
    ExpendingPoint(const Point& point,
                   const Point& source)
        : point(point), source(source)
    { }

    Point point;
    Point source;
};

std::pair<int, ExpendingPoint>
ExpendingPair(const Point& point,
              const Point& source)
{
    Point diff = point - source;
    int dist = Squeue(diff.x) + Squeue(diff.y);
    return std::make_pair(dist, ExpendingPoint(point, source));
}

struct ComparePoints
{
    bool operator()(const Point& p1,
                    const Point& p2) const
    {
        return (p1.y * (1<<16) + p1.x) < (p2.y * (1<<16) + p2.x);
    }
};

/* 在一个以size指定的空间内，计算所有像素点离点集points的最近距离。
 * 返回一个矩阵，每个单元包含距离平方（(x1-x2)^2+(y1-y2)^2）和最近点
 * range指定最大距离，超过这个距离的点不再计算，并返回-1
 * 时间复杂度：O(N * log(N))  N = size.width*size.height
 */
template<class PointsCollection>
MatBase<std::pair<int, Point> > _GetDistMap(
    const Size& size, const PointsCollection& points,
    int range = std::numeric_limits<int>::max())
{
    //结果网格
    MatBase<std::pair<int, Point> > dist_map(size);
    dist_map.Set(std::make_pair(-1, Point(-1,-1)));

    //扩展点集
    std::multimap<int, ExpendingPoint> expanding;
    std::map<Point,
             std::multimap<int, ExpendingPoint>::iterator,
             ComparePoints> expanding_index;

    //初始化，将目标点集加入到扩展点集
    for (auto& point : points)
    {
        assert(point.x >= 0 && point.x < size.width &&
               point.y >= 0 && point.y < size.height);

        auto it = expanding.insert(ExpendingPair(point, point));
        expanding_index.insert(std::make_pair(point, it));
    }

    while (!expanding.empty())
    {
        //获得扩展点集中距离最小的点
        auto it = expanding.begin();
        const int expanding_dist = it->first;
        const Point expanding_point = it->second.point;
        const Point expanding_source = it->second.source;
        //从集合中删除这个点
        expanding_index.erase(expanding_point);
        expanding.erase(it);
        //将这个点的最小距离和最近点更新到结果
        int dist_result = (int)sqrt(expanding_dist);
        dist_map[expanding_point] =
            std::make_pair(dist_result, expanding_source);
        if (dist_result >= range)
            continue;

        //内部函数：更新一个点
        auto _Update = [&](const Point& updating_point){
            if (dist_map[updating_point].first < 0)
            {
                auto expend_pair = ExpendingPair(updating_point,
                                                 expanding_source);
                bool update = true;

                auto found_in_expanding = expanding_index.find(updating_point);
                if (found_in_expanding != expanding_index.end())
                {   //如果更新点在扩展点集中
                    if(found_in_expanding->second->first > expend_pair.first)
                    {   //新距离较小，需要更新
                        expanding.erase(found_in_expanding->second);
                        expanding_index.erase(found_in_expanding);
                    }
                    else
                    {   //不需要更新
                        update = false;
                    }
                }

                if (update)
                {
                    auto it = expanding.insert(expend_pair);
                    expanding_index.insert(std::make_pair(updating_point, it));
                }
            }
        };

        //更新范围：扩展点expanding_point为中心的3*3范围
        Rect update_area(expanding_point + Point(-1,-1), Size(3,3));
        //边缘防止越界
        update_area = OverlapArea(update_area, dist_map.WholeArea());
        //遍历更新范围
        for (auto& point : PointsIn(update_area))
            _Update(point);
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
        Size(BackSamplingRange * 2 + 1,
             BackSamplingRange * 2 + 1));
    sampling_area = OverlapArea(sampling_area, img.WholeArea());

    std::vector<uint8_t> back_vec[3];
    for (auto& v : back_vec)
        v.reserve(sampling_area.size.Total());
    for (auto& point : PointsIn(sampling_area))
    {
        const cv::Vec3b& pixel = img[point];
        for (int cn = 0 ; cn < 3 ; cn++)
            back_vec[cn].push_back(pixel[cn]);
    }

    for (auto& v : back_vec)
        std::sort(v.begin(), v.end());
    int back_index = back_vec[0].size() / 2;
    for (int cn = 0 ; cn < 3 ; cn++)
        sample.mean_back_color[cn] = back_vec[cn][back_index];
}

struct FrontSample
{
    explicit FrontSample(const Point& center)
        : center(center), back_sample(nullptr), kmeans(KFront)
    { }

    Point center;
    const BackSample* back_sample;
    KMeans<cv::Vec3i, DistanceOfVector<int, 3>,
           MeanOnSphere<SphereRadius> > kmeans;
    cv::Vec3i mean_color[KFront];
    int mean_color_squeue[KFront];
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
                       Size(FrontSamplingRange * 2 + 1,
                            FrontSamplingRange * 2 + 1));
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
        pixels_diff_vec.push_back(sphere_vec);
    }
    for (int k = 0 ; k < KFront ; k++) //随便初始化kmeans聚类中心
        front_sample.kmeans.InitCenter(k, cv::Vec3i(k,0,0));
    front_sample.kmeans.Train(pixels_diff_vec.cbegin(),
                              pixels_diff_vec.cend());

    Mean<cv::Vec3i> meaning[KFront];
    for (auto& point : PointsIn(sampling_area))
    {
        const Point point_sub = point - sampling_area.point;
        //前景分类
        int tag = front_sample.kmeans.GetTag(sphere_map[point_sub]);
        meaning[tag].Push((cv::Vec3i)img[point] - back_color);
    }
    for (int k = 0 ; k < KFront ; k++)
    {
        front_sample.mean_color[k] = meaning[k].Count() > 0 ?
            meaning[k].Get() : Normalize(front_sample.kmeans.GetCenter(k),100);
        front_sample.mean_color_squeue[k] =
            std::max(Squeue<int>(MinFrontBackDiff),
                     SqueueVec(front_sample.mean_color[k]));
    }
}

}  //namespace

void _MatBorder(cv::Mat& raw, const cv::Mat& image, const cv::Mat& mask)
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

    MatBase<cv::Vec4b> _raw =
        MakeWrapper<cv::Vec4b>(raw);
    const MatBase<cv::Vec3b> _img =
        MakeConstWrapper<cv::Vec3b>(image);
    const MatBase<uint8_t> _mask =
        MakeConstWrapper<uint8_t>(mask);
    const Size _size = _img.GetSize();

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
            else if (IsFront(_mask[point]))
                distance = -distance;
        }
    }

    //3)
    std::vector<Point> front_sampling_points,
                       back_sampling_points;
    std::map<Point,FrontSample,ComparePoints> front_samples;
    std::map<Point,BackSample,ComparePoints> back_samples;
    MatBase<std::pair<int, Point> > front_dist_map;
    //MatBase<std::pair<int, Point> > back_dist_map;

    {
        sybie::common::StatingTestTimer timer("_MatBorder:3");
        for (auto& point : PointsIn(_size))
        {
            int dist = border_dist_map[point].first;
            if (dist == -FrontSamplingDistance)
                front_sampling_points.push_back(point);
            if (dist == BackSamplingDistance)
                back_sampling_points.push_back(point);
        }

        for (auto& point : front_sampling_points)
            front_samples.insert(std::make_pair(point, FrontSample(point)));

        for (auto& point : back_sampling_points)
            back_samples.insert(std::make_pair(point, BackSample(point)));

        front_dist_map = _GetDistMap(
            _size, front_sampling_points,
            FrontSamplingDistance + BackMattingRange + 2);
        /*
        back_dist_map = _GetDistMap(
            _size, back_sampling_points,
            FrontSamplingDistance + BackSamplingRange + 2);
        */
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
            Point nearest_back_point = GetNearestPoint(
                front_sample_pair.first, back_sampling_points);
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
            cv::Vec4b& raw_pixel = _raw[point];
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
}

/*

enum MatMask
{
    MatMask_Back = 0,
    MatMask_Front = 1,
    MatMask_Border = 2
};

int MatBorder(
    cv::Mat& raw, const cv::Mat& image, const cv::Mat& mask)
{
    sybie_assert(   raw.size() == image.size()
                 && image.size() == mask.size())
        << SHOW(raw.size())
        << SHOW(image.size())
        << SHOW(mask.size());
    const int rows = image.rows, cols = image.cols;

    sybie::common::Graphics::MatBase<MatMask> mask_map(
        sybie::common::Graphics::Size(cols, rows));
    for (int r = 0 ; r < rows ; r++)
        for (int c = 0 ; c < cols ; c++)
            mask_map.at(c,r) = (MatMask)IsFront(mask.at<uint8_t>(r,c));
    for (int r = BorderSize ; r < rows - BorderSize ; r++)
        for (int c = BorderSize ; c < cols - BorderSize ; c++)
        {
            bool cur = IsFront(mask.at<uint8_t>(r,c));
            for (int rr = -BorderSize ; rr <= BorderSize ; rr++)
                for (int cc = -BorderSize ; cc <= BorderSize ; cc++)
                {
                    if (cur != IsFront(mask.at<uint8_t>(r+rr,c+cc)))
                        mask_map.at(c+cc,r+rr) = MatMask_Border; //边缘
                }
        }

    //使用中位数计算标准背景色
    std::vector<uint8_t> back_vec[3];
    for (auto& v : back_vec)
        v.reserve(rows * cols);
    for (int r = 0 ; r < rows ; r++)
        for (int c = 0 ; c < cols ; c++)
        {
            if(mask_map.at(c,r) != MatMask_Back)
                continue;
            const cv::Vec3b& pixel = image.at<cv::Vec3b>(r,c);
            for (int cn = 0 ; cn < 3 ; cn++)
                back_vec[cn].push_back(pixel[cn]);
        }
    for (auto& v : back_vec)
        std::sort(v.begin(), v.end());
    int back_index = back_vec[0].size() / 2;
    cv::Vec3i back_color_int(back_vec[0][back_index],
                             back_vec[1][back_index],
                             back_vec[2][back_index]);
    //cv::Vec3b back_color = TruncIntVec(back_color_int);

    //以平均背景色为球心，将像素颜色映射到一个球面上，并对前景像素执行k-means聚类。
    sybie::common::Graphics::MatBase<cv::Vec3i> sphere_map(
        sybie::common::Graphics::Size(cols, rows));
    std::vector<cv::Vec3i> samples;
    samples.reserve(rows * cols);
    KMeans<cv::Vec3i,
           DistanceOfVector<int, 3>,
           MeanOnSphere<SphereRadius> > kmeans(KFront);
    for (int r = 0 ; r < rows ; r++) //初始化分类样本
        for (int c = 0 ; c < cols ; c++)
        {
            cv::Vec3i sphere_vec =
                Normalize<int, 3>((cv::Vec3i)image.at<cv::Vec3b>(r,c)
                                     - back_color_int,
                                  SphereRadius);
            sphere_map.at(c,r) = sphere_vec;
            if (mask_map.at(c,r) == MatMask_Front)
                samples.push_back(sphere_vec);
        }
    for (int k = 0 ; k < KFront ; k++) //随便初始化kmeans聚类中心
        kmeans.InitCenter(k, cv::Vec3i(k,0,0));
    kmeans.Train(samples.cbegin(), samples.cend());

    //用kmeans结果对每个像素分类
    sybie::common::Graphics::MatBase<int> tag_map(
        sybie::common::Graphics::Size(cols, rows));
    for (int r = 0 ; r < rows ; r++)
        for (int c = 0 ; c < cols ; c++)
            tag_map.at(c,r) = kmeans.GetTag(sphere_map.at(c,r));

    //计算分类内前景平均值（相对典型背景色）
    Mean<cv::Vec3i> mean_diff[KFront];
    for (int r = 0 ; r < rows ; r++)
        for (int c = 0 ; c < cols ; c++)
        {
            if (mask_map.at(c,r) == MatMask_Front)
                mean_diff[tag_map.at(c,r)].Push(
                    (cv::Vec3i)image.at<cv::Vec3b>(r,c) - back_color_int);
        }
    cv::Vec3i mean_diff_val[KFront];
    int mean_diff_squeue[KFront];
    Mean<int> all_diff_squeue;
    for (int k = 0 ; k < KFront ; k++)
    {
        mean_diff_val[k] = mean_diff[k].Count() > 0 ?
            mean_diff[k].Get() : Normalize(kmeans.GetCenter(k),100);
        mean_diff_squeue[k] = std::max(Squeue<int>(MinFrontBackDiff),
                                       SqueueVec(mean_diff_val[k]));
        all_diff_squeue.Push(mean_diff_squeue[k], kmeans.Count(k));
    }

    //计算alpha
    for (int r = 0 ; r < rows ; r++)
        for (int c = 0 ; c < cols ; c++)
        {
            const cv::Vec3b& pixel = image.at<cv::Vec3b>(r,c);
            cv::Vec4b& raw_pixel = raw.at<cv::Vec4b>(r,c);

            //Alpha
            int tag = tag_map.at(c,r);
            int alpha = 255
                * DotProduct(mean_diff_val[tag],
                             (cv::Vec3i)image.at<cv::Vec3b>(r,c)
                               - back_color_int)
                / mean_diff_squeue[tag];
            raw_pixel[3] = TruncByte(alpha);

            //背景色
            *(cv::Vec3b*)&raw_pixel =
                TruncIntVec((back_color_int * alpha +
                            (cv::Vec3i)pixel * (255 - alpha))
                            / 255);
        }

    return -all_diff_squeue.Get();
}

*/

} //namespace portrait
