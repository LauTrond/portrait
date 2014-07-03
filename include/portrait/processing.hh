//portrait/processing.hh

#ifndef INCLUDE_PORTRAIT_PROCESSING_HH
#define INCLUDE_PORTRAIT_PROCESSING_HH

#include "opencv2/opencv.hpp"

namespace portrait {

//抠图、替换背景。
//如果需要一次抠图多次替换背景，使用PortraitProcessSemi和PortraitMix。
cv::Mat PortraitProcessAll(
    cv::Mat&& photo, //CV_8UC3
    const int face_resize_to = 200,
    const cv::Size& portrait_size = cv::Size(300,400),
    const int VerticalOffset = 0,
    const cv::Vec3b& back_color = cv::Vec3b(240,240,240));

struct SemiDataImpl;

//代表图片处理的中间数据
struct SemiData
{
public:
    SemiData() throw(); //产生一个空的、无效的实例，可被赋值。
    SemiData(const SemiData&) = delete; //无法复制
    SemiData(SemiData&& another) throw();
    ~SemiData() throw();
    SemiData& operator=(const SemiData&) = delete; //无法复制
    SemiData& operator=(SemiData&& another) throw();
    void Swap(SemiData& another) throw();
public:
    //获取替换背景前的图片，已被缩放到目标分辨率。
    //返回的Mat是副本，可被修改而不影响SemiData内部行为。
    cv::Mat GetImage() const;
    //同GetImage，增加一些用于检查抠图范围的辅助线
    cv::Mat GetImageWithLines() const;
private:
    friend struct SemiDataImpl;
    explicit SemiData(SemiDataImpl* data) throw();
    SemiDataImpl* _data;
};

/* PortraitProcessSemi和PortraitMix把整个处理过程分为两个阶段，
 * PortraitProcessSemi主要执行抠图，PortraitMix可以对抠图结果混合背景，
 * 因此可以单次抠图、多次混合。
 */

//对photo执行人和背景分离处理。
//photo的类型是CV_8UC3 BGR
//face_resize_to指定图片被缩放后人脸的大小。
//返回抠图结果（中间数据）
SemiData PortraitProcessSemi(
    cv::Mat&& photo,
    const int face_resize_to);

//背景替换。
//semi是抠图结果。
//crop_size指定裁剪的尺寸，如果越界，会自动补边
//VerticalOffset指定垂直方向的偏移量。
cv::Mat PortraitMix(
    SemiData& semi,
    const cv::Size& crop_size,
    const int VerticalOffset,
    const cv::Vec3b& back_color);

}

#endif
