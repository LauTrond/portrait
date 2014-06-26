#include <iostream>
#include <vector>
#include <cmath>

#include "opencv2/opencv.hpp"

#include "sybie/datain/datain.hh"
#include "sybie/common/ManagedRes.hh"
#include "sybie/common/Time.hh"
#include "sybie/common/RichAssert.hh"

using namespace sybie;

const std::string WindowName = "Potrait";
enum { FrameWidth = 1280, FrameHeight = 720 };

const double
    PotraitUpperBound = 0.4,
    PotraitLowerBound = 0.4,
    PotraitWidth = 0.2;

const double
    BGWidth = 0.1,
    BGTop = 0.38,
    BGBottom = 0.1;

const cv::Vec3b NewBackColor
(243, 191, 0);
//(0, 0, 255);

int Dist(const cv::Vec3i& vec)
{
    return (int)sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
}

int main(int argc, char** argv)
{
    cv::namedWindow(WindowName, CV_WINDOW_AUTOSIZE);
    cv::namedWindow(WindowName+"0", CV_WINDOW_AUTOSIZE);
    cv::namedWindow(WindowName+"1", CV_WINDOW_AUTOSIZE);

    //初始化人脸检测模块
    cv::CascadeClassifier face_cascade;
    {
        sybie::common::TemporaryFile CascadeFile =
            sybie::datain::GetTemp("haarcascade_frontalface_alt.xml");
        if (!face_cascade.load(CascadeFile.GetFilename()))
        {
            std::cerr<<"Failed load cascade file: "<<CascadeFile.GetFilename()<<std::endl;
            return 1;
        }
    }

    //初始化摄像头
    cv::VideoCapture cam(-1);
    cam.set(CV_CAP_PROP_FRAME_WIDTH, FrameWidth);
    cam.set(CV_CAP_PROP_FRAME_HEIGHT, FrameHeight);

    while(true)
    {
        //拍照
        cv::Mat frame;
        int key;
        while((key = cv::waitKey(1)) == -1)
        {
            cam.read(frame);
            cv::imshow(WindowName, frame);
        }
        if (key == 27)
            break;

        //开始计时
        common::DateTime start_time = common::DateTime::Now();

        //人脸检测
        cv::Mat frame_gray;
        std::vector<cv::Rect> faces;
        cvtColor(frame, frame_gray, CV_BGR2GRAY);
        equalizeHist(frame_gray, frame_gray);
        face_cascade.detectMultiScale(
            frame_gray, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE, cv::Size(64, 64));
        if (faces.size() == 0)
        {
            std::cout<<"Failed detect face."<<std::endl;
            continue;
        }

        //裁剪人像
        cv::Rect face = faces[0];
        cv::Rect area = face;
        area.x = face.x - (int)(face.width * PotraitWidth);
        area.width = (int)(face.width * (1 + PotraitWidth * 2));
        area.y = face.y - (int)(face.height * PotraitUpperBound);
        area.height = (int)(face.height * (1 + PotraitUpperBound + PotraitLowerBound));
        if (   area.x < 0
            || area.x + area.width >= frame.cols
            || area.y < 0
            || area.y + area.height >= frame.rows)
        {
            std::cout<<"Face out of range."<<std::endl;
            continue;
        }

        cv::Mat img_potrait(frame, area);

        //组织初始mask
        int mask_left = (PotraitWidth - BGWidth) * face.width;
        int mask_right = area.width - mask_left;
        int mask_up = (PotraitUpperBound - BGTop) * face.height;
        int mask_down = area.height - (PotraitLowerBound - BGBottom) * face.height;
        cv::Mat mask(img_potrait.rows, img_potrait.cols, CV_8UC1);
        for (int r = 0 ; r < mask.rows ; r++)
            for (int c = 0 ; c < mask.cols ; c++)
            {
                uint8_t& m = mask.at<uint8_t>(r,c);
                if (((c < mask_left || c > mask_right)
                   && r < mask_down) || r < mask_up)
                    m = cv::GC_BGD;
                else
                    m = cv::GC_PR_FGD;
            }

        //抠图
        cv::Mat bgModel,fgModel; //临时空间
        cv::grabCut(img_potrait, mask, cv::Rect(), bgModel,fgModel, 3,
                    cv::GC_INIT_WITH_MASK);
        cv::imshow(WindowName+"0", img_potrait);

        //找出边缘像素
        cv::Mat mask_border(img_potrait.rows, img_potrait.cols, CV_8UC1);
        for (int r = 0 ; r < mask_border.rows ; r++)
            for (int c = 0 ; c < mask_border.cols ; c++)
            {
                uint8_t& mb = mask_border.at<uint8_t>(r,c);
                bool has_front = false;
                bool has_back = false;
                for (int rr = std::max(r-2,0) ; rr <= std::min(r+2,mask_border.rows-1) ; rr++)
                    for (int cc = std::max(c-2,0) ; cc <= std::min(c+2,mask_border.cols-1) ; cc++)
                    {
                        uint8_t& m = mask.at<uint8_t>(rr,cc);
                        if (m == cv::GC_BGD || m == cv::GC_PR_BGD)
                            has_back = true;
                        else
                            has_front = true;
                    }
                if (has_front && has_back)
                    mb = 128;
                else if (has_front)
                    mb = 255;
                else
                    mb = 0;
            }
        cv::Mat mask_alpha(img_potrait.rows, img_potrait.cols, CV_8UC1);
        cv::Mat back_color(img_potrait.rows, img_potrait.cols, CV_8UC3);
        //cv::Mat& mask_alpha = mask_border;
        for (int r = 0 ; r < mask_alpha.rows ; r++)
            for (int c = 0 ; c < mask_alpha.cols ; c++)
            {
                uint8_t& mb = mask_border.at<uint8_t>(r,c);
                uint8_t& ma = mask_alpha.at<uint8_t>(r,c);
                cv::Vec3b& bc = back_color.at<cv::Vec3b>(r,c);
                ma = mb; //默认混合
                bc = img_potrait.at<cv::Vec3b>(r,c);

                if (mb == 128) //边缘像素，计算混合比例
                {
                    int cnt_back = 0, cnt_front = 0;
                    cv::Vec3i sum_back(0,0,0), sum_front(0,0,0);

                    for (int rr = std::max(r-5,0) ; rr <= std::min(r+5,mask_alpha.rows-1) ; rr++)
                        for (int cc = std::max(c-5,0) ; cc <= std::min(c+5,mask_alpha.cols-1) ; cc++)
                        {
                            uint8_t& mbrange = mask_border.at<uint8_t>(rr,cc);
                            cv::Vec3b& pixel = img_potrait.at<cv::Vec3b>(rr,cc);
                            if (mbrange == 0)
                                sum_back += pixel, cnt_back++;
                            if (mbrange == 255)
                                sum_front += pixel, cnt_front++;
                        }

                    /*if (cnt_back > 0 && cnt_front > 0)
                    {
                        cv::Vec3i adv_back = sum_back / cnt_back;
                        cv::Vec3i adv_front = sum_front / cnt_front;
                        cv::Vec3i cur = img_potrait.at<cv::Vec3b>(r,c);
                        int dist_back = Dist(adv_back - cur);
                        int dist_front = Dist(adv_front - cur);
                        ma = 255 * dist_back / (dist_front + dist_back);
                    }*/

                    if (cnt_back > 0 && cnt_front > 0)
                    {
                        cv::Vec3i cur = (cv::Vec3i)img_potrait.at<cv::Vec3b>(r,c);
                        int dist_back = cnt_front * Dist(sum_back - cur * cnt_back);
                        int dist_front = cnt_back * Dist(sum_front - cur * cnt_front);
                        ma = 255 * dist_back / (dist_front + dist_back);
                    }
                    else if (cnt_back > 0 || cnt_front > 0)
                    {
                        ma = 255 * cnt_front / (cnt_back + cnt_front);
                    }

                    if (cnt_back > 0)
                        bc = sum_back / cnt_back;
                }
            }

        //mask应用到照片，前景合背景混合
        for (int r = 0 ; r < img_potrait.rows ; r++)
            for (int c = 0 ; c < img_potrait.cols ; c++)
            {
                uint8_t ma = mask_alpha.at<uint8_t>(r,c);
                if (ma < 255)
                {
                    double alpha = (double)ma / 255;
                    cv::Vec3b& pixel = img_potrait.at<cv::Vec3b>(r,c);
                    cv::Vec3b& back = back_color.at<cv::Vec3b>(r,c);
                    pixel += ((cv::Vec3i)NewBackColor - (cv::Vec3i)back) * (1 - alpha);
                }
            }

        //显示耗时
        common::DateTime finish_time = common::DateTime::Now();
        std::cout<<"Cost:"<<(finish_time - start_time)<<std::endl;

        //显示结果
        cv::imshow(WindowName+"1", img_potrait);
    }
    return 0;
}
