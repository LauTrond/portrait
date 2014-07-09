//datain.hh
//获取嵌入到程序里的数据

#ifndef INCLUDE_SYBIE_DATAIN_DATAIN_HH
#define INCLUDE_SYBIE_DATAIN_DATAIN_HH

#include <string>

#include "sybie/common/ManagedRes_fwd.hh"

namespace sybie {
namespace datain {

//根据数据ID（可能是文件名）获取源数据。
std::string Load(const std::string& data_id);

//从is读取文本数据并解码成数据源。is必须时可随机访问的。
std::string LoadOnStream(std::istream& is);

//将储存在data_txt中的文本数据（C-style）解码成源数据。
std::string LoadOnData(const char* data_txt);

//根据数据ID加载数据到一个临时文件，返回一个TemporyFile实例，
//可通过TemporyFile::GetFilename()获取文件名。
//TemporyFile析构时会自动删除该文件。
//调用的程序需要包含sybie/common/ManagedRes.hh
common::TemporaryFile GetTemp(const std::string& data_id);

} //namespace datain
} //namespace sybie

#endif //ifndef
