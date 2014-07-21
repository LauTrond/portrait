#include "sybie/datain/Stream.hh"

#include <cstdint>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <map>

#include "sybie/common/Streaming.hh"
#include "sybie/datain/Pool.hh"

namespace sybie {
namespace datain {

class PoolItemBuffer : public std::streambuf
{
public:
    PoolItemBuffer(Pool& pool, const char* data_id)
        : std::streambuf(),
          _pool(pool), _data_id(data_id), _index(-1),
          _end_pos(0), _parts(), _sizes(), _sum_sizes(), _pos_map()
    {
        _Init();
    }
private:
    Pool& _pool;
    const char* _data_id;
    int _index;

    int64_t _end_pos;
    std::vector<char*> _parts;
    std::vector<size_t> _sizes;
    std::vector<size_t> _sum_sizes; //index - sum_size
    std::map<size_t, int> _pos_map; //pos - index

    void _Init()
    {
        int part_index = 0;
        const char* part;
        while ((part = _pool.Get(_data_id, part_index++)) != nullptr)
            _AddPart(part);
        _sum_sizes.push_back(_end_pos);
        _pos_map.insert(
            std::make_pair(_end_pos, (int)_parts.size()));

        PoolItemBuffer::underflow();
    }

    void _AddPart(const char* data)
    {
        int64_t part_size = strlen(data);

        _sum_sizes.push_back(_end_pos);
        _pos_map.insert(
            std::make_pair(_end_pos,
                          (int)_parts.size()));
        _end_pos += part_size;
        _sizes.push_back(part_size);
        _parts.push_back(const_cast<char*>(data));
    }

    virtual int underflow()
    {
        if (_index < (int)_parts.size())
            _index++;

        if (_index < (int)_parts.size())
        {
            setg(_parts[_index], _parts[_index], _parts[_index] + _sizes[_index]);
            return (int)(eback()[0]);
        }
        else
        {
            setg(nullptr, nullptr, nullptr);
            return EOF;
        }
    }

    virtual std::streampos seekoff(std::streamoff off,
                                   std::ios_base::seekdir way,
                                   std::ios_base::openmode which)
    {
        if(!(which & std::ios_base::in))
            return -1;

        int64_t cur_pos = _sum_sizes[_index] + (gptr() - eback());
        int64_t pos;
        switch (way)
        {
        default : return -1;
        case std::ios_base::beg: pos = off; break;
        case std::ios_base::cur: pos = cur_pos + off; break;
        case std::ios_base::end: pos = _end_pos + off; break;
        }
        if (pos < 0 || pos > _end_pos)
            return -1;
        if (pos == _end_pos)
        {
            _index = _parts.size();
            setg(nullptr, nullptr, nullptr);
        }
        else
        {
            _index = _pos_map.upper_bound(pos)->second - 1;
            setg(_parts[_index],
                 _parts[_index] + (pos - _sum_sizes[_index]),
                 _parts[_index] + _sizes[_index]);
        }

        return pos;
    }

    virtual std::streampos seekpos(std::streampos sp,
                                   std::ios_base::openmode which)
    {
        return PoolItemBuffer::seekoff(sp, std::ios_base::beg, which);
    }
}; //class PoolItemBuffer

//class PoolItemStream

PoolItemStream::PoolItemStream(Pool& pool, const char* data_id)
    : std::istream(nullptr)
{
    rdbuf(new PoolItemBuffer(pool, data_id));
}

PoolItemStream::~PoolItemStream() throw()
{
    delete rdbuf();
}

//class StreamSink

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

//class StreamSource

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
