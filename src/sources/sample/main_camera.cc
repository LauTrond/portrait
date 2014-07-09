#include <iostream>
#include <exception>
#include <vector>

#include "opencv2/opencv.hpp"

#include "portrait/portrait.hh"
#include "sybie/common/Time.hh"

using namespace portrait;

const std::string WindowName = "Portrait";
enum { FrameWidth = 1280, FrameHeight = 720 };
enum { FaceResizeTo = 200 };
enum { PortraitWidth = 300, PortraitHeight = 400 };

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
    if (!cam.isOpened())
        throw std::runtime_error("Failed open camera.");
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
        sybie::common::StatingTestTimer::ResetAll();
        sybie::common::StatingTestTimer timer("All");

        try
        {
            //抠图
            SemiData semi = PortraitProcessSemi(std::move(frame), FaceResizeTo);
            cv::imshow(WindowName + "_src", semi.GetImageWithLines());

            //针对每种背景色混合背景
            for (int i = 0 ; i < NewBackColor.size() ; i++)
            {
                //混合
                cv::Mat img_mix = PortraitMix(
                    semi, cv::Size(PortraitWidth, PortraitHeight),
                    0, NewBackColor[i]);
                //显示结果
                cv::imshow(WindowName + std::to_string(i), img_mix);
            }
        }
        catch (std::exception& err)
        {
            std::cout << "Error: " << err.what() << std::endl;
            continue;
        }

        timer.Finish();
        sybie::common::StatingTestTimer::ShowAll(std::cout);
    }
    return 0;
}
