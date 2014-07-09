//UTF-8 with BOM

//禁止字符编码警告
#pragma warning(disable: 4819)

//允许使用tmpnam()
#define _CRT_SECURE_NO_WARNINGS

//Snappy用到的ssize_t是GCC特性，VC没有内建
typedef long long ssize_t;

#ifndef OPENCV_VERSION
#define OPENCV_VERSION "248"
#endif

#ifdef _DEBUG
#define OPENCV_CONFIGURE "d"
#else
#define OPENCV_CONFIGURE ""
#endif

#define OPENCV_LIB(libname) libname OPENCV_VERSION OPENCV_CONFIGURE ".lib"

#pragma comment(lib, OPENCV_LIB("opencv_calib3d"))
#pragma comment(lib, OPENCV_LIB("opencv_contrib"))
#pragma comment(lib, OPENCV_LIB("opencv_core"))
#pragma comment(lib, OPENCV_LIB("opencv_features2d"))
#pragma comment(lib, OPENCV_LIB("opencv_flann"))
#pragma comment(lib, OPENCV_LIB("opencv_gpu"))
#pragma comment(lib, OPENCV_LIB("opencv_highgui"))
#pragma comment(lib, OPENCV_LIB("opencv_imgproc"))
#pragma comment(lib, OPENCV_LIB("opencv_legacy"))
#pragma comment(lib, OPENCV_LIB("opencv_ml"))
#pragma comment(lib, OPENCV_LIB("opencv_nonfree"))
#pragma comment(lib, OPENCV_LIB("opencv_objdetect"))
#pragma comment(lib, OPENCV_LIB("opencv_photo"))
#pragma comment(lib, OPENCV_LIB("opencv_stitching"))
#pragma comment(lib, OPENCV_LIB("opencv_ts"))
#pragma comment(lib, OPENCV_LIB("opencv_video"))
#pragma comment(lib, OPENCV_LIB("opencv_videostab"))
