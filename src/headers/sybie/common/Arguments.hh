//sybie/common/Arguments.hh

#ifndef INCLUDE_SYBIE_COMMON_ARGUMENTS_HH
#define INCLUDE_SYBIE_COMMON_ARGUMENTS_HH

#include "sybie/common/Arguments_fwd.hh"

#include <string>
#include <vector>
#include <stdexcept>

namespace sybie {
namespace common {

static const char WithoutShortName = '\0';
static const char* WithoutLongName = "";

//参数的长名、短名允许使用的字符集合
static const char* ValidArgumentNameCharacters =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "-_";
//参数的长名第一个字符、短名不可以使用的字符
static const char* EcapingArgumentNameCharactersForStart =
    "-_";

struct Argument
{
public:
    Argument(const std::string& id,
             const std::string& long_name,
             const char short_name,
             const ShellArgumentType type,
             const std::string& memo = "")
        : id(id),
          long_name(long_name),
          short_name(short_name),
          type(type),
          memo(memo)
    { }

    //参数的标识，仅在程序中使用。可以使用任何ASCII字符。
    std::string id;

    /* 参数的长名。
     * 长名在程序启动参数中使用“--”开头。
     * 一个“--”后跟一个长名。
     * 长名可以由ValidArgumentNameCharacters中的字符集合组成，
     * 但第一个字符不可以在EcapingArgumentNameCharactersForStart集合中。
     * WithOutLongName（""）表示这个参数没有长名。
     */
    std::string long_name;

    /* 参数的一个字符短名。
     * 短名在程序启动参数中使用“-”开头。
     * 可以将多个短名连续写在一个“-”后面。
     * 短名可以是ValidArgumentNameCharacters中的字符集合或者'\0'，
     * 但EcapingArgumentNameCharactersForStart集合中除外。
     * 设置为WithOutShortName（'\0'），表示这个参数没有短名。
     */
    char short_name;

    /* 参数的类型，可以是Flag或者Variant
     * Flag表示这个参数是一个标记，没有设定值。
     * Variant表示这个参数需要一个设定值，在参数后紧跟“=”符号和参数值来设定。
     * 如果是Flag，这个参数的短名不可以和其它参数短名连续写在同一个“-”后面。
     */
    ShellArgumentType type;

    /* 这个参数的功能描述
     */
    std::string memo;
};

class ShellArguments
{
public: //实例操作
    ShellArguments();
    ShellArguments(const ShellArguments& another);
    ShellArguments(ShellArguments&& another) throw();
    virtual ~ShellArguments() throw();

    ShellArguments& operator=(const ShellArguments& another);
    ShellArguments& operator=(ShellArguments&& another) throw();
    void Swap(ShellArguments& another) throw();

public: //参数控制
    /* 添加一个参数，Argument各成员的含义参见Argument的定义
     * 强异常安全：若抛出异常，对ShellArguments对象没有修改。
     */
    void Add(const Argument& arg) throw(std::logic_error);

    //添加多各参数
    template<class InputIterator>
    inline void Add(InputIterator first, InputIterator last) throw(std::logic_error)
    {
        for (auto it = first ; it != last ; it++)
            Add(*it);
    }

    //添加多个参数
    inline void Add(std::initializer_list<Argument> il) throw(std::logic_error)
    {
        Add(il.begin(), il.end());
    }

    //获取参数列表
    std::vector<Argument> GetArguments() const;

public:
    /* 传入main函数参数执行解释。
     * 如果参数解释失败，抛出异常std::invalid_argument
     * 强异常安全：若抛出异常，对ShellArguments对象没有修改。
     */
    void Parse(int argc, char** argv) throw(std::invalid_argument);

public: //获取目前程序运行参数

    /* 获取由arg_id指定的参数值。如果这个参数被设置了多次，只返回最后一次
     * 对于type=Flag的参数，如果已使用则返回非空字符串，否则返回空字符串
     * 对于type=Variant的参数，如果已使用则返回"="后面的值，否则返回空字符串
     */
    std::string Get(const std::string& arg_id) const;

    // 判断arg_id指定的参数是否已设置。
    bool IsSet(const std::string& arg_id) const;

    // 获取所有无名参数
    std::vector<std::string> GetUnnamedArguments() const;

    // 获取程序执行命令
    std::string GetBinPath() const;

    // 功能同Get，参见Get的使用说明
    inline std::string operator[](const std::string& arg_id) const
    {
        return Get(arg_id);
    }

protected:
    virtual void CheckArguments() throw(std::invalid_argument);

private:
    void* _impl;
}; //class ShellArguments

class ShellArgumentsWithHelp : public ShellArguments
{
public:
    struct Memos
    {
        Memos()
            : help_arg_memo("Show this help information and exit."),
              unnamed_arg("")
        { }

        std::string help_arg_memo;
        std::string unnamed_arg;
    };
public:

    explicit ShellArgumentsWithHelp(const Memos& memos = Memos());
    ShellArgumentsWithHelp(const ShellArgumentsWithHelp& another);
    ShellArgumentsWithHelp(ShellArgumentsWithHelp&& another) throw();
    ~ShellArgumentsWithHelp() throw();

    ShellArgumentsWithHelp& operator=(const ShellArgumentsWithHelp& another);
    ShellArgumentsWithHelp& operator=(ShellArgumentsWithHelp&& another) throw();
    void Swap(ShellArgumentsWithHelp& another) throw();

public:
    inline bool Help() const
    {
        return IsSet("help");
    }

    std::string GetHelpInformation() const;

protected:
    virtual void CheckArguments() throw(std::invalid_argument);

private:
    Memos _memos;
}; //class ShellArgumentsWithHelp

}  //namespace common
}  //namespace sybie

#endif //ifndef INCLUDE_SYBIE_COMMON_ARGUMENTS_HH
