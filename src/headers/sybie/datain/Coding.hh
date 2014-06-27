//Coding.hh
//DataIn的编码模块，在二进制的数据转和文本数据间转换
//一般来说，二进制数据和文本数据的大小比例是3:4

#include <cstddef> //size_t
#include <iosfwd> //std::istream std::ostream (fwd)
#include <string> //std::string

#ifndef INCLUDE_SYBIE_DATAIN_CODING_HH
#define INCLUDE_SYBIE_DATAIN_CODING_HH

namespace sybie {
namespace datain {

size_t Encode(std::istream& bin, std::ostream& txt);
size_t Decode(std::istream& txt, std::ostream& bin);

std::string Encode(const std::string& bin);
std::string Decode(const std::string& txt);

size_t GetEncodeResultSize(const size_t bin_size);
size_t GetDecodeResultSize(const size_t txt_size);

} //namespace datain
} //namespace sybie

#endif //ifndef
