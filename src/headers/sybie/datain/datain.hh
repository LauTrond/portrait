//datain.hh
//获取嵌入到程序里的数据

#ifndef INCLUDE_SYBIE_DATAIN_DATAIN_HH
#define INCLUDE_SYBIE_DATAIN_DATAIN_HH

#include <string>

namespace sybie {
namespace datain {

//根据数据ID（可能是文件名）获取源数据。
std::string Load(const std::string& data_id);

//从is读取文本数据并解码成数据源。is必须时可随机访问的。
std::string LoadOnStream(std::istream& is);

//将储存在data_txt中的文本数据（C-style）解码成源数据。
std::string LoadOnData(const char* data_txt);

} //namespace datain
} //namespace sybie

#endif //ifndef
