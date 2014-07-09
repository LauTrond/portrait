//Arguments.cc

#include "sybie/common/Arguments.hh"

#include <map>
#include <set>
#include <list>
#include <iostream>

#include "sybie/common/Text.hh"
#include "sybie/common/Math.hh"

namespace sybie {
namespace common {

class ShellArgumentsImpl
{
private:
    static std::set<char> _vchars;
    static std::set<char> _escchars;

public:
    ShellArgumentsImpl() = default;
    ShellArgumentsImpl(const ShellArgumentsImpl& another) = default;
    ShellArgumentsImpl(ShellArgumentsImpl&& another) = delete;
    ShellArgumentsImpl& operator=(const ShellArgumentsImpl& another) = default;
    ShellArgumentsImpl& operator=(ShellArgumentsImpl&& another) = delete;
    ~ShellArgumentsImpl() = default;

    void Add(const Argument& arg) throw(std::logic_error)
    {
        _Check(arg);

        {
            if (_args.count(arg.id) > 0)
                throw std::logic_error("ShellArgumentsImpl::Add : arg.id already exists.");
            _args.insert(std::make_pair(arg.id, arg));
        }

        if (arg.long_name != WithoutLongName)
        {
            if (_long_idx.count(arg.long_name) > 0)
                throw std::logic_error("ShellArgumentsImpl::Add : arg.long_name already exists.");
            _long_idx.insert(std::make_pair(arg.long_name, arg.id));
        }

        if (arg.short_name != WithoutShortName)
        {
            if (_short_idx.count(arg.short_name) > 0)
                throw std::logic_error("ShellArgumentsImpl::Add : arg.short_name already exists.");
            _short_idx.insert(std::make_pair(arg.short_name, arg.id));
        }
    }

    std::vector<Argument> GetArguments() const
    {
        std::vector<Argument> result;
        for (auto& arg : _args)
            result.push_back(arg.second);
        return result;
    }

    void Parse(int argc, char** argv) throw(std::invalid_argument)
    {
        if (argc == 0)
            return;

        _cmd = argv[0];
        bool named_arg_disabled = false;

        for (int i = 1 ; i < argc ; i++)
        {
            const std::string arg=argv[i];

            if (!named_arg_disabled)
            {
                const size_t eq_pos = arg.find('=');
                const bool eq_found = (eq_pos != std::string::npos);
                const std::string eq_left = eq_found ? arg.substr(0, eq_pos) : arg;
                const std::string eq_right = eq_found ? arg.substr(eq_pos + 1) : "";

                if (StartsWith(eq_left, "--"))
                {
                    const std::string arg_long_name = eq_left.substr(2);

                    if (arg_long_name.size() == 0 && !eq_found)
                    {
                        named_arg_disabled = true;
                        continue;
                    }

                    _SetArgument(_GetArgumentByLongName(arg_long_name), eq_found, eq_right);
                    continue;
                } //if (StartsWith(eq_left, "--"))

                if (StartsWith(eq_left, "-"))
                {
                    const std::string arg_short_names = eq_left.substr(1);

                    if (arg_short_names.size() > 1 && eq_found) //多个短参数连接在一个"-"后
                        throw std::invalid_argument("Unknown argument: " + eq_left);

                    for (char c : arg_short_names)
                        _SetArgument(_GetArgumentByShortName(c), eq_found, eq_right);
                    continue;
                } //if (StartsWith(eq_left, "-"))
            } //if(!disable_named_arg)

            _unnamed_args.push_back(arg);
        }
    }

    std::string Get(const std::string& arg_id) const
    {
        auto found = _proc_args.find(arg_id);
        if (found != _proc_args.end())
            return found->second;
        else
            return "";
    }

    bool IsSet(const std::string& arg_id) const
    {
        return _proc_args.count(arg_id) > 0;
    }

    std::vector<std::string> GetUnnamedArguments() const
    {
        return _unnamed_args;
    }

    std::string GetBinPath() const
    {
        return _cmd;
    }

private:
    std::map<std::string, Argument> _args;
    std::map<std::string, std::string> _long_idx;
    std::map<char, std::string> _short_idx;

    std::map<std::string, std::string> _proc_args;
    std::vector<std::string> _unnamed_args;
    std::string _cmd;

    void _Check(const Argument& arg) throw(std::logic_error)
    {
        if (arg.id == "")
            throw std::logic_error("Argument::id empty.");
        for (char c : arg.long_name)
            if (_vchars.count(c) == 0)
                throw std::logic_error(
                    std::string() + "Invalid character '"
                                  + c + "' in Argument::long_name. id="
                                  + arg.id);
        if (arg.long_name.size() > 0 && _escchars.count(arg.long_name[0]) > 0)
            throw std::logic_error(
                "Invalid first character in Argument::long_name. id="
                +arg.id);
        if (arg.short_name != WithoutShortName
            && (_vchars.count(arg.short_name)==0 || _escchars.count(arg.short_name)>0))
            throw std::logic_error(
                "Invalid first character in Argument::short_name. id="
                +arg.id);
        if (arg.short_name == WithoutShortName && arg.long_name == WithoutLongName)
            throw std::logic_error(
                "Neither short_name nor long_name specified. id="
                +arg.id);
    }

    const Argument& _GetArgumentByLongName(const std::string& long_name) throw(std::invalid_argument)
    {
        auto found = _long_idx.find(long_name);
        if (found == _long_idx.end())
            throw std::invalid_argument("Unknown argument: --" + long_name);
        return _args.at(found->second);
    }

    const Argument& _GetArgumentByShortName(const char short_name) throw(std::invalid_argument)
    {
        auto found = _short_idx.find(short_name);
        if (found == _short_idx.end())
            throw std::invalid_argument(std::string("Unknown argument: -") + short_name);
        return _args.at(found->second);
    }

    void _SetArgument(const Argument& arg,
                      bool has_eq,
                      const std::string& val) throw(std::invalid_argument)
    {
        std::string arg_name_mix;
        if (arg.long_name == WithoutLongName)
            arg_name_mix = std::string("-") + arg.short_name;
        else if (arg.short_name == WithoutShortName)
            arg_name_mix = std::string("--") + arg.long_name;
        else
            arg_name_mix = std::string("--") + arg.long_name
            + " ( -" + arg.short_name + " )";

        if (arg.type == Flag)
        {
            if (has_eq)
                throw std::invalid_argument(
                    "Argument " + arg_name_mix + " followed by '='");

            _proc_args[arg.id] = "1";
        }
        else if (arg.type == Variant)
        {
            if (!has_eq)
                throw std::invalid_argument(
                   "Argument " + arg_name_mix + " does not have a value specified.");

            _proc_args[arg.id] = val;
        }
    }
}; //class ShellArgumentsImpl

static std::set<char> InitSetByString(const std::string& str)
{
    std::set<char> result;
    for (char c : str)
        result.insert(c);
    return result;
}

std::set<char> ShellArgumentsImpl::_vchars(InitSetByString(ValidArgumentNameCharacters));
std::set<char> ShellArgumentsImpl::_escchars(InitSetByString(EcapingArgumentNameCharactersForStart));

//class ShellArguments

ShellArguments::ShellArguments()
    : _impl(new ShellArgumentsImpl())
{ }

ShellArguments::ShellArguments(const ShellArguments& another)
    : _impl(new ShellArgumentsImpl(*(ShellArgumentsImpl*)another._impl))
{ }

ShellArguments::ShellArguments(ShellArguments&& another) throw()
    : _impl(nullptr)
{
    Swap(another);
}

ShellArguments::~ShellArguments() throw()
{
    delete (ShellArgumentsImpl*)_impl;
}

ShellArguments& ShellArguments::operator=(const ShellArguments& another)
{
    ShellArguments tmp_new(*this);
    Swap(tmp_new);
    return *this;
}

ShellArguments& ShellArguments::operator=(ShellArguments&& another) throw()
{
    Swap(another);
    return *this;
}

void ShellArguments::Swap(ShellArguments& another) throw()
{
    std::swap(_impl, another._impl);
}

void ShellArguments::Add(const Argument& arg) throw(std::logic_error)
{
    ShellArguments tmp_new(*this);
    ((ShellArgumentsImpl*)tmp_new._impl)->Add(arg);
    Swap(tmp_new);
}

std::vector<Argument> ShellArguments::GetArguments() const
{
    return ((ShellArgumentsImpl*)_impl)->GetArguments();
}

void ShellArguments::Parse(int argc, char** argv) throw(std::invalid_argument)
{
    ShellArguments tmp_new(*this);
    ((ShellArgumentsImpl*)tmp_new._impl)->Parse(argc, argv);
    Swap(tmp_new);
    CheckArguments(); //virtual
}

std::string ShellArguments::Get(const std::string& arg_id) const
{
    return ((ShellArgumentsImpl*)_impl)->Get(arg_id);
}

bool ShellArguments::IsSet(const std::string& arg_id) const
{
    return ((ShellArgumentsImpl*)_impl)->IsSet(arg_id);
}

void ShellArguments::CheckArguments() throw(std::invalid_argument)
{ }

std::vector<std::string> ShellArguments::GetUnnamedArguments() const
{
    return ((ShellArgumentsImpl*)_impl)->GetUnnamedArguments();
}

std::string ShellArguments::GetBinPath() const
{
    return ((ShellArgumentsImpl*)_impl)->GetBinPath();
}

//class ShellArgumentsWithHelp

ShellArgumentsWithHelp::ShellArgumentsWithHelp(const Memos& memos)
    : ShellArguments(),
      _memos(memos)
{
    Add(Argument("help","help",WithoutShortName,Flag,"Show this help information and exit."));
}

ShellArgumentsWithHelp::ShellArgumentsWithHelp(const ShellArgumentsWithHelp& another)
    : ShellArguments(another),
      _memos(another._memos)
{ }

ShellArgumentsWithHelp::ShellArgumentsWithHelp(ShellArgumentsWithHelp&& another) throw()
    : ShellArguments(std::move(another)),
      _memos(std::move(another._memos))
{ }

ShellArgumentsWithHelp::~ShellArgumentsWithHelp() throw()
{ }

ShellArgumentsWithHelp& ShellArgumentsWithHelp::operator=(const ShellArgumentsWithHelp& another)
{
    ShellArguments::operator=(another);
    _memos = another._memos;
    return *this;
}

ShellArgumentsWithHelp& ShellArgumentsWithHelp::operator=(ShellArgumentsWithHelp&& another) throw()
{
    ShellArguments::operator=(std::move(another));
    _memos = std::move(another._memos);
    return *this;
}

void ShellArgumentsWithHelp::Swap(ShellArgumentsWithHelp& another) throw()
{
    ShellArguments::Swap(another);
    std::swap(_memos, another._memos);
}

    static std::vector<std::string> Split(const std::string& str, char sp_char)
    {
        std::vector<std::string> result;
        std::string tmp_str = str;
        size_t found;
        while ((found = tmp_str.find(sp_char)) != std::string::npos)
        {
            result.push_back(tmp_str.substr(0, found));
            tmp_str = tmp_str.substr(found + 1);
        }
        result.push_back(std::move(tmp_str));
        return result;
    }

    static void Align(std::list<std::vector<std::string>>& data)
    {
        //分割行（如果某个单元中含有\n，则分割成多行）
        for (auto line = data.begin() ; line != data.end() ; line++)
        {
            std::vector<std::string>& line_vec = *line;
            std::vector<std::vector<std::string>> new_lines;

            for (int col = 0 ; col < line_vec.size() ; col++)
            {
                std::string& col_str = line_vec[col];
                std::vector<std::string> split = Split(col_str, '\n');
                while (new_lines.size() + 1 < split.size())
                    new_lines.push_back(std::vector<std::string>(line_vec.size()));
                col_str = split[0];
                for (int i = 1 ; i < split.size() ; i++)
                    new_lines[i-1][col] = split[i];
            }

            auto next_line = line;
            next_line++;
            data.insert(next_line, new_lines.begin(), new_lines.end());
        }

        //计算列数
        size_t cols = 0;
        for (std::vector<std::string>& line : data)
            cols = std::max(cols, line.size());

        //计算每列大小
        std::vector<size_t> max_size_of_columns(cols,0);
        for (std::vector<std::string>& line : data)
        {
            for (size_t col = 0 ; col < cols && col < line.size() ; col++)
                SetMax(max_size_of_columns[col], line[col].size());
        }

        //用空格填充列
        for (std::vector<std::string>& line : data)
        {
            for (size_t col = 0 ; col < cols && col < line.size() ; col++)
                line[col] += std::string(max_size_of_columns[col] - line[col].size(), ' ');
        }
    }

std::string ShellArgumentsWithHelp::GetHelpInformation() const
{
    std::vector<Argument> args = GetArguments();
    std::string result;

    result += "Usage: "
            + GetFilename(GetBinPath())
            + " [options...] "
            + _memos.unnamed_arg
            + "\n";

    result += "\n";

    std::list<std::vector<std::string>> lines;
    for (auto& arg : args)
    {
        std::vector<std::string> line;

        line.push_back("  ");
        line.push_back((arg.short_name != WithoutShortName)?
                       (std::string("-") + arg.short_name):"");
        line.push_back((  arg.short_name != WithoutShortName
                        && arg.long_name != WithoutLongName) ?
                       " , " : "");
        line.push_back((arg.long_name != WithoutLongName) ?
                       (std::string("--") + arg.long_name) : "");
        line.push_back((arg.type == Variant) ?
                       "=..." : "");
        line.push_back(" ");
        line.push_back(arg.memo);

        lines.push_back(std::move(line));
    }

    Align(lines);
    for (auto& line : lines)
    {
        for (auto& col : line)
            result += col;
        result += "\n";
    }

    return result;
}

void ShellArgumentsWithHelp::CheckArguments() throw(std::invalid_argument)
{
    ShellArguments::CheckArguments();
}

}  //namespace common
}  //namespace sybie
