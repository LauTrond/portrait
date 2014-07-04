/* 用摄像头拍照，然后开始用鼠标调整抠图，最终输出人像
 */
#include <cstdint>
#include <iostream>
#include <exception>
#include <vector>

#include "opencv2/opencv.hpp"

#include "portrait/portrait.hh"
#include "sybie/common/Uncopyable.hh"

using namespace portrait;

const std::string WindowName = "PortraitEdit";
enum { FrameWidth = 1280, FrameHeight = 720 };
enum { FaceResizeTo = 300 };
enum { PortraitWidth = 450, PortraitHeight = 600 };
enum { PenSize = 5 };

const cv::Vec3b NewBackColor(243, 191, 0);

class Editor : sybie::common::Uncopyable
{
public:
    Editor(SemiData& semi, const std::string _caption)
        : _semi(semi),
          _window_name(WindowName + "_edit:" + _caption),
          _drawing_front(false), _drawing_back(false),
          _canvas(), _alpha(), _stroke(semi.GetSize(), CV_8UC1)
    {
        cv::namedWindow(_window_name, CV_WINDOW_AUTOSIZE);
        cv::setMouseCallback(_window_name, _OnMouse, this);
        _Clear();
    }

    ~Editor()
    {
        cv::destroyWindow(_window_name);
    }

    cv::Mat Get() const
    {
        while(cv::waitKey(0) != 27); //一直等待按下退出键
        return PortraitMix(_semi,
                           cv::Size(PortraitWidth, PortraitHeight),
                           0, NewBackColor);
    }
private:
    SemiData& _semi;
    std::string _window_name;

    bool _drawing_front, _drawing_back;

    cv::Mat _canvas;
    cv::Mat _alpha;
    cv::Mat _stroke;
private:

    void _Clear()
    {
        _stroke = cv::Scalar(cv::GC_PR_FGD);
        _Update();
    }

    void _Update()
    {
        SetStroke(_semi, _stroke);
        _canvas = PortraitMixFull(_semi, cv::Vec3b(0,255,0), 0.4);
        _alpha = _semi.GetAlpha();
        _FlashWindow();
    }

    void _FlashWindow()
    {
        cv::imshow(_window_name, _canvas);
    }

private:

    static void _OnMouse(int event, int x, int y, int flag, void* editor_ptr)
    {
        Editor& me = *(Editor*)editor_ptr;

        switch (event)
        {
        default:
            break;
        case cv::EVENT_LBUTTONDOWN:
            me._MouseLeftDown(x, y);
            break;
        case cv::EVENT_LBUTTONUP:
            me._MouseLeftUp();
            break;
        case cv::EVENT_MOUSEMOVE:
            me._MouseMove(x, y);
            break;
        case cv::EVENT_RBUTTONDOWN:
            me._MouseRightDown();
            break;
        }
    }

    void _MouseLeftDown(int x, int y)
    {
        _drawing_front = false;
        _drawing_back = false;
        switch (_alpha.at<uint8_t>(y, x))
        {
        default: //在边缘点下鼠标，什么都不做
            break;
        case 0: //在背景点下鼠标，开始标定背景
            _drawing_back = true;
            break;
        case 255: //在前景点下鼠标，开始标定前景
            _drawing_front = true;
            break;
        }
    }

    void _MouseLeftUp()
    {
        _drawing_front = false;
        _drawing_back = false;
        _Update();
    }

    void _MouseMove(int x, int y)
    {
        if (!_drawing_front && !_drawing_back)
            return;

        cv::Point point(x, y);
        cv::Scalar color(0, 0, 0);
        cv::Scalar stroke_type(cv::GC_PR_FGD);

        if (_drawing_front)
        {
            color = cv::Scalar(255, 0, 0);
            stroke_type = cv::Scalar(cv::GC_FGD);
        }
        if (_drawing_back)
        {
            color = cv::Scalar(0, 255, 0);
            stroke_type = cv::Scalar(cv::GC_BGD);
        }
        cv::circle(_canvas, point, PenSize, color, -1);
        cv::circle(_stroke, point, PenSize, stroke_type, -1);
        _FlashWindow();
    }

    void _MouseRightDown()
    {
        _Clear();
    }
}; //class Editor

int main(int argc, char** argv)
{
    cv::namedWindow(WindowName + "_cam", CV_WINDOW_AUTOSIZE);
    SemiData semi;

    if (argc >= 2) //打开一个图片
    {
        cv::Mat img = cv::imread(argv[1]);
        if (!img.data)
        {
            std::cerr << "Failed load " << argv[1] << std::endl;
            return 1;
        }

        try
        {
            semi = PortraitProcessSemi(img, FaceResizeTo);
        }
        catch (std::exception& err)
        {
            std::cerr << "Error: " << err.what() << std::endl;
            return 1;
        }
    }
    else //拍照
    {
        cv::VideoCapture cam(-1);
        cam.set(CV_CAP_PROP_FRAME_WIDTH, FrameWidth);
        cam.set(CV_CAP_PROP_FRAME_HEIGHT, FrameHeight);
        while (true)
        {
            cv::Mat frame;
            int key;
            while((key = cv::waitKey(1)) == -1)
            {
                cam.read(frame);
                cv::imshow(WindowName + "_cam", frame);
            }
            if (key == 27)
                return 0;

            try
            {
                semi = PortraitProcessSemi(frame, FaceResizeTo);
                break; //找到人脸，开始编辑
            }
            catch (std::exception& err)
            {
                std::cout << "Error: " << err.what() << std::endl;
                //人脸检测失败，重新拍照
            }
        }
    }

    {
        Editor editor(semi, "camera"); //创建窗口开始编辑
        cv::imshow(WindowName + "_cam", editor.Get()); //获取编辑结果
    }
    cv::waitKey(0);

    return 0;
}
