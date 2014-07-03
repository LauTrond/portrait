//portrait/exception.hh

#include <stdexcept>

namespace portrait {

enum ErrorType
{
    FaceNotFound = 1, //找不到人脸
    TooManyFaces = 2, //找到超过一个人脸
    OutOfRange = 3    //越界
};

class Error : public std::logic_error
{
public:
    Error(ErrorType type) throw();
    ~Error() throw();
    inline ErrorType Type() const { return _type; }
private:
    ErrorType _type;
};

}  //namespace portrait
