#include <iostream>

#include "opencv2/opencv.hpp"

#include "portrait/portrait.hh"

#include "sybie/common/Time.hh"

using namespace sybie;
using namespace portrait;

const std::string WindowName = "Potrait";
enum { FrameWidth = 1280, FrameHeight = 720 };
enum { FaceResizeTo = 200};

const std::vector<cv::Vec3b> NewBackColor({
{243, 191, 0},
{0, 0, 255},
{240, 240, 240}
});

int main(int argc, char** argv)
{
    cv::namedWindow(WindowName, CV_WINDOW_AUTOSIZE);
    cv::namedWindow(WindowName + "_src", CV_WINDOW_AUTOSIZE);
    for (int i = 0 ; i < NewBackColor.size() ; i++)
        cv::namedWindow(WindowName + std::to_string(i), CV_WINDOW_AUTOSIZE);

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

        SemiData semi = PortraitProcessSemi(std::move(frame), FaceResizeTo);
        cv::imshow(WindowName + "_src", semi.GetImage());

        for (int i = 0 ; i < NewBackColor.size() ; i++)
        {
            cv::Mat img_mix = PortraitMix(
                semi, cv::Size(250,350), 0, NewBackColor[i]);
            //显示结果
            cv::imshow(WindowName + std::to_string(i), img_mix);
        }

        //显示耗时
        common::DateTime finish_time = common::DateTime::Now();
        std::cout<<"Cost: "<<(finish_time - start_time)<<std::endl;
    }
    return 0;
}
