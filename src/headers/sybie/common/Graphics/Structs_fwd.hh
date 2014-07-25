//sybie/common/Graphics/Structs_fwd.hh

#ifndef INCLUDE_SYBIE_COMMON_GRAPHICS_STRUCTS_FWD_HH
#define INCLUDE_SYBIE_COMMON_GRAPHICS_STRUCTS_FWD_HH

#include <cstdint>
#include <memory>

namespace sybie {
namespace common {
namespace Graphics {

template<class T> struct PointBase;
typedef PointBase<int32_t> Point;
typedef PointBase<int64_t> Point64;
typedef PointBase<float> PointF;
typedef PointBase<double> PointD;

template<class T> struct SizeBase;
typedef SizeBase<int32_t> Size;
typedef SizeBase<int64_t> Size64;
typedef SizeBase<float> SizeF;
typedef SizeBase<double> SizeD;

template<class T> struct RectBase;
typedef RectBase<int32_t> Rect;
typedef RectBase<int64_t> Rect64;
typedef RectBase<float> RectF;
typedef RectBase<double> RectD;

template<class T> struct ArgbBase;
typedef ArgbBase<uint8_t> Argb8888;

template<class T> struct AyuvBase;
typedef AyuvBase<uint8_t> Ayuv8888;

template<class T> struct MatBase;
typedef MatBase<uint8_t> GrayBitmap;
typedef MatBase<Argb8888> ArgbBitmap;
typedef MatBase<Ayuv8888> AyuvBitmap;

template<class IntType> struct ColumnPriorIncreaserBase;
template<class IntType> struct RowPriorIncreaserBase;
template<class IntType, class Increaser> struct RectInteratorBase;
template<class IntType, class Increaser> class PointsInRectBase;

}  //namespace Graphics
}  //namespace common
}  //namespace sybie

#endif //ifndef INCLUDE_SYBIE_COMMON_GRAPHICS_STRUCTS_FWD_HH
