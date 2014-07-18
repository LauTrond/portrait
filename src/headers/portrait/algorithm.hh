//portrait/algorithm.hh
//包含本项目所需的算法实现

#ifndef INCLUDE_PORTRAIT_ALGORITHM_HH
#define INCLUDE_PORTRAIT_ALGORITHM_HH

#include "opencv2/opencv.hpp"

namespace portrait {

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

/* 根据图像（image）和其中的人脸位置（face_area）抠出人像，
 * 返回一个与image同尺寸的矩阵，类型时CV_8UC4，
 * 前三通道的格式与image相同，表示image中每个像素的背景色（可能是近似）
 * 第四通道为Alpha，表示前景的混合比例。
 * 对于Alpha为255的点（全前景），前3通道无意义。
 */
cv::Mat GetMixRaw(
    const cv::Mat& image,
    const cv::Rect& face_area,
    const cv::Mat& stroke);

/* 画出一些用于调试的辅助线，展示绝对前景、绝对背景等区域。
 */
void DrawGrabCutLines(
    cv::Mat& image,
    const cv::Rect& face_area);

/* 如果area超出image范围，则扩展image以包含area
 * 创建新的内存空间，并直接修改image。
 * 扩展区域使用border_pixel填充。
 * 返回area在新图像中的新区域。
 */
cv::Rect Extend(
    cv::Mat& image,
    const cv::Rect& area,
    const cv::Scalar& border_pixel);

/* 使用GetFrontBackMask返回的抠像结果，替换image中的背景。
 * raw：必须是GetFrontBackMask对image执行的返回结果。
 * back_color：背景色
 * mix_alpha：混合比例，1表示完全替换背景，0表示完全不替换，
 *           1和0之间表示按一定的比例替换
 * 返回一个新的图像，类型是CV_8UC3，大小和image一致。
 */
cv::Mat Mix(
    const cv::Mat& image,
    const cv::Mat& raw,
    const cv::Vec3b& back_color,
    const double mix_alpha);

}  //namespace portrait

#endif
