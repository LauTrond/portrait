#include "portrait/algorithm.hh"

#include <algorithm>
#include <cmath>

#include "sybie/common/RichAssert.hh"
#include "sybie/common/Time.hh"

namespace portrait {

const int
    BorderSize = 3,
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

//背景
const double
    BGWidth = 0.20,
    BGTop = 0.50,
    BGBottom = 0.00;
//绝对前景－脸
const double
    FGFaceTop = 0.05,
    FGFaceSide = -0.25,
    FGFaceBottom = -0.30;
//绝对前景-颈
const double
    FGNeckSide = -0.35;
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

int ModulusOf(const cv::Vec3i& vec)
{
    return (int)sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
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
    const cv::Scalar& back_color,
    int thickness )
{
    //几条分割线
    int bg_up = face_area.y - face_area.height * BGTop;
    int bg_down = face_area.y + face_area.height * (1 + BGBottom);
    int bg_left = face_area.x - face_area.width * BGWidth;
    int bg_right = face_area.x + face_area.width * (1 + BGWidth);
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
    const cv::Rect& face_area)
{
    sybie::common::StatingTestTimer timer("GetFrontBackMask");
    sybie_assert(Inside(face_area, image))
        << SHOW(face_area)
        << SHOW(image.rows)
        << SHOW(image.cols);

    //初始化前景/背景掩码
    cv::Mat mask(image.rows, image.cols, CV_8UC1);
    DrawMask(mask, face_area, true, cv::GC_PR_FGD,
             cv::GC_FGD, cv::GC_BGD, CV_FILLED);
    //抠图
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
                        int dist_back = cnt_front * ModulusOf(sum_back - cur * cnt_back);
                        int dist_front = cnt_back * ModulusOf(sum_front - cur * cnt_front);
                        if (dist_front + dist_back > 0)
                            alpha = 255 * dist_back / (dist_front + dist_back);
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

void DrawGrabCutLines(
    cv::Mat& image,
    const cv::Rect& face_area)
{
    DrawMask(image, face_area,
             false, cv::Scalar(0,0,0),
             cv::Scalar(255,0,0), cv::Scalar(0,0,255),
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
    const cv::Vec3b& back_color)
{
    cv::Mat image_mix(image.rows, image.cols, CV_8UC3);
    for (int r = 0 ; r < image.rows ; r++)
        for (int c = 0 ; c < image.cols ; c++)
        {
            const cv::Vec4b& p = raw.at<cv::Vec4b>(r,c);
            const uint8_t& alpha = p[3];
            const cv::Vec3b& backc = *(const cv::Vec3b*)(const void*)&p;

            const cv::Vec3b& src = image.at<cv::Vec3b>(r,c);
            cv::Vec3b& mix = image_mix.at<cv::Vec3b>(r,c);
            mix = (cv::Vec3i)src + (cv::Vec3i)((cv::Vec3i)back_color - (cv::Vec3i)backc)
                        * (1 - (double)alpha / 255);
        }
    return image_mix;
}


}  //namespace portrait
