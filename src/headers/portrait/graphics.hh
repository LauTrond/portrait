//portrait/graphics.hh
//包含本项目所需的基本图形运算

#ifndef INCLUDE_PORTRAIT_GRAPHICS_HH
#define INCLUDE_PORTRAIT_GRAPHICS_HH

#include <cstdint>

#include "opencv2/opencv.hpp"
#include "sybie/common/Graphics/Structs.hh"

namespace portrait {

//返回一个矩形的中心点坐标。
cv::Point CenterOf(const cv::Rect& rect);
// 返回一个x=0、y=0，width和height等于image尺寸的矩形。
cv::Rect WholeArea(const cv::Mat& image);
//四个角的像素坐标
cv::Point TopLeft(const cv::Mat& image);
cv::Point TopRight(const cv::Mat& image);
cv::Point BottomLeft(const cv::Mat& image);
cv::Point BottomRight(const cv::Mat& image);
cv::Point TopLeft(const cv::Rect& rect);
cv::Point TopRight(const cv::Rect& rect);
cv::Point BottomLeft(const cv::Rect& rect);
cv::Point BottomRight(const cv::Rect& rect);

// 检查rect_outter是否完全包含rect_inner
bool Inside(const cv::Rect& rect_inner, const cv::Rect& rect_outter);
// 检查rect_inner是否完全在image范围内
bool Inside(const cv::Rect& rect_inner, const cv::Mat& image);
// 获取rect1和rect2重叠部分
cv::Rect OverlapArea(const cv::Rect& rect1, const cv::Rect& rect2);
// 计算rect1相对offset的坐标。
// 返回结果的width和height与rect1相同，x和y分别减去offset的x和y。
cv::Rect SubArea(const cv::Rect& rect1, const cv::Point& offset);
// 计算Rect(rect1.point - rect2.point, rect1.size)
// 返回结果的width和height与rect1相同，x和y分别减去rect2的x和y。
cv::Rect SubArea(const cv::Rect& rect1, const cv::Rect& rect2);

//判断 OpenCV GrabCut 掩码是否前景
inline bool IsFront(uint8_t val)
{
    return val == cv::GC_FGD || val == cv::GC_PR_FGD;
}

//判断 OpenCV GrabCut 掩码是否背景
inline bool IsBack(uint8_t val)
{
    return val == cv::GC_BGD || val == cv::GC_PR_BGD;
}

}  //namespace portrait

#endif
