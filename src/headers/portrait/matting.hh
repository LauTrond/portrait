//portrait/matting.hh
//包含本项目所需的边缘混合算法实现

#ifndef INCLUDE_PORTRAIT_MATTING_HH
#define INCLUDE_PORTRAIT_MATTING_HH

#include "opencv2/opencv.hpp"

namespace portrait {

/* 边缘混合
 * image：图像
 * mask：cv::grabCut结果的前景／背景掩码
 * 返回一个用于评估准确率的值，应接纳最小的结果
 */
int MatBorder(cv::Mat& raw, const cv::Mat& image, const cv::Mat& mask);

}  //namespace portrait

#endif //ifndef
