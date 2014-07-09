//Stream.hh

#ifndef INCLUDE_SYBIE_DATAIN_STREAM_HH
#define INCLUDE_SYBIE_DATAIN_STREAM_HH

#include "sybie/datain/Stream_fwd.hh"

#include <cstddef>
#include <iostream>

#include "sybie/common/Streaming_fwd.hh"
#include "sybie/datain/Pool_fwd.hh"

#include "snappy-sinksource.h"

namespace sybie {
namespace datain {

class PoolItemStream : public std::istream
{
public:
    PoolItemStream(Pool& pool, const char* data_id);
    PoolItemStream(const PoolItemStream& another) = delete;
    PoolItemStream(PoolItemStream&& another) = delete;
    virtual ~PoolItemStream() throw();

    PoolItemStream& operator=(const PoolItemStream& another) = delete;
    PoolItemStream& operator=(PoolItemStream&& another) = delete;
}; //class PoolItemStream

class StreamSink : public snappy::Sink
{
public:
    explicit StreamSink(std::ostream& os);
    virtual ~StreamSink();
public:
    virtual void Append(const char* bytes, size_t n);
private:
    std::ostream& _os;
}; //class StreamSink

class StreamSource : public snappy::Source
{
public:
    enum { BufferSize = 8192 };
public:
    explicit StreamSource(std::istream& is, size_t bytes);
    virtual ~StreamSource();
public:
    virtual size_t Available() const;
    virtual const char* Peek(size_t* len);
    virtual void Skip(size_t n);
private:
    std::istream& _is;
    size_t _left;

    char* _buf;
    char* _buf_current;
    char* _buf_end;

    void Next();
}; //class StreamSource

} //namespace datain
} //namespace sybie

#endif //ifndef
