#include "portrait/facedetect.hh"

#include "sybie/common/ManagedRes.hh" //sybie::common::TemporaryFile
#include "sybie/common/RichAssert.hh" //sybie_assert
#include "sybie/datain/datain.hh" //sybie::datain::GetTemp

#include "portrait/exception.hh"

namespace portrait {

cv::CascadeClassifier& InitFaceCascadeClassifier()
{
    static cv::CascadeClassifier face_cascade;
    sybie::common::TemporaryFile CascadeFile =
        sybie::datain::GetTemp("haarcascade_frontalface_alt.xml");
    bool succ = face_cascade.load(CascadeFile.GetFilename());
    if (!succ)
    {
        throw std::runtime_error((std::string)"Failed load cascade file: "
                                 + CascadeFile.GetFilename());
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
        image, faces, 1.1, 2,
        0|CV_HAAR_SCALE_IMAGE, cv::Size(128, 128));
    return faces;
}

cv::Rect DetectSingleFace(const cv::Mat& image)
{
    std::vector<cv::Rect> faces = DetectFaces(image);
    if (faces.size() == 0)
        throw Error(FaceNotFound);
    if (faces.size() > 1)
        throw Error(TooManyFaces);
    return faces[0];
}

}
