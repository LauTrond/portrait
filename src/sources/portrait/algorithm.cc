#include "portrait/algorithm.hh"

#include <algorithm>
#include <cmath>

#include "sybie/common/RichAssert.hh"

namespace portrait {

const int
    BorderSize = 3,
    MixSize = BorderSize * 2 + 1;

const int GrabCutInteration = 3;

const double
    BGWidth = 0.1,
    BGTop = 0.4,
    BGBottom = 0.1;

cv::Point CenterOf(const cv::Rect& rect)
{
    return cv::Point(rect.x + rect.width / 2,
                     rect.y + rect.height / 2);
}

cv::Rect WholeArea(const cv::Mat& image)
{
    return cv::Rect(0, 0, image.cols, image.rows);
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

cv::Mat GetFrontBackMask(
    const cv::Mat& image,
    const cv::Rect& face_area)
{
    sybie_assert(Inside(face_area, image))
        << SHOW(face_area)
        << SHOW(image.rows)
        << SHOW(image.cols);

    //几条分割线
    int up = face_area.y - face_area.height * BGTop;
    int down = face_area.y + face_area.height * (1 + BGBottom);
    int left = face_area.x - face_area.width * BGWidth;
    int right = face_area.x + face_area.width * (1 + BGBottom);

    //cv::grabCut的初始掩码
    cv::Mat mask_grab(image.rows, image.cols, CV_8UC1);
    for (int r = 0 ; r < mask_grab.rows ; r++)
        for (int c = 0 ; c < mask_grab.cols ; c++)
        {
            uint8_t& m = mask_grab.at<uint8_t>(r,c);
            if (((c < left || c >= right) && r < down) || r < up)
                m = cv::GC_BGD;
            else
                m = cv::GC_PR_FGD;
        }

    //抠图
    cv::Mat bgModel,fgModel; //临时空间
    cv::grabCut(image, mask_grab, cv::Rect(),
                bgModel,fgModel,
                GrabCutInteration, cv::GC_INIT_WITH_MASK);

    //找出边缘像素
    cv::Mat mask_border(image.rows, image.cols, CV_8UC1);
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
                    uint8_t& m = mask_grab.at<uint8_t>(rr,cc);
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

    //计算背景色和Alpha
    cv::Mat result(image.rows, image.cols, CV_8UC4);
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

    return result;
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
            mix = src;

            if (alpha < 255) //替换背景
                mix += ((cv::Vec3i)back_color - (cv::Vec3i)backc)
                    * (1 - (double)alpha / 255);
        }
    return image_mix;
}


}  //namespace portrait
