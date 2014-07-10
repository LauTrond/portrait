#include <iostream>
#include <exception>

#include "opencv2/opencv.hpp"

#include "portrait/portrait.hh"
#include "sybie/common/Time.hh"

namespace portrait {

const std::string WindowName = "Portrait";
enum { FaceResizeTo = 200 };
enum { PortraitWidth = 300, PortraitHeight = 400 };

const std::vector<cv::Vec3b> NewBackColor({
{243, 191, 0},
{0, 0, 255},
{240, 240, 240}
});

int _main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout<<"Usage: " << argv[0] << " <files...>"<<std::endl;
        return 1;
    }

    cv::namedWindow(WindowName + "_src", CV_WINDOW_AUTOSIZE);
    for (int i = 0; i < NewBackColor.size(); i++)
        cv::namedWindow(WindowName + std::to_string(i), CV_WINDOW_AUTOSIZE);

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
            SemiData semi = PortraitProcessSemi(std::move(image), FaceResizeTo);
            image_show = semi.GetImageWithLines();

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
            cv::putText(image_show, err.what(), cv::Point(0,60),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0,0,255));
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

}  //namespace portrait

int main(int argc, char** argv)
{
    try
    {
        portrait::_main(argc, argv);
    }
    catch (std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
