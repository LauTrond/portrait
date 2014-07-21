//portrait/math.hh
//包含本项目所需的简单数学运算

#ifndef INCLUDE_PORTRAIT_MATH_HH
#define INCLUDE_PORTRAIT_MATH_HH

#include <cmath>
#include <cassert>
#include <algorithm>
#include <iterator>

#include "opencv2/opencv.hpp"
#include "sybie/common/RichAssert.hh"

namespace portrait {

//平方
template<class T>
inline T Squeue(T a)
{
    return a * a;
}

//计算向量内积
template<class T, int n>
inline T DotProduct(const cv::Vec<T,n>& vec1, const cv::Vec<T,n>& vec2)
{
    T sum = 0;
    for (int i = 0 ; i < n ; i++)
        sum += vec1[i] * vec2[i];
    return sum;
}

// 计算vec自身的内积
template<class T, int n>
inline T SqueueVec(const cv::Vec<T,n>& vec)
{
    return DotProduct<T,n>(vec, vec);
}

// 向量模
template<class T, int n>
inline double ModulusOf(const cv::Vec<T,n>& vec)
{
    return sqrt(SqueueVec<T,n>(vec));
}

//整数转8位无符号整数，小于0返回0，大于255返回255。
template<class T>
inline uint8_t TruncByte(T val)
{
    return (uint8_t)std::max<T>(0, std::min<T>(255, val));
}

//整数向量转8位无符号整数向量，每一纬小于0返回0、大于255返回255。
template<class T, int n>
inline cv::Vec<uint8_t, n> TruncIntVec(const cv::Vec<T, n>& val)
{
    cv::Vec<uint8_t, n> result;
    for (int i = 0 ; i < n ; i++)
        result[i] = TruncByte<T>(val[i]);
    return result;
}

//浮点向量转8位无符号整数向量，浮点向量每一纬度是0和1之间。
template<class T, int n>
inline cv::Vec<uint8_t, n> TruncFloatVec(const cv::Vec<T, n>& val)
{
    cv::Vec<uint8_t, n> result;
    for (int i = 0 ; i < n ; i++)
        result[i] = TruncByte<T>(val[i] * 255);
    return result;
}

template<class T, int n>
struct DistanceOfVector
{
    double operator()(const cv::Vec<T,n>& vec1, const cv::Vec<T,n>& vec2)
    {
        return ModulusOf<T,n>(vec1 - vec2);
    }
};

template<class T>
struct Zero
{
    T operator()()
    {
        return 0;
    }
};

//在向量上偏特化Zero
template<class TElement, int n>
struct Zero<cv::Vec<TElement,n>>
{
    cv::Vec<TElement,n> operator()()
    {
        cv::Vec<TElement,n> result;
        for (int i = 0 ; i < n ; i++)
            result[i] = 0;
        return result;
    }
};

//统计集合数学期望
template<class Tval, class TOrigin = Zero<Tval>>
class Mean
{
public:
    Mean() : _sum(TOrigin()()), _sum_weight(0) { }

    inline void Push(const Tval& val, float weight = 1)
    {
        _sum += val;
        _sum_weight += weight;
    }

    Tval Get() const
    {
        if (_sum_weight == 0)
            return TOrigin()();
        else
            return Tval(_sum / _sum_weight);
    }

    float Count() const
    {
        return _sum_weight;
    }

    Tval Sum() const
    {
        return _sum;
    }
private:
    Tval _sum;
    float _sum_weight;
}; //template<class T> class Mean

template<class TVal,
         class TDist,
         class TMean=Mean<TVal> >
class KMeans
{
public:
    explicit KMeans(int k)
        : _k(k), _center(new TVal[k]), _cnt(new int[k])
    { }

    ~KMeans()
    {
        delete[] _center;
        delete[] _cnt;
    }

    void InitCenter(int tag, const TVal& val)
    {
        _center[tag] = val;
    }

    template<class Iterator>
    void Train(Iterator samples_begin, Iterator samples_end,
               int max_iteration = 10)
    {
        bool finished;
        do
        {
            std::unique_ptr<TMean[]> means(new TMean[_k]);
            for (Iterator it = samples_begin;
                 it != samples_end;
                 it++)
            {
                int tag = GetTag(*it, true);
                means[tag].Push(*it);
            }

            finished = true;
            for (int i = 0 ; i < _k ; i++)
            {
                TVal mean = (means[i].Count() > 0) ?
                            means[i].Get() :
                            *samples_begin;
                if (mean != _center[i])
                {
                    _center[i] = mean;
                    finished = false;
                }
                _cnt[i] = means[i].Count();
            }
        }
        while (!finished && (--max_iteration > 0));
    }

    int GetTag(const TVal& val, bool get_empty = false) const
    {
        int tag = -1;
        double min_dist = std::numeric_limits<double>::max();
        TDist Distance;
        for (int i = 0 ; i < _k ; i++)
        {
            if (_cnt[i] == 0 && !get_empty)
                continue;

            double dist = Distance(_center[i], val);
            if (dist < min_dist)
            {
                min_dist = dist;
                tag = i;
            }
        }
        sybie_assert(tag >= 0)<<SHOW(val);
        return tag;
    }

    inline double DistanceOf(const TVal& val, int tag) const
    {
        return TDist()(_center[tag], val);
    }

    inline int Count(int tag) const
    {
        return _cnt[tag];
    }

    inline TVal GetCenter(int tag) const
    {
        return _center[tag];
    }
private:
    int _k;
    TVal* _center;
    int* _cnt;
}; //template<...> class KMeans

}  //namespace portrait

#endif //ifndef
