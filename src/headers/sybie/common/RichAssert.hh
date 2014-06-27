//sybie/common/RichAssert.hh

#ifndef INCLUDE_SYBIE_COMMON_RICH_ASSERT_HH
#define INCLUDE_SYBIE_COMMON_RICH_ASSERT_HH

#include <cstdlib>
#include <iostream>

namespace sybie {
namespace common {

class Assert
{
public:
    Assert(bool check) : _check(check) { }

    ~Assert()
    {
        if (!_check)
            exit(EXIT_FAILURE);
    }

    template<class T>
    Assert operator<<(const T& x) const
    {
        if (!_check)
            std::cerr<<x;
        bool check = _check;
        _check = true;
        return Assert(check);
    }
private:
    mutable bool _check;
}; //class Asert

class Trap {};
static Trap trap;
template<class T>
inline const Trap& operator<<(const Trap& trap, const T&)
{ return trap; }

}  //namespace common
}  //namespace sybie

#ifdef _DEBUG
#define sybie_assert(check) sybie::common::Assert(check) \
    << "Assertion failed: (" << #check << ")\n" \
    << "Function: " << __FUNCTION__ << "()\n" \
    << "File:" << __FILE__ << " Line:" << __LINE__ << "\n"
#else
#define sybie_assert(check) sybie::common::trap << 1
#endif

#define SHOW(val) #val << "=" << val << "\n"

#endif
