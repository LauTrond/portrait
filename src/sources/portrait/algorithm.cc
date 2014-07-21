//这是对algorithm.hh的实现
#include "portrait/algorithm.hh"

#include "sybie/common/RichAssert.hh"
#include "sybie/common/Time.hh"
#include "sybie/common/Graphics/Structs.hh"

#include "portrait/math.hh"
#include "portrait/graphics.hh"
#include "portrait/matting.hh"

namespace portrait {

const int
    BorderSize = 5;

const int GrabCutInteration = 3;
//GrabCut的图片大小
const double
    GrabCutWidthScale = 0.5,
    GrabCutHeightScale = 0.5;
const double
    GrabCutInitWidthScale = 0.2,
    GrabCutInitHeightScale = 0.2;

//以下多个常数定义前景、背景划分的关键数值，全是检测出人脸矩形的长宽比例。

//绝对背景
const double
    BGWidth = 0.25,
    BGTop = 0.50,
    BGBottom = 0.00;
//可能背景
const double
    PR_BGWidth = 0.10,
    PR_BGTop = 0.30,
    PR_BGBottom = 0.10;
//绝对前景－脸
const double
    FGFaceTop = 0.00,
    FGFaceSide = -0.25,
    FGFaceBottom = -0.30;
//绝对前景-颈
const double
    FGNeckSide = -0.35;
//绝对前景-身体
const double
    FGBodyTop = 0.30,
    FGBodySide = 0.00;

cv::Rect TryCutPortrait(
    cv::Mat& image,
    const cv::Rect& face_area,
    const double max_up_expand,
    const double max_down_expand,
    const double max_width_expand)
{
    sybie_assert(Inside(face_area, image))
        << SHOW(face_area)
        << SHOW(image.rows)
        << SHOW(image.cols);

    cv::Rect area = face_area;
    area.x = face_area.x - (int)(face_area.width * max_width_expand);
    area.width = (int)(face_area.width * (1 + max_width_expand * 2));
    area.y = face_area.y - (int)(face_area.height * max_up_expand);
    area.height = (int)(face_area.height * (1 + max_up_expand + max_down_expand));

    area = OverlapArea(area, WholeArea(image));
    image = image(area);
    return cv::Rect(face_area.x - area.x, face_area.y - area.y,
                    face_area.width, face_area.height);
}

cv::Rect ResizeFace(
    cv::Mat& image,
    const cv::Rect& face_area,
    const cv::Size& face_resize_to)
{
    sybie_assert(Inside(face_area, image))
        << SHOW(face_area)
        << SHOW(image.rows)
        << SHOW(image.cols);

    double scale_x = (double)face_resize_to.width / face_area.width;
    double scale_y = (double)face_resize_to.height / face_area.height;
    cv::Size new_size(image.cols * scale_x, image.rows * scale_x);
    cv::Mat resized_image;
    cv::resize(image, resized_image, new_size, 0, 0, cv::INTER_AREA);
    image = resized_image;

    return cv::Rect (face_area.x * scale_x,
                     face_area.y * scale_y,
                     face_resize_to.width,
                     face_resize_to.height);
}

static void DrawMask(
    cv::Mat& image,
    const cv::Rect& face_area,
    bool clear,
    const cv::Scalar& clear_with_color,
    const cv::Scalar& front_color,
    const cv::Scalar& pr_back_color,
    const cv::Scalar& back_color,
    int thickness )
{
    //分割线
    int bg_up = face_area.y - face_area.height * BGTop;
    int bg_down = face_area.y + face_area.height * (1 + BGBottom);
    int bg_left = face_area.x - face_area.width * BGWidth;
    int bg_right = face_area.x + face_area.width * (1 + BGWidth);
    int pr_bg_up = face_area.y - face_area.height * PR_BGTop;
    int pr_bg_down = face_area.y + face_area.height * (1 + PR_BGBottom);
    int pr_bg_left = face_area.x - face_area.width * PR_BGWidth;
    int pr_bg_right = face_area.x + face_area.width * (1 + PR_BGWidth);
    int fgface_up = face_area.y - face_area.height * FGFaceTop;
    int fgface_down = face_area.y + face_area.height * (1 + FGFaceBottom);
    int fgface_left = face_area.x - face_area.width * FGFaceSide;
    int fgface_right = face_area.x + face_area.width * (1 + FGFaceSide);
    int fgneck_left = face_area.x - face_area.width * FGNeckSide;
    int fgneck_right = face_area.x + face_area.width * (1 + FGNeckSide);
    int fgbody_up = face_area.y + face_area.height * (1 + FGBodyTop);
    int fgbody_left = face_area.x - face_area.width * FGBodySide;
    int fgbody_right = face_area.x + face_area.width * (1 + FGBodySide);

    if (clear)
        cv::rectangle(image, WholeArea(image), clear_with_color, CV_FILLED);
    cv::rectangle(image,
                  TopLeft(image),
                  cv::Point(pr_bg_left - 1, pr_bg_down - 1),
                  pr_back_color, thickness);
    cv::rectangle(image,
                  TopRight(image),
                  cv::Point(pr_bg_right - 1, pr_bg_down - 1),
                  pr_back_color, thickness);
    cv::rectangle(image,
                  cv::Point(pr_bg_left, 0),
                  cv::Point(pr_bg_right - 1, pr_bg_up - 1),
                  pr_back_color, thickness);
    cv::rectangle(image,
                  TopLeft(image),
                  cv::Point(bg_left - 1, bg_down - 1),
                  back_color, thickness);
    cv::rectangle(image,
                  TopRight(image),
                  cv::Point(bg_right - 1, bg_down - 1),
                  back_color, thickness);
    cv::rectangle(image,
                  cv::Point(bg_left, 0),
                  cv::Point(bg_right - 1, bg_up - 1),
                  back_color, thickness);
    cv::rectangle(image,
                  cv::Point(fgface_left, fgface_up),
                  cv::Point(fgface_right - 1, fgface_down - 1),
                  front_color, thickness);
    cv::rectangle(image,
                  cv::Point(fgneck_left, fgface_down),
                  cv::Point(fgneck_right - 1, fgbody_up - 1),
                  front_color, thickness);
    cv::rectangle(image,
                  cv::Point(fgbody_left, fgbody_up),
                  cv::Point(fgbody_right - 1, image.rows - 1),
                  front_color, thickness);
}

namespace { //GetMixRaw函数内使用的组件

template<class T>
void CheckedFloodFill(cv::Mat& image,
                             const cv::Point& seed_point,
                             const cv::Scalar& old_val,
                             const cv::Scalar& new_val)
{
    if (cv::Scalar(image.at<T>(seed_point)) == old_val)
        cv::floodFill(image, seed_point, new_val);
}

void Clear(cv::Mat& mask)
{
    cv::Mat mask_tmp(mask.rows, mask.cols, CV_8UC1);
    for (int r = 0 ; r < mask.rows ; r++)
        for (int c = 0 ; c < mask.cols ; c++)
            mask_tmp.at<uint8_t>(r,c) = IsFront(mask.at<uint8_t>(r,c));

    CheckedFloodFill<uint8_t>(
        mask_tmp, cv::Point(0,0), 0, 2);
    CheckedFloodFill<uint8_t>(
        mask_tmp, cv::Point(mask_tmp.cols-1,0),  0, 2);
    CheckedFloodFill<uint8_t>(
        mask_tmp, cv::Point(mask_tmp.cols/2,mask_tmp.rows-1), 1, 3);

    for (int r = 0 ; r < mask.rows ; r++)
        for (int c = 0 ; c < mask.cols ; c++)
        {
            uint8_t t = mask_tmp.at<uint8_t>(r,c) ;
            uint8_t& m = mask.at<uint8_t>(r,c);
            if (t == 1 && m == cv::GC_PR_FGD) //孤立前景
                m = cv::GC_PR_BGD;
            if (t == 0 && m == cv::GC_PR_BGD) //孤立背景
                m = cv::GC_PR_FGD;
        }
}

} //namespace GetMixRaw内使用的组件

cv::Mat GetMixRaw(
    const cv::Mat& image,
    const cv::Rect& face_area,
    const cv::Mat& stroke)
{
    sybie_assert(Inside(face_area, image))
        << SHOW(face_area)
        << SHOW(image.rows)
        << SHOW(image.cols);

    //初始化前景/背景掩码
    cv::Mat mask(image.rows, image.cols, CV_8UC1);
    DrawMask(mask, face_area, true, cv::GC_PR_FGD,
             cv::GC_FGD, cv::GC_PR_BGD, cv::GC_BGD, CV_FILLED);

    //自定义的关键点
    if (stroke.data != nullptr)
    {
        for (int r = 0 ; r < stroke.rows ; r++)
            for (int c = 0 ; c < stroke.cols ; c++)
            {
                uint8_t stroke_point = stroke.at<uint8_t>(r,c);
                if ( stroke_point == cv::GC_FGD ||
                     stroke_point == cv::GC_BGD)
                    mask.at<uint8_t>(r,c) = stroke_point;
            }
    }

    //使用cv::grabCut分离前景和背景
    {
        sybie::common::StatingTestTimer timer("GetMixRaw.grabCut");

        cv::Size full_size(image.cols,
                           image.rows); //缩略图尺寸
        cv::Size grab_size(image.cols * GrabCutWidthScale,
                           image.rows * GrabCutHeightScale); //GrabCut缩略图尺寸
        cv::Size init_size(image.cols * GrabCutInitWidthScale,
                           image.rows * GrabCutInitHeightScale); //GrabCut初始化尺寸

        cv::Mat bgModel,fgModel; //前景模型、背景模型

        //初始化模型
        cv::Mat image_init, mask_init;
        cv::resize(image, image_init, init_size, 0, 0, cv::INTER_AREA);
        cv::resize(mask, mask_init, init_size, 0, 0, cv::INTER_NEAREST);
        cv::grabCut(image_init, mask_init, cv::Rect(),
                    bgModel,fgModel,
                    0, cv::GC_INIT_WITH_MASK);

        //抠图
        cv::Mat image_grab, mask_grab;
        cv::resize(image, image_grab, grab_size, 0, 0, cv::INTER_AREA);
        cv::resize(mask, mask_grab, grab_size, 0, 0, cv::INTER_NEAREST);
        cv::grabCut(image_grab, mask_grab, cv::Rect(),
                    bgModel,fgModel,
                    GrabCutInteration, cv::GC_EVAL);

        //抠图结果恢复到最大尺寸
        cv::resize(mask_grab, mask,
                   full_size, 0, 0,
                   cv::INTER_NEAREST);
    }

    {
        sybie::common::StatingTestTimer timer("GetMixRaw.Clear");
        Clear(mask);
    }

    cv::Mat raw(image.rows, image.cols, CV_8UC4);
    {
        sybie::common::StatingTestTimer timer("GetMixRaw.Matting");

        cv::Mat tmp_raw(BorderSize * 2 + 1, BorderSize * 2 + 1, CV_8UC4);
        std::unique_ptr<int[]> dist_map(new int[image.rows * image.cols]);

        for (int r = 0 ; r < image.rows ; r++)
            for (int c = 0 ; c < image.cols ; c++)
            {
                const cv::Vec3b& pixel = image.at<cv::Vec3b>(r,c);
                const uint8_t pixel_alpha =
                    IsFront(mask.at<uint8_t>(r,c)) ? 0xff : 0;
                raw.at<cv::Vec4b>(r,c) = cv::Vec4b(
                    pixel[0], pixel[1], pixel[2], pixel_alpha);
                dist_map[r * image.cols + c] = std::numeric_limits<int>::max();
            }

        for (int r = BorderSize ; r < image.rows - BorderSize ; r++)
            for (int c = BorderSize ; c < image.cols - BorderSize ; c++)
            {
                if (IsBack(mask.at<uint8_t>(r,c)))
                    continue;

                if (   IsFront(mask.at<uint8_t>(r - 1,c))
                    && IsFront(mask.at<uint8_t>(r + 1,c))
                    && IsFront(mask.at<uint8_t>(r,c - 1))
                    && IsFront(mask.at<uint8_t>(r,c + 1)))
                    continue;

                //边缘像素，计算边缘混合比例
                cv::Rect sub_area(c - BorderSize,
                                  r - BorderSize,
                                  BorderSize * 2 + 1,
                                  BorderSize * 2 + 1);
                int dist_cur = MatBorder(tmp_raw,
                                         image(sub_area),
                                         mask(sub_area));
                for (int rr = -BorderSize ; rr <= BorderSize ; rr++)
                    for (int cc = -BorderSize ; cc <= BorderSize ; cc++)
                    {
                        int& dist_old = dist_map[(r + rr) * image.cols + (c + cc)];
                        if (dist_old > dist_cur)
                        {
                            dist_old = dist_cur;
                            raw.at<cv::Vec4b>(r + rr, c + cc) =
                                tmp_raw.at<cv::Vec4b>(BorderSize + rr, BorderSize + cc);
                        }
                    }
            }
    }

    return raw;
}

void DrawGrabCutLines(
    cv::Mat& image,
    const cv::Rect& face_area)
{
    DrawMask(image, face_area,
             false, cv::Scalar(0,0,0),
             cv::Scalar(255,0,0), cv::Scalar(0,255,0), cv::Scalar(0,0,255),
             1);
}

cv::Rect Extend(
    cv::Mat& image,
    const cv::Rect& area,
    const cv::Scalar& border_pixel)
{
    int top = std::max(0, -area.y);
    int bottom = std::max(0, area.y + area.height - image.rows);
    int left = std::max(0, -area.x);
    int right = std::max(0, area.x + area.width - image.cols);
    cv::Mat image_new;
    cv::copyMakeBorder(image, image_new,
                       top, bottom, left, right,
                       cv::BORDER_CONSTANT, border_pixel);
    image = image_new;
    return SubArea(area, cv::Point(-left, -top));
}

cv::Mat Mix(
    const cv::Mat& image,
    const cv::Mat& raw,
    const cv::Vec3b& back_color,
    const double mix_alpha)
{
    assert(mix_alpha >= 0 && mix_alpha <= 1);
    cv::Mat image_mix(image.rows, image.cols, CV_8UC3);
    for (int r = 0 ; r < image.rows ; r++)
        for (int c = 0 ; c < image.cols ; c++)
        {
            const cv::Vec4b& p = raw.at<cv::Vec4b>(r,c);
            const uint8_t& pixiel_alpha = p[3];
            const cv::Vec3b& backc = *(const cv::Vec3b*)(const void*)&p;

            const cv::Vec3b& src = image.at<cv::Vec3b>(r,c);
            cv::Vec3b& mix = image_mix.at<cv::Vec3b>(r,c);
            mix = (cv::Vec3i)src + (((cv::Vec3i)back_color - (cv::Vec3i)backc)
                        * (1 - (double)pixiel_alpha / 255) * mix_alpha);
        }
    return image_mix;
}

}  //namespace portrait
