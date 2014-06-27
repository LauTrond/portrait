//portrait/exception.hh

#include <stdexcept>

namespace portrait {

enum ErrorType
{
    FaceNotFound = 1,
    TooManyFaces = 2,
    OutOfRange = 3
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
