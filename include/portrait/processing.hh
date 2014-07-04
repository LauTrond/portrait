//portrait/processing.hh

#ifndef INCLUDE_PORTRAIT_PROCESSING_HH
#define INCLUDE_PORTRAIT_PROCESSING_HH

#include "opencv2/opencv.hpp"

namespace portrait {

/* 抠除人像、替换背景。
 * 这个函数是简易接口，打包执行PortraitProcessSemi和PortraitMix。
 * 如果需要一次抠图多次替换背景，或添加抠图关键点，应使用
 * PortraitProcessSemi、SetStroke、PortraitMix。
 *
 * photo：类型是CV_8UC3、格式使BGR的照片。
 * face_resize_to：指定图片被缩放后人脸的大小。
 * crop_size：指定裁剪的尺寸
 * vertical_offset：指定垂直方向的偏移量，0表示人脸完全居中。
 * back_color：背景色。
 * 返回：处理完成的图片。
 */
cv::Mat PortraitProcessAll(
    const cv::Mat& photo, //CV_8UC3
    const int face_resize_to = 200,
    const cv::Size& crop_size = cv::Size(300,400),
    const int vertical_offset = 0,
    const cv::Vec3b& back_color = cv::Vec3b(240,240,240));

struct SemiDataImpl;

/* 代表抠图结果，但未替换背景背景。
 * 对SemiData的修改不是线程安全的，但可以安全地并发读取。
 */
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
    //获取图片的大小，即GetImage、GetAlpha、GetImageWithLines返回的Mat尺寸。
    cv::Size GetSize() const;
    //获取人脸位置
    cv::Rect GetFaceArea() const;
    //获取替换背景前的图片，已被缩放到目标分辨率。
    //返回的Mat是副本，可被修改而不影响SemiData内部行为。
    cv::Mat GetImage() const;
    //获取照片中人像（前景）区域的掩码，尺寸同GetImage()的返回值，格式为CV_8UC1
    //对每一个像素，0表示背景，255表示前景，介乎于1-254之间表示半透明。
    //返回的Mat是副本，可被修改而不影响SemiData内部行为。
    cv::Mat GetAlpha() const;
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

/* 对照片执行人和背景分离（抠图）处理。
 * 本函数做了这些工作：
 * 1）执行人脸检测。
 * 2）按照一个默认比例（人脸大小的2.2*1.8）裁剪照片。
 * 3）缩小或者拉升照片，使人脸缩放到指定大小。
 * 4）执行前景和背景分离（抠图）并返回结果。
 *
 * photo：类型是CV_8UC3、格式使BGR的照片。
 * face_resize_to：指定图片被缩放后人脸的大小。
 * 返回：抠图结果（中间数据）
 *
 * 若找不到人脸或者找到超过一个人脸，抛出异常：portrait::Error
 * 参见portrait::Error的定义。
 */
SemiData PortraitProcessSemi(
    const cv::Mat& photo,
    const int face_resize_to);

/* 设置抠图的关键点，并重新抠图。关键点可提高抠图的准确率。
 * semi：抠图结果
 * stroke：类型为CV_8UC1，尺寸为SemiData::GetSize()
 *        每个单元，cv::GC_BGD表示背景，cv::GC_FGD表示前景，
 *        其它表示自动。
 *        如果是空，清空之前设置的关键点。
 *
 * 这个函数会替换之前已经设置的关键点（如果有）并重新抠图。
 */
void SetStroke(SemiData& semi, const cv::Mat& stroke);

/* 检查用PortraitMix替换背景时是否完整裁剪。
 * 所谓完整裁剪，即PortraitMix裁剪时不会在除头顶之外的方向扩展，而且头顶方向也不会剪掉头发。
 * 一般来说，如果返回false，表示拍照时没有给人脸附近留有足够的空间，应重新拍照。
 */
bool CanCropIntegrallty(
    const SemiData& semi,
    const cv::Size& crop_size,
    const int vertical_offset);

/* 背景替换。一般这个功能可以输出符合规格要求的证件照。
 * semi：抠图的结果。
 * crop_size：指定裁剪的尺寸
 * vertical_offset：指定垂直方向的偏移量，0表示人脸完全居中。
 * back_color：背景色。
 * mix_alpha：混合比例，1表示完全替换背景，0表示完全不替换，
 *            1和0之间表示按一定的比例替换
 * 返回：替换背景后的照片。
 *
 * 注意1：
 * crop_size和vertical_offset决定裁剪范围，
 * 如果超出semi中的照片范围，
 * 本函数会自动在照片的边缘补充back_color指定的底色。
 * 在产品应用中，最好不依赖自动扩展功能，而是引导用户拍照时预留更多空间，
 * 包括上、下、左、右的空间。
 * 通过CanCropIntegrallty函数可以评估是否有足够的空间。
 */
cv::Mat PortraitMix(
    const SemiData& semi,
    const cv::Size& crop_size,
    const int vertical_offset,
    const cv::Vec3b& back_color,
    const double mix_alpha = 1.0);

/* 背景替换，但不裁剪也不扩展。一般这个功能用于编辑和预览。
 * 参数和返回内容同PortraitMix。
 */
cv::Mat PortraitMixFull(
    const SemiData& semi,
    const cv::Vec3b& back_color,
    const double mix_alpha = 1.0);

}  //namespace portrait

#endif
