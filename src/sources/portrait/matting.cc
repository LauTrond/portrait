//portrait/matting.cc

#include "portrait/matting.hh"

#include "sybie/common/Graphics/Structs.hh"
#include "sybie/common/RichAssert.hh"

#include "portrait/math.hh"
#include "portrait/graphics.hh"

namespace portrait {

//在边缘处理时用到的前景分类数，越大越准确、越慢。
enum {KFront = 4};

enum {SphereRadius = 0xfff};

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

int MatBorder(
    cv::Mat& raw, const cv::Mat& image, const cv::Mat& mask)
{
    sybie_assert(   raw.size() == image.size()
                 && image.size() == mask.size())
        << SHOW(raw.size())
        << SHOW(image.size())
        << SHOW(mask.size());
    const int rows = image.rows, cols = image.cols;

    //使用中位数计算标准背景色
    std::vector<uint8_t> back_vec[3];
    for (auto& v : back_vec)
        v.reserve(rows * cols);
    for (int r = 0 ; r < rows ; r++)
        for (int c = 0 ; c < cols ; c++)
        {
            if(IsFront(mask.at<uint8_t>(r,c)))
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
    cv::Vec3b back_color = TruncIntVec(back_color_int);

    //Debug:
    //std::cout<<back_color<<std::endl;

    //以平均背景色为圆心，将前景像素颜色映射到一个球面上，执行k-means聚类。
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
            if (IsFront(mask.at<uint8_t>(r,c)))
                samples.push_back(sphere_vec);
        }
    for (int k = 0 ; k < KFront ; k++) //随便初始化kmeans中心
        kmeans.InitCenter(k, cv::Vec3i(k,0,0));
    kmeans.Train(samples.begin(), samples.end());

    //Debug:
    /*
    for (int r = 0 ; r < rows ; r++)
    {
        for (int c = 0 ; c < cols ; c++)
            std::cout << sphere_map.at(c,r) << " ";
        std::cout<<std::endl;
    }
    for (int k = 0 ; k < KFront ; k++)
        std::cout<<kmeans.GetCenter(k)<<" ";
    std::cout<<std::endl;
    */

    //用kmeans结果对每个像素分类
    sybie::common::Graphics::MatBase<int> tag_map(
        sybie::common::Graphics::Size(cols, rows));
    for (int r = 0 ; r < rows ; r++)
        for (int c = 0 ; c < cols ; c++)
            tag_map.at(c,r) = kmeans.GetTag(sphere_map.at(c,r));

    //Debug:
    /*
    for (int r = 0 ; r < rows ; r++)
    {
        for (int c = 0 ; c < cols ; c++)
            std::cout << tag_map.at(c,r) << " ";
        std::cout<<std::endl;
    }
    */

    //计算分类内前景平均值（相对典型背景色）
    Mean<cv::Vec3i> mean_diff[KFront];
    for (int r = 0 ; r < rows ; r++)
        for (int c = 0 ; c < cols ; c++)
        {
            if (IsFront(mask.at<uint8_t>(r,c)))
                mean_diff[tag_map.at(c,r)].Push(
                    (cv::Vec3i)image.at<cv::Vec3b>(r,c) - back_color_int);
        }
    cv::Vec3i mean_diff_val[KFront];
    int mean_diff_squeue[KFront];
    for (int k = 0 ; k < KFront ; k++)
    {
        mean_diff_val[k] = mean_diff[k].Count() > 0 ?
            mean_diff[k].Get() : kmeans.GetCenter(k);
        mean_diff_squeue[k] = std::max(100, SqueueVec(mean_diff_val[k]));
    }

    //Debug:
    /*
    for (int k = 0 ; k < KFront ; k++)
        std::cout<<mean_diff_val[k];
    std::cout<<std::endl;
    */

    //计算alpha
    for (int r = 0 ; r < rows ; r++)
        for (int c = 0 ; c < cols ; c++)
        {
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
            *(cv::Vec3b*)&raw_pixel = back_color;
        }

    return -back_index;
}


} //namespace portrait
