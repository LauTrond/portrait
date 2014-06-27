//sybie/common/Uncopyable.hh

#ifndef INCLUDE_SYBIE_COMMON_UNCOPYABLE_HH
#define INCLUDE_SYBIE_COMMON_UNCOPYABLE_HH

namespace sybie {
namespace common {

//派生这个类，可让子类不可复制（屏蔽复制构造函数和赋值函数）
class Uncopyable
{
protected:
    Uncopyable() {};
    ~Uncopyable() throw() {};
private:
    Uncopyable(const Uncopyable&);
    Uncopyable& operator=(const Uncopyable&);
};

}  //namespace common
}  //namespace sybie

#endif //ifndef INCLUDE_SYBIE_COMMON_UNCOPYABLE_HH
