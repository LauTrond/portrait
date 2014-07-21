//portrait/facedetect.hh

#ifndef INCLUDE_PORTRAIT_FACE_DETECT_HH
#define INCLUDE_PORTRAIT_FACE_DETECT_HH

#include "opencv2/opencv.hpp"

namespace portrait {

/* 显式初始化人脸检测模块。
 *
 * 一般来说，人脸检测模块会在首次调用DetectFaces
 * 或者DetectSingleFace时自动初始化，
 * 但多线程环境下依赖自动初始化是不安全的，
 * 应在并发调用人脸监测前调用本函数显式初始化。
 */
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
