//这是对algorithm.hh的实现
#include "portrait/algorithm.hh"

#include <algorithm>
#include <cmath>

#include "sybie/common/RichAssert.hh"
#include "sybie/common/Time.hh"

namespace portrait {

const int
    BorderSize = 5,
    MixSize = BorderSize * 2 + 1;

const int GrabCutInteration = 3;
//GrabCut的图片大小
const double
    GrabCutWidthScale = 0.5,
    GrabCutHeightScale = 0.5;
const double
    GrabCutInitWidthScale = 0.2,
    GrabCutInitHeightScale = 0.2;

//以下多个常数定义前景、背景划分的关键数值，全是检测出人脸矩形的长宽比例。

//绝对背景
const double
    BGWidth = 0.25,
    BGTop = 0.50,
    BGBottom = 0.00;
//可能背景
const double
    PR_BGWidth = 0.10,
    PR_BGTop = 0.30,
    PR_BGBottom = 0.10;
//绝对前景－脸
const double
    FGFaceTop = 0.05,
    FGFaceSide = -0.25,
    FGFaceBottom = -0.30;
//绝对前景-颈
const double
    FGNeckSide = -0.35;
//绝对前景-身体
const double
    FGBodyTop = 0.30,
    FGBodySide = 0.00;

cv::Point CenterOf(const cv::Rect& rect)
{
    return cv::Point(rect.x + rect.width / 2,
                     rect.y + rect.height / 2);
}

cv::Rect WholeArea(const cv::Mat& image)
{
    return cv::Rect(0, 0, image.cols, image.rows);
}

cv::Point TopLeft(const cv::Mat& image)
{
    return cv::Point(0,0);
}

cv::Point TopRight(const cv::Mat& image)
{
    return cv::Point(image.cols - 1, 0);
}

cv::Point BottomLeft(const cv::Mat& image)
{
    return cv::Point(0, image.rows - 1);
}

cv::Point TopLeft(const cv::Rect& rect)
{
    return cv::Point(rect.x, rect.y);
}

cv::Point TopRight(const cv::Rect& rect)
{
    return cv::Point(rect.x + rect.width, rect.y);
}

cv::Point BottomLeft(const cv::Rect& rect)
{
    return cv::Point(rect.x, rect.y + rect.height);
}

cv::Point BottomRight(const cv::Rect& rect)
{
    return cv::Point(rect.x + rect.width, rect.y + rect.height);
}


cv::Point BottomRight(const cv::Mat& image)
{
    return cv::Point(image.cols - 1, image.rows - 1);
}

bool Inside(const cv::Rect& rect_inner, const cv::Rect& rect_outter)
{
    return rect_inner.x >= rect_outter.x
        && rect_inner.y >= rect_outter.y
        && rect_inner.x + rect_inner.width
           <= rect_outter.x + rect_outter.width
        && rect_inner.y + rect_inner.height
           <= rect_outter.y + rect_outter.height;
}

bool Inside(const cv::Rect& rect_inner, const cv::Mat& image)
{
    return Inside(rect_inner, WholeArea(image));
}

cv::Rect OverlapArea(const cv::Rect& rect1, const cv::Rect& rect2)
{
    int overlap_left   = std::max(rect1.x, rect2.x);
    int overlap_top    = std::max(rect1.y, rect2.y);
    int overlap_right  = std::min(rect1.x + rect1.width,  rect2.x + rect2.width );
    int overlap_bottom = std::min(rect1.y + rect1.height, rect2.y + rect2.height);
    if (overlap_top >= overlap_bottom || overlap_left >= overlap_right)
        return cv::Rect(rect1.x, rect1.y, 0, 0);
    else
        return cv::Rect(
            overlap_left,
            overlap_top,
            overlap_right - overlap_left,
            overlap_bottom - overlap_top);
}

cv::Rect SubArea(const cv::Rect& rect1, const cv::Point& offset)
{
    return cv::Rect(TopLeft(rect1) - offset, rect1.size());
}

cv::Rect SubArea(const cv::Rect& rect1, const cv::Rect& rect2)
{
    return SubArea(rect1, TopLeft(rect2));
}

inline int DotProduct(const cv::Vec3i& vec1, const cv::Vec3i& vec2)
{
    return vec1[0] * vec2[0] + vec1[1] * vec2[1] + vec1[2] * vec2[2];
}

inline int DotProduct(const cv::Vec3i& vec)
{
    return DotProduct(vec, vec);
}

inline int ModulusOf(const cv::Vec3i& vec)
{
    return (int)sqrt(DotProduct(vec, vec));
}

cv::Rect TryCutPortrait(
    cv::Mat& image,
    const cv::Rect& face_area,
    const double max_up_expand,
    const double max_down_expand,
    const double max_width_expand)
{
    sybie_assert(Inside(face_area, image))
        << SHOW(face_area)
        << SHOW(image.rows)
        << SHOW(image.cols);

    cv::Rect area = face_area;
    area.x = face_area.x - (int)(face_area.width * max_width_expand);
    area.width = (int)(face_area.width * (1 + max_width_expand * 2));
    area.y = face_area.y - (int)(face_area.height * max_up_expand);
    area.height = (int)(face_area.height * (1 + max_up_expand + max_down_expand));

    area = OverlapArea(area, WholeArea(image));
    image = image(area);
    return cv::Rect(face_area.x - area.x, face_area.y - area.y,
                    face_area.width, face_area.height);
}

cv::Rect ResizeFace(
    cv::Mat& image,
    const cv::Rect& face_area,
    const cv::Size& face_resize_to)
{
    sybie_assert(Inside(face_area, image))
        << SHOW(face_area)
        << SHOW(image.rows)
        << SHOW(image.cols);

    double scale_x = (double)face_resize_to.width / face_area.width;
    double scale_y = (double)face_resize_to.height / face_area.height;
    cv::Size new_size(image.cols * scale_x, image.rows * scale_x);
    cv::Mat resized_image;
    cv::resize(image, resized_image, new_size, 0, 0, cv::INTER_AREA);
    image = resized_image;

    return cv::Rect (face_area.x * scale_x,
                     face_area.y * scale_y,
                     face_resize_to.width,
                     face_resize_to.height);
}

static void DrawMask(
    cv::Mat& image,
    const cv::Rect& face_area,
    bool clear,
    const cv::Scalar& clear_with_color,
    const cv::Scalar& front_color,
    const cv::Scalar& pr_back_color,
    const cv::Scalar& back_color,
    int thickness )
{
    //分割线
    int bg_up = face_area.y - face_area.height * BGTop;
    int bg_down = face_area.y + face_area.height * (1 + BGBottom);
    int bg_left = face_area.x - face_area.width * BGWidth;
    int bg_right = face_area.x + face_area.width * (1 + BGWidth);
    int pr_bg_up = face_area.y - face_area.height * PR_BGTop;
    int pr_bg_down = face_area.y + face_area.height * (1 + PR_BGBottom);
    int pr_bg_left = face_area.x - face_area.width * PR_BGWidth;
    int pr_bg_right = face_area.x + face_area.width * (1 + PR_BGWidth);
    int fgface_up = face_area.y - face_area.height * FGFaceTop;
    int fgface_down = face_area.y + face_area.height * (1 + FGFaceBottom);
    int fgface_left = face_area.x - face_area.width * FGFaceSide;
    int fgface_right = face_area.x + face_area.width * (1 + FGFaceSide);
    int fgneck_left = face_area.x - face_area.width * FGNeckSide;
    int fgneck_right = face_area.x + face_area.width * (1 + FGNeckSide);
    int fgbody_up = face_area.y + face_area.height * (1 + FGBodyTop);
    int fgbody_left = face_area.x - face_area.width * FGBodySide;
    int fgbody_right = face_area.x + face_area.width * (1 + FGBodySide);

    if (clear)
        cv::rectangle(image, WholeArea(image), clear_with_color, CV_FILLED);
    cv::rectangle(image,
                  TopLeft(image),
                  cv::Point(pr_bg_left - 1, pr_bg_down - 1),
                  pr_back_color, thickness);
    cv::rectangle(image,
                  TopRight(image),
                  cv::Point(pr_bg_right - 1, pr_bg_down - 1),
                  pr_back_color, thickness);
    cv::rectangle(image,
                  cv::Point(pr_bg_left, 0),
                  cv::Point(pr_bg_right - 1, pr_bg_up - 1),
                  pr_back_color, thickness);
    cv::rectangle(image,
                  TopLeft(image),
                  cv::Point(bg_left - 1, bg_down - 1),
                  back_color, thickness);
    cv::rectangle(image,
                  TopRight(image),
                  cv::Point(bg_right - 1, bg_down - 1),
                  back_color, thickness);
    cv::rectangle(image,
                  cv::Point(bg_left, 0),
                  cv::Point(bg_right - 1, bg_up - 1),
                  back_color, thickness);
    cv::rectangle(image,
                  cv::Point(fgface_left, fgface_up),
                  cv::Point(fgface_right - 1, fgface_down - 1),
                  front_color, thickness);
    cv::rectangle(image,
                  cv::Point(fgneck_left, fgface_down),
                  cv::Point(fgneck_right - 1, fgbody_up - 1),
                  front_color, thickness);
    cv::rectangle(image,
                  cv::Point(fgbody_left, fgbody_up),
                  cv::Point(fgbody_right - 1, image.rows - 1),
                  front_color, thickness);
}

cv::Mat GetFrontBackMask(
    const cv::Mat& image,
    const cv::Rect& face_area,
    const cv::Mat& stroke)
{
    sybie::common::StatingTestTimer timer("GetFrontBackMask");
    sybie_assert(Inside(face_area, image))
        << SHOW(face_area)
        << SHOW(image.rows)
        << SHOW(image.cols);

    //初始化前景/背景掩码
    cv::Mat mask(image.rows, image.cols, CV_8UC1);
    DrawMask(mask, face_area, true, cv::GC_PR_FGD,
             cv::GC_FGD, cv::GC_PR_BGD, cv::GC_BGD, CV_FILLED);

    //自定义的关键点
    if (stroke.data != nullptr)
    {
        for (int r = 0 ; r < stroke.rows ; r++)
            for (int c = 0 ; c < stroke.cols ; c++)
            {
                uint8_t stroke_point = stroke.at<uint8_t>(r,c);
                if ( stroke_point == cv::GC_FGD ||
                     stroke_point == cv::GC_BGD)
                    mask.at<uint8_t>(r,c) = stroke_point;
            }
    }

    //使用cv::grabCut分离前景和背景
    {
        sybie::common::StatingTestTimer timer("GetFrontBackMask.grabCut");

        cv::Size full_size(image.cols,
                           image.rows); //缩略图尺寸
        cv::Size grab_size(image.cols * GrabCutWidthScale,
                           image.rows * GrabCutHeightScale); //GrabCut缩略图尺寸
        cv::Size init_size(image.cols * GrabCutInitWidthScale,
                           image.rows * GrabCutInitHeightScale); //GrabCut初始化尺寸

        cv::Mat bgModel,fgModel; //前景模型、背景模型

        //初始化模型
        cv::Mat image_init, mask_init;
        cv::resize(image, image_init, init_size, 0, 0, cv::INTER_AREA);
        cv::resize(mask, mask_init, init_size, 0, 0, cv::INTER_NEAREST);
        cv::grabCut(image_init, mask_init, cv::Rect(),
                    bgModel,fgModel,
                    0, cv::GC_INIT_WITH_MASK);

        //抠图
        cv::Mat image_grab, mask_grab;
        cv::resize(image, image_grab, grab_size, 0, 0, cv::INTER_AREA);
        cv::resize(mask, mask_grab, grab_size, 0, 0, cv::INTER_NEAREST);
        cv::grabCut(image_grab, mask_grab, cv::Rect(),
                    bgModel,fgModel,
                    GrabCutInteration, cv::GC_EVAL);

        //抠图结果恢复到最大尺寸
        cv::resize(mask_grab, mask,
                   full_size, 0, 0,
                   cv::INTER_NEAREST);
    }

    //找出边缘像素
    cv::Mat mask_border(image.rows, image.cols, CV_8UC1);
    {
        sybie::common::StatingTestTimer timer("GetFrontBackMask.FindBorder");
        for (int r = 0 ; r < image.rows ; r++)
            for (int c = 0 ; c < image.cols ; c++)
            {
                uint8_t& mb = mask_border.at<uint8_t>(r,c);
                bool has_front = false;
                bool has_back = false;
                for (int rr = std::max(r-BorderSize,0) ;
                         rr <= std::min(r+BorderSize,image.rows-1) ;
                         rr++)
                    for (int cc = std::max(c-BorderSize,0) ;
                             cc <= std::min(c+BorderSize,image.cols-1) ;
                             cc++)
                    {
                        uint8_t& m = mask.at<uint8_t>(rr,cc);
                        if (m == cv::GC_BGD || m == cv::GC_PR_BGD)
                            has_back = true;
                        else
                            has_front = true;
                    }
                if (has_front && has_back)
                    mb = 128;
                else if (has_front)
                    mb = 255;
                else
                    mb = 0;
            }
    }

    //计算边缘的混合比例

    cv::Mat result(image.rows, image.cols, CV_8UC4);
    {
        sybie::common::StatingTestTimer timer("GetFrontBackMask.CalcAlpha");
        for (int r = 0 ; r < image.rows ; r++)
            for (int c = 0 ; c < image.cols ; c++)
            {
                uint8_t& mb = mask_border.at<uint8_t>(r,c);

                cv::Vec4b& p = result.at<cv::Vec4b>(r,c);
                uint8_t& alpha = p[3];
                cv::Vec3b& backc = *(cv::Vec3b*)(void*)&p;

                alpha = mb; //默认混合
                backc = image.at<cv::Vec3b>(r,c); //默认背色

                if (mb == 128) //边缘像素，计算混合比例
                {
                    int cnt_back = 0, cnt_front = 0;
                    cv::Vec3i sum_back(0,0,0), sum_front(0,0,0);

                    for (int rr = std::max(r-MixSize,0) ;
                             rr <= std::min(r+MixSize,image.rows-1) ;
                             rr++)
                        for (int cc = std::max(c-MixSize,0) ;
                                 cc <= std::min(c+MixSize,image.cols-1) ;
                                 cc++)
                        {
                            uint8_t& mbrange = mask_border.at<uint8_t>(rr,cc);
                            const cv::Vec3b& pixel = image.at<cv::Vec3b>(rr,cc);
                            if (mbrange == 0)
                                sum_back += pixel, cnt_back++;
                            if (mbrange == 255)
                                sum_front += pixel, cnt_front++;
                        }

                    if (cnt_back > 0 && cnt_front > 0)
                    {
                        cv::Vec3i cur = (cv::Vec3i)image.at<cv::Vec3b>(r,c);

                        cv::Vec3i adv_back = sum_back / cnt_back,
                                  adv_front = sum_front / cnt_front;
                        cv::Vec3i diff_cur_back = cur - adv_back,
                                  diff_front_back = adv_front - adv_back;
                        int alpha_int = 255 * DotProduct(diff_cur_back, diff_front_back) /
                                        std::max(1, DotProduct(diff_front_back, diff_front_back));
                        alpha = std::max(0, std::min(255, alpha_int));

                        /*int dist_back = cnt_front * ModulusOf(sum_back - cur * cnt_back);
                        int dist_front = cnt_back * ModulusOf(sum_front - cur * cnt_front);
                        if (dist_front + dist_back > 0)
                            alpha = 255 * dist_back / (dist_front + dist_back);*/
                    }
                    else if (cnt_back > 0 || cnt_front > 0)
                    {
                        alpha = 255 * cnt_front / (cnt_back + cnt_front);
                    }

                    if (cnt_back > 0)
                        backc = sum_back / cnt_back;
                }
            }
    }

    return result;
}

//GetMixRaw函数内使用的组件
namespace {

inline bool IsFront(uint8_t val)
{
    return val == cv::GC_FGD || val == cv::GC_PR_FGD;
}

inline bool IsBack(uint8_t val)
{
    return val == cv::GC_BGD || val == cv::GC_PR_BGD;
}

template<class T>
inline T Squeue(T a)
{
    return a * a;
}

template<class Tval, class Tcnt = int>
class Mean
{
public:
    Mean() : _sum(), _count(0) { }
    Mean(const Tval& zero) : _sum(zero), _count(0) { }

    void Count(const Tval& val)
    {
        _sum += val;
        _count++;
    }

    Tval Get() const
    {
        if (_count == 0)
            return _sum;
        else
            return _sum / _count;
    }

    Tcnt Count() const
    {
        return _count;
    }

    Tval Sum() const
    {
        return _sum;
    }
private:
    Tval _sum;
    Tcnt _count;
}; //template<class T> class Mean

template<class T>
uint8_t TruncByte(T val)
{
    return (uint8_t)std::max<T>(0, std::min<T>(255, val));
}

cv::Vec3b TruncVec(const cv::Vec3i& val)
{
    return cv::Vec3b(TruncByte(val[0]),
                     TruncByte(val[1]),
                     TruncByte(val[2]));
}

template<class T>
void CheckedFloodFill(cv::Mat& image,
                             const cv::Point& seed_point,
                             const cv::Scalar& old_val,
                             const cv::Scalar& new_val)
{
    if (cv::Scalar(image.at<T>(seed_point)) == old_val)
        cv::floodFill(image, seed_point, new_val);
}

void Clear(cv::Mat& mask)
{
    cv::Mat mask_tmp(mask.rows, mask.cols, CV_8UC1);
    for (int r = 0 ; r < mask.rows ; r++)
        for (int c = 0 ; c < mask.cols ; c++)
            mask_tmp.at<uint8_t>(r,c) = IsFront(mask.at<uint8_t>(r,c));

    CheckedFloodFill<uint8_t>(
        mask_tmp, cv::Point(0,0), 0, 2);
    CheckedFloodFill<uint8_t>(
        mask_tmp, cv::Point(mask_tmp.cols-1,0),  0, 2);
    CheckedFloodFill<uint8_t>(
        mask_tmp, cv::Point(mask_tmp.cols/2,mask_tmp.rows-1), 1, 3);

    for (int r = 0 ; r < mask.rows ; r++)
        for (int c = 0 ; c < mask.cols ; c++)
        {
            uint8_t t = mask_tmp.at<uint8_t>(r,c) ;
            uint8_t& m = mask.at<uint8_t>(r,c);
            if (t == 1 && m == cv::GC_PR_FGD) //孤立前景
                m = cv::GC_PR_BGD;
            if (t == 0 && m == cv::GC_PR_BGD) //孤立背景
                m = cv::GC_PR_FGD;
        }
}

int CalcBorderRaw(
    cv::Mat& raw, const cv::Mat& image, const cv::Mat& mask)
{
    sybie_assert(   raw.size() == image.size()
                 && image.size() == mask.size())
        << SHOW(raw.size())
        << SHOW(image.size())
        << SHOW(mask.size());

    //计算前景和背景平均值
    Mean<cv::Vec3i> mean[2];
    mean[0] = mean[1] = Mean<cv::Vec3i>(cv::Vec3i(0,0,0));
    for (int r = 0 ; r < image.rows ; r++)
        for (int c = 0 ; c < image.cols ; c++)
        {
            //在循环内避免使用条件判断（避免CPU分支预测错误）以提高性能
            mean[IsFront(mask.at<uint8_t>(r,c))]
                .Count((cv::Vec3i)image.at<cv::Vec3b>(r,c));
        }
    cv::Vec3i mean_val[2];
    for (int i = 0 ; i < 2 ; i++)
        mean_val[i] = mean[i].Get();
    const cv::Vec3i mean_back = mean_val[0],
                    mean_front = mean_val[1];
    cv::Vec3i diff = mean_front - mean_back;
    int abs_diff = DotProduct(diff);
    if (abs_diff == 0)
        abs_diff = 1;

    //计算前景和背景方差
    int sum_squeue_diff[2] = {1, 1};
    for (int r = 0 ; r < image.rows ; r++)
        for (int c = 0 ; c < image.cols ; c++)
        {
            int is_front = IsFront(mask.at<uint8_t>(r,c));
            sum_squeue_diff[is_front] +=
                DotProduct((cv::Vec3i)image.at<cv::Vec3b>(r,c) - mean_val[is_front]);
        }

    std::unique_ptr<int[]> rgr_map(new int[image.rows * image.cols]);
    std::vector<int> rgr_vec[2];
    for (auto& v : rgr_vec)
        v.reserve(image.rows * image.cols);
    for (int r = 0 ; r < image.rows ; r++)
        for (int c = 0 ; c < image.cols ; c++)
        {
            int rgr_val = DotProduct(
                diff, (cv::Vec3i)image.at<cv::Vec3b>(r,c) - mean_back);
            rgr_map[r * image.cols + c] = rgr_val;
            rgr_vec[IsFront(mask.at<uint8_t>(r,c))]
                .push_back(rgr_val);
        }
    for (auto& v : rgr_vec)
        std::sort(v.begin(), v.end());
    int rgr_back = rgr_vec[0][rgr_vec[0].size() / 4],
        rgr_front = rgr_vec[1][rgr_vec[1].size() / 2];
    int rgr_diff = rgr_front - rgr_back;
    if (rgr_diff == 0)
        rgr_diff = 1;

    cv::Vec3i back_int = diff * ((double)rgr_back / abs_diff) + mean_back;
    cv::Vec3b back_byte = TruncVec(back_int);
    for (int r = 0 ; r < image.rows ; r++)
        for (int c = 0 ; c < image.cols ; c++)
        {
            cv::Vec4b& raw_pixel = raw.at<cv::Vec4b>(r,c);
            int rgr_val = rgr_map[r * image.cols + c];
            int alpha = 255 * (rgr_val - rgr_back) / rgr_diff;
            raw_pixel[3] = TruncByte(alpha);
            *(cv::Vec3b*)&raw_pixel = back_byte;
            //*(cv::Vec3b*)&raw_pixel = mean_back;
        }

    return sum_squeue_diff[0] * sum_squeue_diff[1];
}

} //namespace GetMixRaw内使用的组件

cv::Mat GetMixRaw(
    const cv::Mat& image,
    const cv::Rect& face_area,
    const cv::Mat& stroke)
{
    sybie_assert(Inside(face_area, image))
        << SHOW(face_area)
        << SHOW(image.rows)
        << SHOW(image.cols);

    //初始化前景/背景掩码
    cv::Mat mask(image.rows, image.cols, CV_8UC1);
    DrawMask(mask, face_area, true, cv::GC_PR_FGD,
             cv::GC_FGD, cv::GC_PR_BGD, cv::GC_BGD, CV_FILLED);

    //自定义的关键点
    if (stroke.data != nullptr)
    {
        for (int r = 0 ; r < stroke.rows ; r++)
            for (int c = 0 ; c < stroke.cols ; c++)
            {
                uint8_t stroke_point = stroke.at<uint8_t>(r,c);
                if ( stroke_point == cv::GC_FGD ||
                     stroke_point == cv::GC_BGD)
                    mask.at<uint8_t>(r,c) = stroke_point;
            }
    }

    //使用cv::grabCut分离前景和背景
    {
        sybie::common::StatingTestTimer timer("GetMixRaw.grabCut");

        cv::Size full_size(image.cols,
                           image.rows); //缩略图尺寸
        cv::Size grab_size(image.cols * GrabCutWidthScale,
                           image.rows * GrabCutHeightScale); //GrabCut缩略图尺寸
        cv::Size init_size(image.cols * GrabCutInitWidthScale,
                           image.rows * GrabCutInitHeightScale); //GrabCut初始化尺寸

        cv::Mat bgModel,fgModel; //前景模型、背景模型

        //初始化模型
        cv::Mat image_init, mask_init;
        cv::resize(image, image_init, init_size, 0, 0, cv::INTER_AREA);
        cv::resize(mask, mask_init, init_size, 0, 0, cv::INTER_NEAREST);
        cv::grabCut(image_init, mask_init, cv::Rect(),
                    bgModel,fgModel,
                    0, cv::GC_INIT_WITH_MASK);

        //抠图
        cv::Mat image_grab, mask_grab;
        cv::resize(image, image_grab, grab_size, 0, 0, cv::INTER_AREA);
        cv::resize(mask, mask_grab, grab_size, 0, 0, cv::INTER_NEAREST);
        cv::grabCut(image_grab, mask_grab, cv::Rect(),
                    bgModel,fgModel,
                    GrabCutInteration, cv::GC_EVAL);

        //抠图结果恢复到最大尺寸
        cv::resize(mask_grab, mask,
                   full_size, 0, 0,
                   cv::INTER_NEAREST);
    }

    {
        sybie::common::StatingTestTimer timer("GetMixRaw.Clear");
        Clear(mask);
    }

    cv::Mat raw(image.rows, image.cols, CV_8UC4);
    {
        sybie::common::StatingTestTimer timer("GetMixRaw.FindBorder");

        cv::Mat tmp_raw(BorderSize * 2 + 1, BorderSize * 2 + 1, CV_8UC4);
        std::unique_ptr<int[]> dist_map(new int[image.rows * image.cols]);

        for (int r = 0 ; r < image.rows ; r++)
            for (int c = 0 ; c < image.cols ; c++)
            {
                const cv::Vec3b& pixel = image.at<cv::Vec3b>(r,c);
                const uint8_t pixel_alpha =
                    IsFront(mask.at<uint8_t>(r,c)) ? 0xff : 0;
                raw.at<cv::Vec4b>(r,c) = cv::Vec4b(
                    pixel[0], pixel[1], pixel[2], pixel_alpha);
                dist_map[r * image.cols + c] = 0x7fffffff;
            }
        for (int r = BorderSize ; r < image.rows - BorderSize ; r++)
            for (int c = BorderSize ; c < image.cols - BorderSize ; c++)
            {
                if (IsBack(mask.at<uint8_t>(r,c)))
                    continue;

                if (   IsFront(mask.at<uint8_t>(r - 1,c))
                    && IsFront(mask.at<uint8_t>(r + 1,c))
                    && IsFront(mask.at<uint8_t>(r,c - 1))
                    && IsFront(mask.at<uint8_t>(r,c + 1)))
                    continue;

                //边缘像素，计算边缘混合比例
                cv::Rect sub_area(c - BorderSize,
                                  r - BorderSize,
                                  BorderSize * 2 + 1,
                                  BorderSize * 2 + 1);
                int dist_cur = CalcBorderRaw(tmp_raw,
                                             image(sub_area),
                                             mask(sub_area));
                for (int rr = -BorderSize ; rr <= BorderSize ; rr++)
                    for (int cc = -BorderSize ; cc <= BorderSize ; cc++)
                    {
                        int& dist_old = dist_map[(r + rr) * image.cols + (c + cc)];
                        if (dist_old > dist_cur)
                        {
                            dist_old = dist_cur;
                            raw.at<cv::Vec4b>(r + rr, c + cc) =
                                tmp_raw.at<cv::Vec4b>(BorderSize + rr, BorderSize + cc);
                        }
                    }
            }
    }

    return raw;
}

void DrawGrabCutLines(
    cv::Mat& image,
    const cv::Rect& face_area)
{
    DrawMask(image, face_area,
             false, cv::Scalar(0,0,0),
             cv::Scalar(255,0,0), cv::Scalar(0,255,0), cv::Scalar(0,0,255),
             1);
}

cv::Rect Extend(
    cv::Mat& image,
    const cv::Rect& area,
    const cv::Scalar& border_pixel)
{
    int top = std::max(0, -area.y);
    int bottom = std::max(0, area.y + area.height - image.rows);
    int left = std::max(0, -area.x);
    int right = std::max(0, area.x + area.width - image.cols);
    cv::Mat image_new;
    cv::copyMakeBorder(image, image_new,
                       top, bottom, left, right,
                       cv::BORDER_CONSTANT, border_pixel);
    image = image_new;
    return SubArea(area, cv::Point(-left, -top));
}

cv::Mat Mix(
    const cv::Mat& image,
    const cv::Mat& raw,
    const cv::Vec3b& back_color,
    const double mix_alpha)
{
    assert(mix_alpha >= 0 && mix_alpha <= 1);
    cv::Mat image_mix(image.rows, image.cols, CV_8UC3);
    for (int r = 0 ; r < image.rows ; r++)
        for (int c = 0 ; c < image.cols ; c++)
        {
            const cv::Vec4b& p = raw.at<cv::Vec4b>(r,c);
            const uint8_t& pixiel_alpha = p[3];
            const cv::Vec3b& backc = *(const cv::Vec3b*)(const void*)&p;

            const cv::Vec3b& src = image.at<cv::Vec3b>(r,c);
            cv::Vec3b& mix = image_mix.at<cv::Vec3b>(r,c);
            mix = (cv::Vec3i)src + (((cv::Vec3i)back_color - (cv::Vec3i)backc)
                        * (1 - (double)pixiel_alpha / 255) * mix_alpha);
        }
    return image_mix;
}

}  //namespace portrait
