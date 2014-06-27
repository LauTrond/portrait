//sybie/common/Graphics/Math.hh

#ifndef INCLUDE_SYBIE_COMMON_MATH_HH
#define INCLUDE_SYBIE_COMMON_MATH_HH

#include <cstdint>
#include <algorithm>

namespace sybie {
namespace common {

template<class FLOAT_TYPE, class INT_TYPE>
inline INT_TYPE Round(FLOAT_TYPE val)
{
    return (INT_TYPE)(val + (FLOAT_TYPE)0.5);
}

template<class FLOAT_TYPE>
inline int RoundToInt(FLOAT_TYPE val)
{
    return Round<int, FLOAT_TYPE>(val);
}

template<class FLOAT_TYPE>
inline int16_t RoundToInt16(FLOAT_TYPE val)
{
    return Round<int16_t, FLOAT_TYPE>(val);
}

template<class FLOAT_TYPE>
inline int32_t RoundToInt32(FLOAT_TYPE val)
{
    return Round<int32_t, FLOAT_TYPE>(val);
}

template<class FLOAT_TYPE>
inline int64_t RoundToInt64(FLOAT_TYPE val)
{
    return Round<int64_t, FLOAT_TYPE>(val);
}

template<class T>
inline void SetMax(T& var, const T& compare_to)
{
    var = std::max<T>(var, compare_to);
}

template<class T>
inline void SetMin(T& var, const T& compare_to)
{
    var = std::min<T>(var, compare_to);
}

}  //common
}  //sybie

#endif //#ifndef INCLUDE_SYBIE_COMMON_MATH_HH
