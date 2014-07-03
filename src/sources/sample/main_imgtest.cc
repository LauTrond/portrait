#include <iostream>
#include <exception>
#include <vector>

#include "opencv2/opencv.hpp"

#include "portrait/portrait.hh"
#include "sybie/common/Time.hh"

using namespace portrait;

const std::string WindowName = "Portrait";
enum { FaceResizeTo = 200 };
enum { PortraitWidth = 300, PortraitHeight = 400 };

const std::vector<cv::Vec3b> NewBackColor({
{243, 191, 0},
{0, 0, 255},
{240, 240, 240}
});

enum DrawingState {None, DrawFront, DrawBack};

struct SharedData
{
    SemiData semi;
    std::vector<cv::Point> front;
    std::vector<cv::Point> back;
    DrawingState drawing;
    const cv::Mat image_origin;
    cv::Mat image_drawed;
};

void onMouse( int event, int x, int y, int flags, void* userdata)
{
    SharedData& data = *(SharedData*)userdata;

    switch (event)
    {
    default: break;
    case cv::EVENT_RBUTTONDOWN:
        data.front.clear();
        data.back.clear();
        break;
    case cv::EVENT_LBUTTONDOWN:
        if (flags == cv::EVENT_FLAG_CTRLKEY) data.drawing = DrawFront;
        if (flags == cv::EVENT_FLAG_ALTKEY) data.drawing = DrawBack;
        break;
    case cv::EVENT_LBUTTONUP:
        data.drawing = None;
        break;
    case cv::EVENT_MOUSEMOVE:
        if (data.drawing == DrawFront) data.front.push_back();
    }

}

int main(int argc, char** argv)
{
    cv::namedWindow(WindowName + "_src", CV_WINDOW_AUTOSIZE);
    SharedData shared_data;
    cv::setMouseCallback(WindowName + "_src", onMouse, (MouseEventData*)&shared_data);
    for (int i = 0 ; i < NewBackColor.size() ; i++)
        cv::namedWindow(WindowName + std::to_string(i), CV_WINDOW_AUTOSIZE);

    if (argc < 2)
    {
        std::cout<<"Usage: imgtest <files...>"<<std::endl;
        return 1;
    }

    int index = 1; //显示的文件索引
    while (true)
    {
        const std::string filename(argv[index]);
        cv::Mat image = cv::imread(filename, CV_LOAD_IMAGE_COLOR);
        cv::Mat image_show = cv::Mat(PortraitHeight, PortraitWidth, CV_8UC3);
        image_show = cv::Scalar(0,0,0);
        try
        {
            //抠图
            data.semi = PortraitProcessSemi(std::move(image), FaceResizeTo);
            image_show = data.semi.GetImageWithLines();

            //针对每种背景色混合背景
            for (int i = 0 ; i < NewBackColor.size() ; i++)
            {
                //混合
                cv::Mat img_mix = PortraitMix(
                    data.semi, cv::Size(PortraitWidth, PortraitHeight),
                    0, NewBackColor[i]);
                //显示结果
                cv::imshow(WindowName + std::to_string(i), img_mix);
            }
        }
        catch (std::exception& err)
        {
            cv::putText(image_show, err.what(), cv::Point(0,60),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,0,255));
            data.semi = SemiData();
        }
        cv::putText(image_show, filename, cv::Point(0,30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,255,0));
        cv::imshow(WindowName + "_src", image_show);

        while (true)
        {
            int key = cv::waitKey(0);
            if (key == 27)
                return 0;
            if (key == ',')
            {
                index--;
                if (index == 0)
                    index = argc - 1;
                break;
            }
            if (key == '.')
            {
                index++;
                if (index == argc)
                    index = 1;
                break;
            }
        }

    }

    return 0;
}
