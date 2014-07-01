//portrait/processing.hh

#ifndef INCLUDE_PORTRAIT_PROCESSING_HH
#define INCLUDE_PORTRAIT_PROCESSING_HH

#include "opencv2/opencv.hpp"

namespace portrait {

cv::Mat PortraitProcessAll(
    cv::Mat&& photo, //CV_8UC3
    const int face_resize_to = 200,
    const cv::Size& portrait_size = cv::Size(300,400),
    const int VerticalOffset = 0,
    const cv::Vec3b& back_color = {240,240,240});

struct SemiData
{
public:
    SemiData() throw();
    explicit SemiData(void* data) throw();
    SemiData(const SemiData&) = delete;
    SemiData(SemiData&& another) throw();
    ~SemiData() throw();
    SemiData& operator=(const SemiData&) = delete;
    SemiData& operator=(SemiData&& another) throw();
    void Swap(SemiData& another) throw();
public:
    void* Get() const throw();
    cv::Mat GetImage() const;
    cv::Mat GetImageWithLines() const;
private:
    void* _data;
};

SemiData PortraitProcessSemi(
    cv::Mat&& photo, //CV_8UC3 BGR
    const int face_resize_to);

cv::Mat PortraitMix(
    const SemiData& semi,
    const cv::Size& portrait_size,
    const int VerticalOffset,
    const cv::Vec3b& back_color);

}

#endif
