//Generate.cc
//这是对Generate.hh的实现

#include "sybie/datain/Generate.hh"

#include <cassert> //assert
#include <cstdint> //int64_t
#include <istream> //std::istream
#include <ostream> //std::ostream
#include <utility> //std::make_pair
#include <memory> //std::unique_ptr c++11
#include <future> //std::async c++11
#include <functional> ///std::ref c++11

#include "sybie/common/Streaming.hh"
#include "sybie/common/ManagedRes.hh"

#include "snappy.h"

#include "sybie/datain/Coding.hh" //Encode
#include "sybie/datain/Stream.hh" //StreamSink StreamSource

namespace sybie {
namespace datain {

enum { RowSize = 128 };

/* 从is读入数据，压缩、编码后写入os
 * istream is -> 压缩线程 -> pipe1 -> 编码线程 -> pipe2 -> 主线程组织代码 -> os
 */
void Generate(std::istream& is, std::ostream& os,
              const std::string& data_id,
              size_t part_size)
{
    if (part_size == 0)
        throw std::runtime_error("Generate: part_size = 0.");

    common::PipeStream pipe1, pipe2;

    //压缩线程
    auto compress_result = std::async([&]{
        auto _os = pipe1.GetOutputStream();
        StreamSink sink(*_os);

        int64_t src_size = common::GetStreamSize(is);
        if (src_size < 0)
            throw std::runtime_error("Generate: Cannot get size of input stream.");
        StreamSource source(is, src_size);

        snappy::Compress(&source, &sink);
    });
    //编码线程
    auto encode_result = std::async([&]{
        auto _is = pipe1.GetInputStream();
        auto _os = pipe2.GetOutputStream();
        Encode(*_is, *_os);
    });

    auto is_txt = pipe2.GetInputStream();
    //组织代码写入到os
    os << "#include \"sybie/datain/DataItem.hh\"" << std::endl
       << std::endl;
    std::unique_ptr<char[]> row_data(new char[RowSize + 1]);

    for (int part_index = 0 ; !is_txt->eof() ; part_index++)
    {
        os << "static sybie::datain::DataItem item"
           << part_index
           << "(\""<< data_id << "\","
           << part_index << ","
           << std::endl;
        size_t sum_bytes_read = 0;
        do
        {
            is_txt->read(row_data.get(),
                        std::min<size_t>(RowSize, part_size - sum_bytes_read));
            size_t bytes_read = is_txt->gcount();
            sum_bytes_read += bytes_read;

            row_data[bytes_read] = '\0';
            os << "\"";
            os << row_data.get();
            os << "\"" << std::endl;
        }
        while(is_txt->good() && sum_bytes_read < part_size);

        os << ");" <<std::endl;
    }

    //如果子线程有异常，在这里抛出
    compress_result.get();
    encode_result.get();
}

} //namespace datain
} //namespace sybie
