//Generate.hh

#include <iosfwd>
#include <string>

#ifndef INCLUDE_SYBIE_DATAIN_GENERATE_HH
#define INCLUDE_SYBIE_DATAIN_GENERATE_HH

namespace sybie {
namespace datain {

//若要生成的文件在VC下编译通过，一个字符串长度不可大于64K
enum { DefaultPartSize = 32 * 1024 };

void Generate(std::istream& is, std::ostream& os,
              const std::string& data_id,
              size_t part_size = DefaultPartSize);

} //namespace datain
} //namespace sybie

#endif //ifndef
