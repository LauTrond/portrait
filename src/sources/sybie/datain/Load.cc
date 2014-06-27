//Get.cc
//这是对datain.hh的实现

#include "sybie/datain/datain.hh"

#include <cstdio> //tmpnam()
#include <cstring> //strlen size_t
#include <stdexcept> //std::runtime_error
#include <memory> //std::unique_ptr
#include <future> //std::thread
#include <fstream> //std::ofstream

#include "sybie/common/Uncopyable.hh" //common::Uncopyable
#include "sybie/common/Streaming.hh" //common::PipeStream common::PipeBuffer
#include "sybie/common/ManagedRes.hh" //common::TemporaryFile
//#include "sybie/common/Log.hh" //(debug) LOG_TRACE_POINT

#include "snappy.h" //snappy::Uncompress

#include "sybie/datain/Coding.hh" //Decode
#include "sybie/datain/Pool.hh" //Pool
#include "sybie/datain/Stream.hh" //ReadDataBuffer WriteDataBuffer

namespace sybie {
namespace datain {

class DataLoader : common::Uncopyable
{
public:
    DataLoader(const char* data_txt, size_t data_size)
        : _src_stream(const_cast<char*>(data_txt), data_size),
          _pipe_stream(),
          _decode_result(std::async([&]{
              auto os = _pipe_stream.GetOutputStream();
              Decode(_src_stream, os);
          }))
    { }

    ~DataLoader()
    {
        _decode_result.wait();
    }

    size_t GetSize()
    {
        common::PipeInputStream pipe_in_stream = _pipe_stream.GetInputStream();
        StreamSource source(pipe_in_stream, GetDecodeResultSize(_src_stream.Size()));

        uint32_t _dst_size;
        if (!snappy::GetUncompressedLength(&source, &_dst_size))
            throw std::runtime_error(
                "DataLoader::GetSize: GetUncompressedLength failed.");

        return _dst_size;
    }

    void Uncompress(char* out_bytes)
    {
        common::PipeInputStream pipe_in_stream = _pipe_stream.GetInputStream();
        StreamSource source(pipe_in_stream, GetDecodeResultSize(_src_stream.Size()));

        if (!snappy::RawUncompress(&source, out_bytes))
            throw std::runtime_error(
                "DataLoader::Uncompress: RawUncompress failed.");
    }

private:
    common::ByteArrayStream _src_stream;
    common::PipeStream _pipe_stream;
    std::future<void> _decode_result;
}; //class DataLoader

std::string Load(const std::string& data_id)
{
    return LoadOnData(Pool::GetGlobalPool().Get(data_id.c_str()));
}

std::string LoadOnData(const char* data_txt)
{
    size_t data_size = strlen(data_txt);
    size_t dst_size;

    {
        DataLoader loader(data_txt, data_size);
        dst_size = loader.GetSize();
    }
    std::string result(dst_size, '\0');

    {
        DataLoader loader(data_txt, data_size);
        loader.Uncompress(&(result[0]));
    }

    return result;
}

common::TemporaryFile GetTemp(const std::string& data_id)
{
    const std::string data = Load(data_id);
    common::TemporaryFile tmp_file;
    std::ofstream ofs(tmp_file.GetFilename());
    if (!ofs)
        throw std::runtime_error("GetTemp: Cannot open tempory file.");
    ofs.write(data.data(), data.size());
    if (!ofs)
        throw std::runtime_error("GetTemp: Cannot write tempory file.");
    return tmp_file;
}

} //namespace datain
} //namespace sybie
