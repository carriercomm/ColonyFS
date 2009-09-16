/*
 * cache.hpp
 *
 *  Created on: Sep 14, 2009
 *      Author: vjeko
 */

#ifndef CACHE_HPP_
#define CACHE_HPP_

#include "basic_cache.hpp"

#include "../xmlrpc/harmony.hpp"

#include "../intercom/user_harmony.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/asio/io_service.hpp>


template <typename T>
struct DataDeleter {

  DataDeleter() {}

  void OnRead(T* p) {

    std::cout << "\t MetadataDeleter OnRead Called!" << std::endl;

  }

  void OnWrite(T* p) {

    std::cout << "\tMetadataDeleter OnWrite Called!" << std::endl;

    //boost::shared_ptr<T> tmp(p);

    //accessor_.deposit_chunk("codered", p);
    //io_service_.run();
    //io_service_.reset();

  }

  void OnFlush(T* p) {

    std::cout << "\tMetadataDeleter OnFlush Called!" << std::endl;

  }

  //TODO: IO SERVICE
  //colony::intercom::user_harmony accessor_;

};

template <typename T>
struct MetadataDeleter {

  MetadataDeleter() : accessor_("harmony-test") {}

  void OnRead(T* p) {

    std::cout << "\t MetadataDeleter OnRead Called!" << std::endl;

  }

  void OnWrite(T* p) {

    std::cout << "\tMetadataDeleter OnWrite Called!" << std::endl;

    accessor_.set_pair(*p);

  }

  void OnFlush(T* p) {

    std::cout << "\tMetadataDeleter OnFlush Called!" << std::endl;

  }


  colony::xmlrpc::harmony accessor_;

};

namespace colony {


template <typename T, typename D >
class cache : private basic_cache<T> {

public:

  typedef typename T::key_type  key_type;
  typedef T                     value_type;

  cache() {}

  const boost::shared_ptr<value_type>
  operator()(const key_type key) {

    boost::shared_ptr<T> value(this->read(key));
    boost::shared_ptr<T>
    dummy(
        value.get(),
        boost::bind(&D::OnRead, deleter_, _1)
    );

    return dummy;

  }

  const boost::shared_ptr<value_type>
  operator[](const key_type key) {

    boost::shared_ptr<T> value(this->mutate(key));
    boost::shared_ptr<T>
    dummy(
        value.get(),
        boost::bind(&D::OnWrite, deleter_, _1)
    );

    return dummy;

  }

  D deleter_;

};


} // Namespace


#endif /* CACHE_HPP_ */