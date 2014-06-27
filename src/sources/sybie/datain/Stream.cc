#include "sybie/datain/Stream.hh"

#include <cassert>
#include <stdexcept>

#include "sybie/common/Streaming.hh"

namespace sybie {
namespace datain {

StreamSink::StreamSink(std::ostream& os)
    : _os(os)
{ }

StreamSink::~StreamSink()
{ }

void StreamSink::Append(const char* bytes, size_t n)
{
    _os.write(bytes, n);
    if (_os.eof())
        throw std::runtime_error("StreamSink::Append: EOF.");
    else if (_os.fail())
        throw std::runtime_error("StreamSink::Append: Failed write ostream.");
}

StreamSource::StreamSource(std::istream& is, size_t bytes)
    : _is(is), _left(bytes),
      _buf(new char[BufferSize]), _buf_current(nullptr), _buf_end(nullptr)
{ }

StreamSource::~StreamSource()
{
    delete[] _buf;
}

size_t StreamSource::Available() const
{
    return _left;
}

const char* StreamSource::Peek(size_t* len)
{
    if (!_buf_current)
        Next();

    *len = _buf_end - _buf_current;
    return _buf_current;
}

void StreamSource::Skip(size_t n)
{
    assert(n <= _left);
    _left -= n;
    while (_buf_end > _buf) //_buf_end == _buf while eof
    {
        if (_buf_end - _buf_current > n)
        {
            _buf_current += n;
            n = 0;
            break;
        }

        n -= _buf_end - _buf_current;
        Next();
    }

    if (n > 0)
        throw std::runtime_error("StreamSource::Skip: Unexpected EOF.");
}

void StreamSource::Next()
{
    _is.read(_buf, BufferSize);
    if (!_is.eof() && _is.fail())
        throw std::runtime_error("StreamSink::Append: Failed write ostream.");
    size_t read_size = _is.gcount();
    _buf_current = _buf;
    _buf_end = _buf + read_size;
}

}  //namespace datain
}  //namespace sybie
