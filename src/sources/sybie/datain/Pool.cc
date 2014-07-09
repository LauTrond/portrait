//Pool.cc
//这是对Pool.hh的实现

#include "sybie/datain/Pool.hh"

#include <map>
#include <string>

namespace sybie {
namespace datain {

typedef std::map<std::string, std::map<int, const char*>> PoolMap;

Pool& Pool::GetGlobalPool()
{
    static Pool global_pool;
    return global_pool;
}

Pool::Pool()
    : _impl(new PoolMap())
{ }

Pool::~Pool()
{
    delete (PoolMap*)_impl;
}

void Pool::Set(const char* data_id, const int index, const char* data)
{
    (*(PoolMap*)_impl)[data_id][index] = data;
}

const char* Pool::Get(const char* data_id, const int index) const
{
    auto data_all = ((PoolMap*)_impl)->at(data_id);
    if (data_all.count(index) > 0)
        return data_all.at(index);
    else
        return nullptr;
}

} //namespace datain
} //namespace sybie
