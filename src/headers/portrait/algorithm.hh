//portrait/algorithm.hh
//包含本项目所需的算法实现

#ifndef INCLUDE_PORTRAIT_ALGORITHM_HH
#define INCLUDE_PORTRAIT_ALGORITHM_HH

#include "opencv2/opencv.hpp"

namespace portrait {

//返回一个矩形的中心点坐标。
cv::Point CenterOf(const cv::Rect& rect);
// 返回一个x=0、y=0，width和height等于image尺寸的矩形。
cv::Rect WholeArea(const cv::Mat& image);
// 检查rect_outter是否完全包含rect_inner
bool Inside(const cv::Rect& rect_inner, const cv::Rect& rect_outter);
// 检查rect_inner是否完全在image范围内
bool Inside(const cv::Rect& rect_inner, const cv::Mat& image);
// 获取rect1和rect2重叠部分
cv::Rect OverlapArea(const cv::Rect& rect1, const cv::Rect& rect2);

/* 给出图像（image）和其中人脸的位置（face_area），
 * 按max_up_expand、max_down_expand、max_width_expand指定的上下左右范围，
 * 切割出一个区域。
 * 如果max_up_expand、max_down_expand、max_width_expand无法满足，
 * 则按最大可切割的范围。
 * 结果直接修改image，并返回人脸的新位置。
 */
cv::Rect TryCutPortrait(
    cv::Mat& image,
    const cv::Rect& face_area,
    const double max_up_expand,
    const double max_down_expand,
    const double max_width_expand);

/* 给出图像（image）和其中人脸的位置（face_area），
 * 改变image的大小，使人脸的大小等于指定的大小（face_resize_to），
 * 结果直接修改在image上，并返回新的人脸位置
 */
cv::Rect ResizeFace(
    cv::Mat& image,
    const cv::Rect& face_area,
    const cv::Size& face_resize_to);

/* 根据图像（image）和其中的人脸位置抠出人像，
 * 返回一个于image同尺寸的矩阵，类型时CV_8UC4，
 * 前三通道的格式与image相同，表示image中每个像素的背景色（可能是近似）
 * 第四通道为Alpha，表示前景的混合比例。
 * 对于Alpha为255的点（全前景），前3通道无意义。
 */
cv::Mat GetFrontBackMask(
    const cv::Mat& image,
    const cv::Rect& face_area);

/* 使用GetFrontBackMask返回的抠像结果，替换image中的背景。
 * raw必须是GetFrontBackMask对image执行的返回结果。
 * 返回一个新的图像，类型是CV_8UC3，大小和image一致。
 */
cv::Mat Mix(
    const cv::Mat& image,
    const cv::Mat& raw,
    const cv::Vec3b& back_color);

}  //namespace portrait

#endif
