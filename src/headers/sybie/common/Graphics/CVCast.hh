//sybie/common/Graphics/CVCast.hpp
//Structs.hh定义类型和opencv定义类型的转换

#ifndef INCLUDE_SYBIE_COMMON_GRAPHICS_CVCAST_HH
#define INCLUDE_SYBIE_COMMON_GRAPHICS_CVCAST_HH

#include <cassert>

#include "opencv2/opencv.hpp"
#include "sybie/common/Graphics/Structs.hh"

namespace sybie {
namespace common {
namespace Graphics {

template<class T>
cv::Point_<T> ToCvType(const PointBase<T>& point)
{
    return cv::Point_<T>(point.x, point.y);
}

template<class T>
PointBase<T> ToComType(const cv::Point_<T>& point)
{
    return PointBase<T>(point.x, point.y);
}

template<class T>
cv::Size_<T> ToCvType(const SizeBase<T>& size)
{
    return cv::Size_<T>(size.width, size.height);
}

template<class T>
SizeBase<T> ToComType(const cv::Size_<T>& size)
{
    return SizeBase<T>(size.width, size.height);
}

template<class T>
cv::Rect_<T> ToCvType(const RectBase<T>& rect)
{
    return cv::Rect_<T>(rect.Left(), rect.Top(),
                        rect.Width(), rect.Height());
}

template<class T>
RectBase<T> ToComType(const cv::Rect_<T>& rect)
{
    return RectBase<T>(PointBase<T>(rect.x, rect.y),
                       SizeBase<T>(rect.width, rect.height));
}

template<class T>
MatBase<T> MakeWrapper(cv::Mat& mat)
{
    assert(mat.dims == 2);
    assert(mat.step == mat.cols * mat.elemSize());
    assert(sizeof(T) == mat.elemSize());
    return MatBase<T>(Size(mat.cols, mat.rows), (T*)(void*)mat.data);
}

template<class T>
const MatBase<T> MakeConstWrapper(const cv::Mat& mat)
{
    return MakeWrapper<T>(const_cast<cv::Mat&>(mat));
}

}  //namespace Graphics
}  //namespace common
}  //namespace sybie

#endif //ifndef
