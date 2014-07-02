//Streaming.cc
#include "sybie/common/Streaming.hh"

//#include <cassert> //assert(...)
#include <memory> //std::unique_ptr
#include <queue> //std::queue
#include <utility> //std::move
#include <mutex> //std::lock_guard std::mutex
#include <istream>
#include <ostream>

#include "sybie/common/RichAssert.hh"
#include "sybie/common/Uncopyable.hh"
#include "sybie/common/Event.hh"

namespace sybie {
namespace common {

//non-class functions

int64_t GetStreamSize(std::istream& is)
{
    std::streampos cur = is.tellg();
    is.seekg(0, std::ios_base::end);
    if (is.fail())
        return 0;
    std::streampos result = is.tellg() - cur;
    is.seekg(cur);
    return result;
}

//classes

//class ByteArrayStream

ByteArrayBuffer::ByteArrayBuffer(char* data, size_t size)
    : std::streambuf()
{
    _InitByteArrayBuffer(*this, data, size);
}

ByteArrayBuffer::ByteArrayBuffer(std::string& str)
    : std::streambuf()
{
    _InitByteArrayBuffer(*this, &str[0], str.size());
}

ByteArrayBuffer::~ByteArrayBuffer() throw()
{ }

char* ByteArrayBuffer::Data()
{
    return eback();
}

size_t ByteArrayBuffer::Size() const
{
    return egptr() - eback();
}

int ByteArrayBuffer::underflow()
{
    return EOF;
}

int ByteArrayBuffer::overflow(int c)
{
    return EOF;
}

std::streampos ByteArrayBuffer::seekoff(std::streamoff off,
                                        std::ios_base::seekdir way,
                                        std::ios_base::openmode which)
{
    std::streampos result = -1;

    char* base = eback();
    char* end = egptr();

    char* g_cur = gptr();
    char* p_cur = pptr();

    switch (way)
    {
    default : return -1;
    case std::ios_base::beg: g_cur = p_cur = base + off; break;
    case std::ios_base::cur: g_cur = g_cur + off; p_cur = p_cur + off; break;
    case std::ios_base::end: g_cur = p_cur = end + off; break;
    }

    if (which & std::ios_base::in)
    {
        setg(base, g_cur, end);
        result = g_cur - base;
    }

    if (which & std::ios_base::out)
    {
        setp(p_cur, end);
        result = p_cur - base;
    }

    return result;
}

std::streampos ByteArrayBuffer::seekpos(std::streampos sp,
                                        std::ios_base::openmode which)
{
    return ByteArrayBuffer::seekoff(sp, std::ios_base::beg, which);
}

void ByteArrayBuffer::_InitByteArrayBuffer(
    ByteArrayBuffer& instance, char* data, size_t size)
{
    instance.setg(data, data, data + size);
    instance.setp(data, data + size);
}

//class ByteArrayStream

ByteArrayStream::ByteArrayStream(char* data, size_t size)
    : std::iostream(new ByteArrayBuffer(data, size))
{ }

ByteArrayStream::ByteArrayStream(std::string& str)
    : std::iostream(new ByteArrayBuffer(str))
{ }

ByteArrayStream::~ByteArrayStream() throw()
{
    delete rdbuf();
}

char* ByteArrayStream::Data()
{
    return static_cast<ByteArrayBuffer*>(rdbuf())->Data();
}

size_t ByteArrayStream::Size() const
{
    return static_cast<ByteArrayBuffer*>(rdbuf())->Size();
}

class PipeBufferImpl : Uncopyable
{
public:
    struct AtomicBool : Uncopyable
    {
    public:
        AtomicBool(bool val) : _val(val) { }
        AtomicBool& operator=(bool val)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _val = val;
            return *this;
        }
        operator bool()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            return _val;
        }
    private:
        bool _val;
        std::mutex _mutex;
    }; //struct AtomicBool
public:
    struct BufferItem : Uncopyable
    {
    public:
        BufferItem() //eof
            : _size(0), _data(nullptr)
        { }

        explicit BufferItem(size_t size)
            : _size(size), _data(new char[size])
        { }

        ~BufferItem() throw()
        {
            delete[] _data;
        }

        inline size_t Size() const { return _size; }

        inline void Resize(const size_t size)
        {
            sybie_assert(size <= _size);
            _size = size;
        }

        inline char* Data() { return _data; }

        inline const char* Data() const { return _data; }

        inline bool Eof() { return _data == nullptr; }

    private:
        size_t _size;
        char* _data;
    }; //struct BufferItem
public:
    PipeBufferImpl(size_t buf_size, size_t buf_count)
        : _buf_size(buf_size), _queue_size(buf_count),
          _get_eof(false), _put_eof(false),
          _gbuf(), _pbuf(), _buf_queue(),
          _mutex(), _put_event(), _get_event()
    { }

    ~PipeBufferImpl() throw()
    { }

    BufferItem& PutSync(char* pptr)
    {
        if (_get_eof)
            PutEof(pptr);

        if (!_put_eof)
        {
            _TrySync(pptr);
            _NewPutBuf();
        }
        else
        {
            _NewPutEofBuf();
        }
        return *_pbuf;
    }

    void PutEof(char* pptr)
    {
        if (!_put_eof)
        {
            _TrySync(pptr);
            _PutEof();
            _put_eof = true;
        }
    }

    BufferItem& GetNext()
    {
        if (!_get_eof)
        {
            _gbuf = _Pop();
            if (_gbuf->Eof()) //put eof
                _get_eof = true;
        }
        else
        {
            _NewGetEofBuf();
        }
        return *_gbuf;
    }

    void GetEof()
    {
        _get_eof = true;
        _get_event.SetEvent();
    }
private:
    size_t _buf_size;
    size_t _queue_size;
    AtomicBool _get_eof, _put_eof;

    std::unique_ptr<BufferItem> _gbuf, _pbuf;
    std::queue<std::unique_ptr<BufferItem>> _buf_queue;

    mutable std::mutex _mutex;
    mutable common::Event _put_event, _get_event;

private:
    void _TrySync(char* pptr)
    {
        if (_pbuf && !_pbuf->Eof())
            _Push(std::move(_pbuf), pptr);
    }

    void _PutEof()
    {
        _Push(std::unique_ptr<BufferItem>(new BufferItem()), nullptr);
    }

    void _NewPutBuf()
    {
        _pbuf.reset(new BufferItem(_buf_size));
    }

    void _NewPutEofBuf()
    {
        _pbuf.reset(new BufferItem());
    }

    void _NewGetEofBuf()
    {
        _gbuf.reset(new BufferItem());
    }

private:
    void _Push(std::unique_ptr<BufferItem>&& buf, char* pptr)
    {
        while (true) //queue over length, wait for get
        {
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (_get_eof || _buf_queue.size() < _queue_size)
                    break;
            }
            _get_event.Wait();
        }

        if (!buf->Eof())
        {
            sybie_assert(pptr >= buf->Data() && pptr <= buf->Data() + buf->Size())
                << "pptr=" << (size_t)pptr << "\n"
                << "Data()=" << (size_t)buf->Data() << "\n"
                << "Data()+Size()=" << (size_t)buf->Data() + buf->Size() << "\n";
            buf->Resize(pptr - buf->Data());
        }

        {
            std::lock_guard<std::mutex> lock(_mutex);
            _buf_queue.push(std::move(buf));
        }

        _put_event.SetEvent();
    }

    std::unique_ptr<BufferItem> _Pop()
    {
        while (true)
        {
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (_buf_queue.size() > 0)
                    break;
            }
            _put_event.Wait();
        }

        std::unique_ptr<BufferItem> result;

        {
            std::lock_guard<std::mutex> lock(_mutex);
            result = std::move(_buf_queue.front());
            _buf_queue.pop();
        }

        _get_event.SetEvent();
        return result;
    }
}; //class PipeBufferImpl

//class PipeBuffer

PipeBuffer::PipeBuffer(size_t buf_size, size_t buf_count)
    : std::streambuf(), _impl(new PipeBufferImpl(buf_size, buf_count))
{ }

PipeBuffer::~PipeBuffer() throw()
{
    delete (PipeBufferImpl*)_impl;
}

void PipeBuffer::PutEof()
{
    overflow(EOF);
}

void PipeBuffer::GetEof()
{
    ((PipeBufferImpl*)_impl)->GetEof();
}

bool PipeBuffer::Skip(size_t bytes)
{
    if (gptr() == nullptr)
        PipeBuffer::underflow();
    while (true)
    {
        if (gptr() == nullptr) //EOF
            return false;

        if (egptr() - gptr() > bytes)
        {
            setg(gptr(), gptr() + bytes, egptr());
            bytes = 0;
            break;
        }

        bytes -= egptr() - gptr();
        PipeBuffer::underflow();
    }
    return true;
}

int PipeBuffer::underflow()
{
    PipeBufferImpl::BufferItem& new_get_buffer = ((PipeBufferImpl*)_impl)->GetNext();
    if (new_get_buffer.Eof())
    {
        setg(nullptr, nullptr, nullptr);
        return EOF;
    }
    else
    {
        setg(new_get_buffer.Data(),
             new_get_buffer.Data(),
             new_get_buffer.Data() + new_get_buffer.Size());
        return (int)(unsigned char)(new_get_buffer.Data()[0]);
    }
}

int PipeBuffer::overflow(int c)
{
    if (c == EOF)
    {
        ((PipeBufferImpl*)_impl)->PutEof(pptr());
        setp(nullptr, nullptr);
    }
    else
    {
        PipeBufferImpl::BufferItem& new_put_buffer =
            ((PipeBufferImpl*)_impl)->PutSync(pptr());
        if (!new_put_buffer.Eof())
        {
            new_put_buffer.Data()[0] = (char)c;
            setp(new_put_buffer.Data() + 1,
                 new_put_buffer.Data() + new_put_buffer.Size());
        }
        else
        {
            setp(nullptr, nullptr);
            return EOF;
        }
    }
    return c;
}

int PipeBuffer::sync()
{
    PipeBufferImpl::BufferItem& new_put_buffer =
        ((PipeBufferImpl*)_impl)->PutSync(pptr());
    setp(new_put_buffer.Data(),
         new_put_buffer.Data() + new_put_buffer.Size());
    return 0;
}

//class PipeInputStream

PipeInputStream::PipeInputStream(PipeBuffer* buffer)
    : std::istream(buffer)
{ }

PipeInputStream::~PipeInputStream() throw()
{
    if (rdbuf())
        GetEof();
}

bool PipeInputStream::Skip(size_t bytes)
{
    return static_cast<PipeBuffer*>(rdbuf())->Skip(bytes);
}

void PipeInputStream::GetEof()
{
    sybie_assert(rdbuf() != nullptr);
    static_cast<PipeBuffer*>(rdbuf())->GetEof();
}

//class PipeOutputStream

PipeOutputStream::PipeOutputStream(PipeBuffer* buffer)
    : std::ostream(buffer)
{ }

PipeOutputStream::~PipeOutputStream() throw()
{
    if (rdbuf())
        PutEof();
}

void PipeOutputStream::PutEof()
{
    sybie_assert(rdbuf() != nullptr);
    static_cast<PipeBuffer*>(rdbuf())->PutEof();
}

//class PipeStream

PipeStream::PipeStream(size_t buf_size, size_t buf_count)
    : _impl(new PipeBuffer(buf_size, buf_count))
{ }

PipeStream::~PipeStream() throw()
{
    delete (PipeBuffer*)_impl;
}

std::unique_ptr<PipeInputStream> PipeStream::GetInputStream()
{
    return std::unique_ptr<PipeInputStream>(
        new PipeInputStream((PipeBuffer*)_impl));
}

std::unique_ptr<PipeOutputStream> PipeStream::GetOutputStream()
{
    return std::unique_ptr<PipeOutputStream>(
        new PipeOutputStream((PipeBuffer*)_impl));
}

PipeBuffer& PipeStream::GetBuffer()
{
    return *(PipeBuffer*)_impl;
}

}  //namespace common
}  //namespace sybie
