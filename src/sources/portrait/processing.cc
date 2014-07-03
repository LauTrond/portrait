#include "portrait/processing.hh"

#include <utility>
#include <cassert>

#include "portrait/exception.hh"
#include "portrait/algorithm.hh"
#include "portrait/facedetect.hh"

namespace portrait {

cv::Mat PortraitProcessAll(
    cv::Mat&& photo,
    const int face_resize_to,
    const cv::Size& portrait_size,
    const int VerticalOffset,
    const cv::Vec3b& back_color)
{
    SemiData semi = PortraitProcessSemi(std::move(photo), face_resize_to);
    return PortraitMix(semi, portrait_size, VerticalOffset, back_color);
}

struct SemiDataImpl
{
public:
    cv::Mat image; //CV_8UC3 R,G,B
    cv::Mat raw; //CV_8UC4 R,G,B,A
    cv::Rect face_area;
public:
    static SemiData NewWrapper()
    {
        return SemiData(new SemiDataImpl());
    }
    static SemiDataImpl& GetFrom(SemiData& wrapper)
    {
        return *wrapper._data;
    }
    static const SemiDataImpl& GetFrom(const SemiData& wrapper)
    {
        return *wrapper._data;
    }
}; //struct SemiDataImpl

SemiData::SemiData() throw()
    : _data(nullptr)
{ }

SemiData::SemiData(SemiDataImpl* data) throw()
    : _data(data)
{ }

SemiData::SemiData(SemiData&& another) throw()
    : _data(nullptr)
{
    Swap(another);
}

SemiData::~SemiData() throw()
{
    delete _data;
}

SemiData& SemiData::operator=(SemiData&& another) throw()
{
    Swap(another);
    return *this;
}

void SemiData::Swap(SemiData& another) throw()
{
    std::swap(_data, another._data);
}

cv::Mat SemiData::GetImage() const
{
    cv::Mat tmp;
    _data->image.copyTo(tmp);
    return tmp;
}
cv::Mat SemiData::GetImageWithLines() const
{
    cv::Mat tmp = GetImage();
    DrawGrabCutLines(tmp, _data->face_area);
    return tmp;
}

SemiData PortraitProcessSemi(
    cv::Mat&& photo,
    const int face_resize_to)
{
    SemiData semi = SemiDataImpl::NewWrapper();
    SemiDataImpl& data = SemiDataImpl::GetFrom(semi);
    data.image = photo;
    photo = cv::Mat();

    cv::Mat image_gray;
    cv::cvtColor(data.image,image_gray,CV_BGR2GRAY);
    data.face_area = DetectSingleFace(image_gray);
    data.face_area = TryCutPortrait(data.image, data.face_area, 0.6, 0.6, 0.4);
    data.face_area = ResizeFace(data.image, data.face_area,
                                cv::Size(face_resize_to, face_resize_to));
    data.raw = GetFrontBackMask(data.image, data.face_area);

    return semi;
}

cv::Mat PortraitMix(
    SemiData& semi,
    const cv::Size& crop_size,
    const int VerticalOffset,
    const cv::Vec3b& back_color)
{
    SemiDataImpl& data = SemiDataImpl::GetFrom(semi);
    //裁剪
    cv::Point face_center = CenterOf(data.face_area);
    cv::Rect crop_area(face_center.x - crop_size.width / 2,
                       face_center.y - crop_size.height / 2
                           + VerticalOffset,
                       crop_size.width,
                       crop_size.height);
    if (!Inside(crop_area, data.image))
    {
        cv::Rect new_crop_area_1 = Extend(data.image, crop_area, cv::Scalar(0,0,0));
        cv::Rect new_crop_area_2 = Extend(data.raw, crop_area, cv::Scalar(0,0,0,0));
        assert(new_crop_area_1 == new_crop_area_2);
        data.face_area.x += new_crop_area_1.x - crop_area.x;
        data.face_area.y += new_crop_area_1.y - crop_area.y;
        crop_area = new_crop_area_1;
    }

    //替换背景
    cv::Mat mix = Mix(data.image(crop_area), data.raw(crop_area), back_color);

    return mix;
}


}
