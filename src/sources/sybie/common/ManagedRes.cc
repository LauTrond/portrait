//ManagedRes.cc

#include "sybie/common/ManagedRes.hh"

#include <cstdio>
#include <stdexcept> //std::runtime_error

namespace sybie {
namespace common {

TemporaryFile::TemporaryFile()
    : _filename(tmpnam(nullptr))
{ }

TemporaryFile::TemporaryFile(const std::string& filename)
    : _filename(filename)
{ }

TemporaryFile::~TemporaryFile() throw()
{
    if (_filename != "")
        remove(_filename.c_str()); //ignore error
}

TemporaryFile::TemporaryFile(TemporaryFile&& another) throw()
    : _filename(std::move(another._filename))
{ }

TemporaryFile& TemporaryFile::operator=(TemporaryFile&& another) throw()
{
    _filename = std::move(another._filename);
    return *this;
}

void TemporaryFile::Swap(TemporaryFile& another) throw()
{
    std::swap(_filename, another._filename);
}

std::string TemporaryFile::GetFilename() const
{
    return _filename;
}

void TemporaryFile::Remove()
{
    if (_filename == "")
        return;
    if (remove(_filename.c_str()))
        throw std::runtime_error((std::string)"Failed remove " + _filename);
    _filename = "";
}


} //namespace common
} //namespace sybie
