//sybie/common/Streaming.hh
//输入输出流相关功能

#ifndef INCLUDE_SYBIE_COMMON_STREAMING_HH
#define INCLUDE_SYBIE_COMMON_STREAMING_HH

#include "sybie/common/Streaming_fwd.hh"

#include <cstddef> //size_t
#include <cstdint> //int64_t
#include <streambuf> //std::streambuf
#include <istream> //std::istream
#include <ostream> //std::ostream
#include <string>
#include <memory> //std::unique_ptr

namespace sybie {
namespace common {

//获取一个输入流剩余可用数据量。
//返回-1表示流不可定长。
int64_t GetStreamSize(std::istream& is);

//在一个已有的char数组上读写的streambuf实现
class ByteArrayBuffer : public std::streambuf
{
public:
    ByteArrayBuffer(char* data, size_t size);
    //把string内部数据当作char数组（警告：在ByteArrayBuffer析构前不可修改str）
    explicit ByteArrayBuffer(std::string& str);
    ByteArrayBuffer(const ByteArrayBuffer& another) = delete;
    ByteArrayBuffer(ByteArrayBuffer&& another) = delete;
    virtual ~ByteArrayBuffer() throw();

    ByteArrayBuffer& operator=(const ByteArrayBuffer& another) = delete;
    ByteArrayBuffer& operator=(ByteArrayBuffer&& another) = delete;
public:
    char* Data();
    size_t Size() const;
private:
    virtual int underflow();
    virtual int overflow(int c = EOF);
    virtual std::streampos seekoff(std::streamoff off,
                                   std::ios_base::seekdir way,
                                   std::ios_base::openmode which);
    virtual std::streampos seekpos(std::streampos sp,
                                   std::ios_base::openmode which);
    static void _InitByteArrayBuffer(
        ByteArrayBuffer& instance, char* data, size_t size);
};

class ByteArrayStream : public std::iostream
{
public:
    ByteArrayStream(char* data, size_t size);
    //把string内部数据当作char数组（警告：在ByteArrayBuffer析构前不可修改str）
    explicit ByteArrayStream(std::string& str);
    ByteArrayStream(const ByteArrayStream& another) = delete;
    ByteArrayStream(ByteArrayStream&& another)  = delete;
    virtual ~ByteArrayStream() throw();

    ByteArrayStream& operator=(const ByteArrayStream& another) = delete;
    ByteArrayStream& operator=(ByteArrayStream&& another) = delete;
public:
    char* Data();
    size_t Size() const;
}; //class ByteArrayStream

//双向通讯的streambuf实现。
//允许一个istream和ostream关联一个Pipe对象，并各自一个线程同时读写。
//当写入完毕后，调用SetEof()，读取的线程获得EOF
class PipeBuffer : public std::streambuf
{
public:
    enum {DefaultBufferSize = 8192, DefaultBufferCount = 4};
public: //实例控制
    PipeBuffer(size_t buf_size = DefaultBufferSize,
               size_t buf_count = DefaultBufferCount);
    PipeBuffer(const PipeBuffer& another) = delete;
    PipeBuffer(PipeBuffer&& another) = delete;
    virtual ~PipeBuffer() throw();

    PipeBuffer& operator=(const PipeBuffer& another) = delete;
    PipeBuffer& operator=(PipeBuffer&& another) = delete;
public: //Pipe特有功能
    void PutEof(); //写端关闭，当缓冲队列清空后，read会返回EOF
    void GetEof(); //读端关闭，当下一次同步缓冲区时，write会返回EOF
    bool Skip(size_t bytes); //读端跳过n个字节，如果遇到EOF则返回false。
public: //返回streambuf内部的多个关键指针（警告：这是压榨性能的低层功能，谨慎使用）
    inline const char* pub_eback() const { return eback(); }
    inline const char* pub_gptr()  const { return gptr();  }
    inline const char* pub_egptr() const { return egptr(); }
    inline char* pub_pbase() { return pbase(); }
    inline char* pub_pptr()  { return pptr();  }
    inline char* pub_epptr() { return epptr(); }
private: //实现std::streambuf
    virtual int underflow();
    virtual int overflow(int c = EOF);
    virtual int sync();
private:
    void* _impl;
}; //class PipeBuffer

//PipeBuffer的读端，析构时会自动调用GetEof()
class PipeInputStream : public std::istream
{
public:
    explicit PipeInputStream(PipeBuffer* buffer);
    PipeInputStream(const PipeInputStream& another) = delete;
    PipeInputStream(PipeInputStream&& another) = delete;
    virtual ~PipeInputStream() throw();

    PipeInputStream& operator=(const PipeInputStream& another) = delete;
    PipeInputStream& operator=(PipeInputStream&& another) = delete;
public:
    bool Skip(size_t bytes); //跳过n个字节，如果遇到EOF则返回false。
    void GetEof(); //读端关闭，当下一次同步缓冲区时，write会返回EOF
}; //class PipeInputStream

//PipeBuffer的写端，析构时会自动调用PutEof()
class PipeOutputStream : public std::ostream
{
public:
    explicit PipeOutputStream(PipeBuffer* buffer);
    PipeOutputStream(const PipeOutputStream& another) = delete;
    PipeOutputStream(PipeOutputStream&& another) = delete;
    virtual ~PipeOutputStream() throw();

    PipeOutputStream& operator=(const PipeOutputStream& another) = delete;
    PipeOutputStream& operator=(PipeOutputStream&& another) = delete;
public:
    void PutEof(); //写端关闭，当缓冲队列清空后，read会返回EOF
}; //class PipeOutputStream

/* 创建并维护一个PipeBuffer，
 * 通过GetInputStream和GetOutputStream获取两个方向的Stream。
 */
class PipeStream
{
public:
    explicit PipeStream(size_t buf_size = PipeBuffer::DefaultBufferSize,
                        size_t buf_count = PipeBuffer::DefaultBufferCount);
    PipeStream(const PipeStream& another) = delete;
    PipeStream(PipeStream&& another) = delete;
    ~PipeStream() throw();

    PipeStream& operator=(const PipeStream& another) = delete;
    PipeStream& operator=(PipeStream&& another) = delete;
public:
    //获取读端。注意，在PipeInputStream析构前要保留PipeStream。
    std::unique_ptr<PipeInputStream> GetInputStream(); //return PipeInputStream(&GetBuffer());
    //获取写端。注意，在PipeOutputStream析构前要保留PipeStream。
    std::unique_ptr<PipeOutputStream> GetOutputStream(); //return PipeOutputStream(&GetBuffer());
    //直接访问内部的PipeBuffer（警告：除非明确知道这是什么，否则不要使用）
    PipeBuffer& GetBuffer();
private:
    void* _impl;
}; //class PipeWriter

}  //namespace common
}  //namespace sybie

#endif //ifndef INCLUDE_SYBIE_COMMON_STREAMING_HH
