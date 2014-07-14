//在VC++下，所有C++源文件都包含这个文件（使用强制包含文件功能）。

//禁止编译UTF-8 without BOM源代码时的警告。
#pragma warning(disable: 4819)

//允许使用tmpnam()
#define _CRT_SECURE_NO_WARNINGS

//Snappy用到的ssize_t是GCC特性，VC没有内建
typedef long long ssize_t;

//下面代码连接OpenCV

#ifndef OPENCV_VERSION //可以通过修改这个变量来修改连接的OpenCV版本。
#define OPENCV_VERSION "248" //OpenCV 2.4.8
#endif

#ifdef _DEBUG
#define OPENCV_CONFIGURE "d"
#else
#define OPENCV_CONFIGURE ""
#endif

#define OPENCV_LIB(libname) "opencv_" libname OPENCV_VERSION OPENCV_CONFIGURE ".lib"
#pragma comment(lib, OPENCV_LIB("core"))
#pragma comment(lib, OPENCV_LIB("highgui"))
#pragma comment(lib, OPENCV_LIB("imgproc"))
#pragma comment(lib, OPENCV_LIB("objdetect"))
#undef OPENCV_LIB
