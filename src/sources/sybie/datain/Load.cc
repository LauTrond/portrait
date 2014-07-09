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
    DataLoader(std::istream& is, size_t data_size)
        : _data_size(data_size),
          _pipe_stream(),
          _decode_result(std::async([&]{
              auto os = _pipe_stream.GetOutputStream();
              Decode(is, *os);
          }))
    { }

    ~DataLoader()
    {
        _decode_result.wait();
    }

    size_t GetSize()
    {
        auto pipe_in_stream = _pipe_stream.GetInputStream();
        StreamSource source(*pipe_in_stream,
                            GetDecodeResultSize(_data_size));

        uint32_t _dst_size;
        if (!snappy::GetUncompressedLength(&source, &_dst_size))
            throw std::runtime_error(
                "DataLoader::GetSize: GetUncompressedLength failed.");

        return _dst_size;
    }

    void Uncompress(char* out_bytes)
    {
        auto pipe_in_stream = _pipe_stream.GetInputStream();
        StreamSource source(*pipe_in_stream,
                            GetDecodeResultSize(_data_size));

        if (!snappy::RawUncompress(&source, out_bytes))
            throw std::runtime_error(
                "DataLoader::Uncompress: RawUncompress failed.");
    }

private:
    size_t _data_size;
    common::PipeStream _pipe_stream;
    std::future<void> _decode_result;
}; //class DataLoader

std::string Load(const std::string& data_id)
{
    PoolItemStream src_stream(Pool::GetGlobalPool(), data_id.c_str());
    return LoadOnStream(src_stream);
}

std::string LoadOnStream(std::istream& src_stream)
{
    src_stream.clear(); //按照C++11标准，istream::seekg会先清空eof状态，但在部分实现中却不这样。
    if(!src_stream.seekg(0))
        throw std::runtime_error("LoadOnStream: input stream is not seekable.[0]");

    int64_t stream_size = common::GetStreamSize(src_stream);
    if (stream_size < 0)
        throw std::runtime_error("LoadOnStream: input stream is not seekable.[1]");

    size_t dst_size;
    {
        DataLoader loader(src_stream, stream_size);
        dst_size = loader.GetSize();
    }

    src_stream.clear();
    if(!src_stream.seekg(0))
        throw std::runtime_error("LoadOnStream: input stream is not seekable.[2]");

    std::string result(dst_size, '\0');
    {
        DataLoader loader(src_stream, stream_size);
        loader.Uncompress(&(result[0]));
    }

    return result;
}

std::string LoadOnData(const char* data_txt)
{
    common::ByteArrayStream src_stream(
        const_cast<char*>(data_txt), strlen(data_txt));
    return LoadOnStream(src_stream);
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
