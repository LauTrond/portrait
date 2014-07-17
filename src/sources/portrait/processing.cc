#include "portrait/processing.hh"

#include <cassert>

#include "portrait/exception.hh"
#include "portrait/algorithm.hh"
#include "portrait/graphics.hh"
#include "portrait/facedetect.hh"

namespace portrait {

cv::Mat PortraitProcessAll(
    const cv::Mat& photo,
    const int face_resize_to,
    const cv::Size& crop_size,
    const int vertical_offset,
    const cv::Vec3b& back_color)
{
    SemiData semi = PortraitProcessSemi(photo, face_resize_to);
    return PortraitMix(semi, crop_size, vertical_offset, back_color);
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

cv::Size SemiData::GetSize() const
{
    return _data->image.size();
}

cv::Rect SemiData::GetFaceArea() const
{
    return _data->face_area;
}

cv::Mat SemiData::GetImage() const
{
    cv::Mat tmp;
    _data->image.copyTo(tmp);
    return tmp;
}

cv::Mat SemiData::GetAlpha() const
{
    cv::Mat tmp(_data->raw.rows, _data->raw.cols, CV_8UC1);
    int from_to[] = {3, 0};
    cv::mixChannels(&_data->raw, 1, &tmp, 1, from_to, 1);
    return tmp;
}

cv::Mat SemiData::GetImageWithLines() const
{
    cv::Mat tmp = GetImage();
    DrawGrabCutLines(tmp, _data->face_area);
    return tmp;
}

SemiData PortraitProcessSemi(
    const cv::Mat& photo,
    const int face_resize_to)
{
    SemiData semi = SemiDataImpl::NewWrapper();
    SemiDataImpl& data = SemiDataImpl::GetFrom(semi);
    data.image = photo;

    cv::Mat image_gray;
    cv::cvtColor(data.image,image_gray,CV_BGR2GRAY);
    data.face_area = DetectSingleFace(image_gray);
    data.face_area = TryCutPortrait(
        data.image, data.face_area,
        0.6, 0.6, 0.4); //经验参数：裁剪出超过所有已知证件照规格的尺寸
    data.face_area = ResizeFace(data.image, data.face_area,
                                cv::Size(face_resize_to, face_resize_to));
    data.raw = GetMixRaw(data.image, data.face_area, cv::Mat());

    return semi;
}

void SetStroke(SemiData& semi, const cv::Mat& stroke)
{
    SemiDataImpl& data = SemiDataImpl::GetFrom(semi);
    data.raw = GetMixRaw(data.image, data.face_area, stroke);
}

    static cv::Rect GetCropArea(const cv::Rect face_area,
                                const cv::Size& crop_size,
                                const int vertical_offset)
    {
        cv::Point face_center = CenterOf(face_area);
        return cv::Rect(face_center.x - crop_size.width / 2,
                        face_center.y - crop_size.height / 2 + vertical_offset,
                        crop_size.width,
                        crop_size.height);
    }

bool CanCropIntegrallty(
    const SemiData& semi,
    const cv::Size& crop_size,
    const int vertical_offset)
{
    const SemiDataImpl& data = SemiDataImpl::GetFrom(semi);
    cv::Rect crop_area = GetCropArea(
        data.face_area, crop_size, vertical_offset);
    return crop_area.x >= 0
        && crop_area.x + crop_area.width <= data.image.cols
        && crop_area.y + crop_area.height <= data.image.rows
        && data.face_area.y >= data.face_area.height
                               * 0.3; //经验参数：人脸上方至少预留空间为脸高度的30%
}

cv::Mat PortraitMix(
    const SemiData& semi,
    const cv::Size& crop_size,
    const int vertical_offset,
    const cv::Vec3b& back_color,
    const double mix_alpha)
{
    const SemiDataImpl& data = SemiDataImpl::GetFrom(semi);
    //决定裁剪区域
    cv::Rect crop_area = GetCropArea(
        data.face_area, crop_size, vertical_offset);
    cv::Rect crop_area_in_image = OverlapArea(crop_area, WholeArea(data.image));

    //替换背景
    cv::Mat result = Mix(data.image(crop_area_in_image),
                         data.raw(crop_area_in_image),
                         back_color, mix_alpha);

    //如果上下左右空间不足，扩展边缘
    cv::Rect crop_area_extend = SubArea(crop_area, crop_area_in_image);
    if (!Inside(crop_area_extend, result))
        Extend(result, crop_area_extend,
               cv::Scalar(back_color[0], back_color[1], back_color[2]));

    return result;
}

cv::Mat PortraitMixFull(
    const SemiData& semi,
    const cv::Vec3b& back_color,
    const double mix_alpha)
{
    const SemiDataImpl& data = SemiDataImpl::GetFrom(semi);
    return Mix(data.image, data.raw, back_color, mix_alpha);
}

}  //namespace Portrait
