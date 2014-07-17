//sybie/common/Graphics/Structs.hpp

#ifndef INCLUDE_SYBIE_COMMON_GRAPHICS_STRUCTS_HH
#define INCLUDE_SYBIE_COMMON_GRAPHICS_STRUCTS_HH

#include "Structs_fwd.hh"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace sybie {
namespace common {
namespace Graphics {

inline std::istream& operator>>(std::istream& is, const char& ch_expected)
{
    char ch;
    do { is>>ch; } while(ch != ch_expected);
    return is;
}

template<class T>
struct PointBase
{
public:
    T x,y;

    PointBase() { }

    PointBase(const T& x, const T& y)
        : x(x), y(y) { }

    template<class T2>
    PointBase(const PointBase<T2>& other)
        : x(other.x), y(other.y) { }

    template<class T2>
    explicit PointBase(const SizeBase<T2>& size)
        : x(size.width), y(size.height) { }

    bool operator==(const PointBase& another) const
    {
        return x == another.x && y == another.y;
    }

    bool operator!=(const PointBase& another) const
    {
        return !operator==(another);
    }

    inline PointBase& operator=(const PointBase& another)
    {
        x = another.x;
        y = another.y;
        return *this;
    }
    inline PointBase& operator+=(const PointBase& another)
    {
        x += another.x;
        y += another.y;
        return *this;
    }
    inline PointBase& operator-=(const PointBase& another)
    {
        x += another.x;
        y += another.y;
        return *this;
    }
    inline PointBase& operator*=(const T& val)
    {
        x *= val;
        y *= val;
        return *this;
    }
    inline PointBase& operator/=(const T& val)
    {
        x /= val;
        y /= val;
        return *this;
    }
    inline PointBase operator+(const PointBase& another) const
    {
        return PointBase(x + another.x, y + another.y);
    }
    inline PointBase operator-(const PointBase& another) const
    {
        return PointBase(x - another.x, y - another.y);
    }
    inline PointBase operator*(const T& val) const
    {
        return PointBase(x * val, y * val);
    }
    inline PointBase operator/(const T& val) const
    {
        return PointBase(x / val, y / val);
    }
    inline PointBase operator-() const
    {
        return PointBase(-x, -y);
    }
}; //template<class T> struct PointBase

template<class T>
std::ostream& operator<<(std::ostream& os, const PointBase<T>& val)
{
    os<<'('<<val.x<<','<<val.y<<')';
    return os;
}

template<class T>
std::istream& operator>>(std::istream& is, PointBase<T>& val)
{
    return is>>'('>>val.x>>','>>val.y>>')';
}

template<class T>
struct SizeBase
{
public:
    T width, height;

    SizeBase() { }

    SizeBase(const T& width, const T& height)
        : width(width), height(height) { }

    template<class T2>
    SizeBase(const SizeBase<T2>& other)
        : width(other.width), height(other.height) { }

    template<class T2>
    explicit SizeBase(const PointBase<T2>& point)
        : width(point.x), height(point.y) { }

    bool operator==(const SizeBase& another) const
    {
        return width == another.width && height == another.height;
    }

    bool operator!=(const SizeBase& another) const
    {
        return !operator==(another);
    }
    inline SizeBase& operator+=(const SizeBase& another)
    {
        width += another.width;
        height += another.height;
        return *this;
    }
    inline SizeBase& operator-=(const SizeBase& another)
    {
        width += another.width;
        height += another.height;
        return *this;
    }
    inline SizeBase& operator*=(const T& val)
    {
        width *= val;
        height *= val;
        return *this;
    }
    inline SizeBase& operator/=(const T& val)
    {
        width /= val;
        height /= val;
        return *this;
    }
    inline SizeBase operator+(const SizeBase& another) const
    {
        return SizeBase(width + another.width, height + another.height);
    }
    inline SizeBase operator-(const SizeBase& another) const
    {
        return SizeBase(width - another.width, height - another.height);
    }
    inline SizeBase operator*(const T& val) const
    {
        return SizeBase(width * val, height * val);
    }
    inline SizeBase operator/(const T& val) const
    {
        return SizeBase(width / val, height / val);
    }
    inline SizeBase operator-() const
    {
        return SizeBase(-width, -height);
    }

    inline T Total() const {return width * height;}
}; //template<class T> struct SizeBase

template<class T>
std::ostream& operator<<(std::ostream& os, const SizeBase<T>& val)
{
    return os<<'('<<val.width<<','<<val.height<<')';
}

template<class T>
std::istream& operator>>(std::istream& is, SizeBase<T>& val)
{
    return is>>'('>>val.width>>','>>val.height>>')';
}

template<class T>
struct RectBase
{
public:
    PointBase<T> point;
    SizeBase<T> size;

public:
    RectBase() { }
    RectBase(const PointBase<T>& point, const SizeBase<T>& size)
        : point(point), size(size) { }
    template<class T2> RectBase(const RectBase<T2>& other)
        : point(other.point), size(other.size) { }

    bool operator==(const RectBase& another) const
    {
        return point == another.point && size == another.size;
    }

    bool operator!=(const RectBase& another) const
    {
        return !operator==(another);
    }

    bool Inside(const RectBase& another) const
    {
        return Left()   >= another.Left()
            && Top()    >= another.Top()
            && Right()  <= another.Right()
            && Bottom() <= another.Bottom();
    }

    inline T Left() const { return point.x; }
    inline T Top() const { return point.y; }
    inline T Right() const { return point.x + size.width; }
    inline T Bottom() const { return point.y + size.height; }
    inline T Height() const { return size.height; }
    inline T Width() const { return size.width; }

    inline PointBase<T> LeftTop() const { return point; }
    inline PointBase<T> RightBottom() const { return point + PointBase<T>(size); }

    inline void SetLeft(const T& val) { size.width += point.x - val; point.x = val; }
    inline void SetTop(const T& val) { size.height += point.y - val; point.y = val; }
    inline void SetRight(const T& val) { size.width = val - point.x; }
    inline void SetBottom(const T& val) { size.height = val - point.y; }

    inline PointBase<T> Center() const
        { return Point(point.x + size.width/2, point.y + size.height/2); }
public:
    static RectBase FromCenterSize(const PointBase<T>& center, const SizeBase<T>& size)
    {
        return RectBase(center - PointBase<T>(size / 2), size);
    }

    static RectBase FromLeftTopAndRightBottom(
        const PointBase<T>& left_top, const PointBase<T>& right_bottom)
    {
        return RectBase(left_top, SizeBase<T>(right_bottom - left_top));
    }
}; //template<class T> struct RectBase

template<class T>
std::ostream& operator<<(std::ostream& os, const RectBase<T>& val)
{
    return os<<'('<<val.point<<','<<val.size<<')';
}

template<class T>
std::istream& operator>>(std::istream& is, RectBase<T>& val)
{
    return is>>'('>>val.point>>','>>val.size>>')';
}

template<class T>
struct ArgbBase
{
    T r,g,b,a;

    ArgbBase()
    { }

    template<class T2>
    ArgbBase(const ArgbBase<T2>& another) { operator=<T2>(another); }

    template<class T2>
    ArgbBase& operator=(const ArgbBase<T2>& another)
    {
        r = another.r;
        g = another.g;
        b = another.b;
        a = another.a;
        return *this;
    }
};

template<class T>
struct AyuvBase
{
    T y,u,v,a;

    AyuvBase()
    { }

    template<class T2>
    AyuvBase(const AyuvBase<T2>& another) { operator=<T2>(another); }

    template<class T2>
    AyuvBase& operator=(const AyuvBase<T2>& another)
    {
        y = another.y;
        u = another.u;
        v = another.v;
        a = another.a;
        return *this;
    }
};

template<class T>
struct MatBase
{
public:
    //初始化一个无效的实例
    MatBase() throw()
        : _size(0, 0), _own_data(false), _data(nullptr)
    { }

    //用指定大小初始化，分配内存空间
    MatBase(Size size)
        : _size(size), _own_data(true), _data(new T[std::max(0, size.Total())])
    {
        assert(size.width > 0);
        assert(size.height > 0);
    }

    //用指定大小和数据初始化MatBase，当Matbase析构时不会释放data的空间
    //_data指向的空间不小于size.Total() * sizeof(T)
    MatBase(Size size, T* data) throw()
        : _size(size), _own_data(false), _data(data)
    { }

    //复制构造，对another中矩阵克隆一份。
    MatBase(const MatBase& another)
        : _size(another._size), _own_data(true), _data(new T[_size.Total()])
    {
        memcpy(_data, another._data, _size.Total() * sizeof(T));
    }

    //移动构造
    MatBase(MatBase&& another) throw()
        : _size(0, 0), _own_data(false), _data(nullptr)
    {
        Swap(another);
    }

    ~MatBase() throw()
    {
        if (_own_data)
            delete[] _data;
    }

    void Swap(MatBase& another) throw()
    {
        std::swap(_size, another._size);
        std::swap(_own_data, another._own_data);
        std::swap(_data, another._data);
    }

    bool IsValid() const throw()
    {
        return _data != nullptr;
    }

    //复制赋值
    inline MatBase& operator=(const MatBase& another)
    {
        MatBase newmat(another);
        Swap(newmat);
        return *this;
    }

    //移动赋值
    inline MatBase& operator=(MatBase&& another) throw()
    {
        Swap(another);
        return *this;
    }

    inline void Set(const T& val)
    {
        for (int i = 0 ; i < _size.Total() ; i++)
            _data[i] = val;
    }

    //获取大小
    inline Size GetSize() const
    {
        assert(_data != nullptr);
        return _size;
    }

    //获取指定位置元素的指针
    inline T* Get(const Point& p)
    {
        assert(_data != nullptr);
        return _data + _Pos(p);
    }

    //获取指定行首元素的指针
    inline T* Get(int y)
    {
        return Get(Point(0, y));
    }

    //获取首行首元素的指针
    inline T* Get()
    {
        return Get(Point(0, 0));
    }

    //获取指定行首元素的指针
    inline T* operator[](int y)
    {
        return Get(y);
    }

    inline T& at(int x, int y)
    {
        return at(Point(x, y));
    }

    inline T& at(const Point& p)
    {
        return *Get(p);
    }

    inline T* begin()
    {
        return _data;
    }

    inline T* end()
    {
        return _data + _size.Total();
    }

    //获取指定位置元素的指针
    inline const T* Get(const Point& p) const
    {
        assert(_data != nullptr);
        return _data + _Pos(p);
    }

    //获取指定行首元素的指针
    inline const T* Get(int y) const
    {
        return Get(Point(0, y));
    }

    //获取首行首元素的指针
    inline const T* Get() const
    {
        return Get(Point(0, 0));
    }

    //获取指定行首元素的指针
    inline const T* operator[](int y) const
    {
        return Get(y);
    }

    inline const T& at(int x, int y) const
    {
        return at(Point(x, y));
    }

    inline const T& at(const Point& p) const
    {
        return *Get(p);
    }

    inline const T* begin() const
    {
        return _data;
    }

    inline const T* end() const
    {
        return _data + _size.Total();
    }

    inline Rect WholeArea() const
    {
        return Rect(Point(0,0), _size);
    }

private:
    Size _size;
    bool _own_data;
    T* _data;

    inline int _Pos(const Point& point) const
    {
        assert(point.x >= 0);
        assert(point.y >= 0);
        assert(point.x < _size.width);
        assert(point.y < _size.height);
        return point.y * _size.width + point.x;
    }
}; //template<class T> struct MatBase

template<class T>
RectBase<T> OverlapArea(const RectBase<T>& rect1, const RectBase<T>& rect2)
{
    T overlap_top    = std::max(rect1.Top(),    rect2.Top()   );
    T overlap_bottom = std::min(rect1.Bottom(), rect2.Bottom());
    T overlap_left   = std::max(rect1.Left(),   rect2.Left()  );
    T overlap_right  = std::min(rect1.Right(),  rect2.Right() );
    if (overlap_top >= overlap_bottom || overlap_left >= overlap_right)
        return RectBase<T>(
            rect1.point, SizeBase<T>(0,0));
    else
        return RectBase<T>(
            PointBase<T>(overlap_left, overlap_top),
            SizeBase<T>(overlap_right - overlap_left, overlap_bottom - overlap_top));
}

//计算两个矩形重叠面积，没有重叠则返回0
template<class T>
T OverlapTotal(const RectBase<T>& rect1, const RectBase<T>& rect2)
{
    return OverlapArea(rect1, rect2).size.Total();
}

}  //Graphics
}  //namespace common
}  //namespace sybie

#endif //ifndef INCLUDE_SYBIE_COMMON_GRAPHICS_STRUCTS_HH
