#include "portrait/graphics.hh"

namespace portrait {

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

}  //namespace portrait
