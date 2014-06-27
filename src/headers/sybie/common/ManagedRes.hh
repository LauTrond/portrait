//sybie/common/ManagedRes.hh

#ifndef INCLUDE_SYBIE_COMMON_MANAGED_RES_HH
#define INCLUDE_SYBIE_COMMON_MANAGED_RES_HH

#include "sybie/common/ManagedRes_fwd.hh"

#include <functional>
#include <string>

namespace sybie {
namespace common {

//构造时指定需要执行的函数，析构时自动调用。可用于资源的自动释放。
//一般不直接使用本类，而时使用宏ON_SCOPE_EXIT
class ScopeGuard
{
public:
    inline explicit ScopeGuard(std::function<void()> onExitScope)
        : onExitScope_(onExitScope)
    { }
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    inline ~ScopeGuard()
    {
        onExitScope_();
    }
private:
    std::function<void()> onExitScope_;
}; //class ScopeGuard

//构造时，生成一个临时文件名，可使用GetFilename获得文件名，析构时尝试删除这个文件
class TemporaryFile
{
public:
    //自动生成临时文件名，保证不存在，保证可写入。（使用C标准函数tmpnam(char*)）
    TemporaryFile();
    //指定一个文件名
    explicit TemporaryFile(const std::string& filename);
    //析构时尝试删除文件，删除失败不抛出异常
    ~TemporaryFile() throw();
    //不允许复制
    TemporaryFile(const TemporaryFile&) = delete;
    //移动构造，从另外一个实例中夺过文件的生存期管理权。
    //被夺的实例析构时不再会删除文件。
    TemporaryFile(TemporaryFile&& another) throw();
    //不允许复制
    TemporaryFile& operator=(const TemporaryFile&) = delete;
    //移动赋值，从另外一个实例中夺过文件的生存期管理权。
    //被夺的实例析构时不再会删除文件。
    TemporaryFile& operator=(TemporaryFile&& another) throw();
    //交换两个实例
    void Swap(TemporaryFile& another) throw();
public:
    //获取文件名
    std::string GetFilename() const;
    //直接尝试删除文件，不等待析构。如果失败（例如文件未创建），抛出异常。
    //删除后，GetFilename返回空串。
    void Remove();
private:
    std::string _filename;
};

}  //namespace common
}  //namespace sybie

/* 可保证函数无论以任何方式结束都会执行指定代码（释放资源）
 * 例如：
 * void func()
 * {
 *   int* int = new int();
 *   ON_SCOPE_EXIT([&]{ delete ptr; }); //Lamda表达式(C++11语法)
 *   //使用ptr
 * }
 */
#define ON_SCOPE_EXIT(callback) ::sybie::common::ScopeGuard SCOPE_GUARD_##__LINE__(callback)

#endif //ifndef
