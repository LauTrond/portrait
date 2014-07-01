//portrait/facedetect.hh

#ifndef INCLUDE_PORTRAIT_FACE_DETECT_HH
#define INCLUDE_PORTRAIT_FACE_DETECT_HH

#include "opencv2/opencv.hpp"

namespace portrait {

//手动初始化人脸检测模块
//多线程并发检测前，手动下调用
void InitFaceDetect();

//检测人脸
//image是CV_8UC1（Gray）
std::vector<cv::Rect> DetectFaces(const cv::Mat& image);

//检测单个人脸
//如果找到超过一个人脸，或者没有找到人脸，抛出异常
//其余同DetectFaces
cv::Rect DetectSingleFace(const cv::Mat& image);

}  //namespace portrait

#endif
