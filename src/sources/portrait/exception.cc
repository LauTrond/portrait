#include "portrait/exception.hh"

#include <string>

namespace portrait {

static std::string GetErrorDescription(ErrorType _type)
{
#define ERRORMSG(val) if(_type == val) return #val
    ERRORMSG(FaceNotFound);
    ERRORMSG(TooManyFaces);
    ERRORMSG(OutOfRange);
    return "";
}

Error::Error(ErrorType type)
    : std::logic_error(GetErrorDescription(_type)), _type(type)
{ }

Error::~Error() throw()
{ }

}  //namespace portrait
