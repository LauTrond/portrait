#include "portrait/facedetect.hh"

#include "sybie/common/ManagedRes.hh" //sybie::common::TemporaryFile
#include "sybie/datain/datain.hh" //sybie::datain::GetTemp

#include "exception.hh"

namespace portrait {

cv::CascadeClassifier& InitFaceCascadeClassifier()
{
    static cv::CascadeClassifier face_cascade;
    sybie::common::TemporaryFile CascadeFile =
        sybie::datain::GetTemp("haarcascade_frontalface_alt.xml");
    if (!face_cascade.load(CascadeFile.GetFilename()))
    {
        std::cerr<<"Failed load cascade file: "
                 <<CascadeFile.GetFilename()<<std::endl;
        return 1;
    }
    return face_cascade;
}

cv::CascadeClassifier& GetFaceCascadeClassifier()
{
    static cv::CascadeClassifier& face_cascade =
        InitFaceCascadeClassifier(); //首次调用时初始化
    return face_cascade;
}

void InitFaceDetect()
{
    GetFaceCascadeClassifier();
}

std::vector<cv::Rect> DetectFaces(const cv::Mat& image)
{
    std::vector<cv::Rect> faces;
    GetFaceCascadeClassifier().detectMultiScale(
        frame_gray, faces, 1.1, 2,
        0|CV_HAAR_SCALE_IMAGE, cv::Size(64, 64));
    return faces;
}

cv::Rect DetectSingleFace(const cv::Mat& image)
{
    std::vector<cv::Rect> faces = DetectFaces(image);
    if (faces == 0)
        return Error(FaceNotFound);
    if (faces > 1)
        return Error(TooManyFaces);
    return faces[0];
}

}
