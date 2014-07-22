#include "portrait/facedetect.hh"

#include "sybie/common/RichAssert.hh" //sybie_assert
#include "sybie/datain/datain.hh" //sybie::datain::GetTemp

#include "portrait/exception.hh"

namespace portrait {

namespace {

/* 这个类通过继承cv::CascadeClassifier，允许从FileNode加载旧式分类器。
 *
 * 解释：
 * （1）cv::CascadeClassifier支持加载OpenCV 1.x（旧）或OpenCV 2.x（新）的分类器。
 * （2）新分类器可以通过文件或内存方式（cv::FileNode）加载，
 *     但由于未知动机，OpenCV 1.x分类器只能通过文件加载。
 * （3）OpenCV只集成OpenCV 1.x分类器，而本项目为了方便跨平台开发，
 *     把分类器数据嵌入到代码中。
 */
class MyCascadeClassifier : public cv::CascadeClassifier
{
public:
    MyCascadeClassifier(const cv::FileNode &node)
    {
        oldCascade = cv::Ptr<CvHaarClassifierCascade>(
            (CvHaarClassifierCascade*)node.readObj());
    }
}; //class MyCascadeClassifier

cv::CascadeClassifier& InitFaceCascadeClassifier()
{
    std::string data = sybie::datain::Load("haarcascade_frontalface_alt.xml");
    cv::FileStorage fs(data, cv::FileStorage::MEMORY | cv::FileStorage::READ);
    static MyCascadeClassifier face_cascade(fs.getFirstTopLevelNode());
    if (face_cascade.empty())
    {
        throw std::runtime_error("Failed load cascade.");
    }

    return face_cascade;
}

cv::CascadeClassifier& GetFaceCascadeClassifier()
{
    static cv::CascadeClassifier& face_cascade =
        InitFaceCascadeClassifier(); //首次调用时初始化
    return face_cascade;
}

}  //namespace

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

}  //namespace portrait
