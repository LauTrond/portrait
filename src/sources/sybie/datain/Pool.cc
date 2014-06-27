//Pool.cc
//这是对Pool.hh的实现

#include "sybie/datain/Pool.hh"

#include <map>
#include <string>
#include <iostream>

namespace sybie {
namespace datain {

typedef std::map<std::string, const char*> PoolMap;

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

void Pool::Set(const char* data_id, const char* data)
{
#ifdef _DEBUG
    std::cout<<data_id<<" imported."<<std::endl;
#endif
    (*(PoolMap*)_impl)[data_id] = data;
}

const char* Pool::Get(const char* data_id) const
{
    return (*(PoolMap*)_impl).at(data_id);
}

} //namespace datain
} //namespace sybie
