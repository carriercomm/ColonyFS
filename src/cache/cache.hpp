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
#include "../accessor.hpp"
#include "../synchronization.hpp"

#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include <tbb/spin_mutex.h>


namespace colony {


template <typename T, typename Policy >
class cache {

public:

  typedef cache<T, Policy>      this_type;

  typedef typename T::key_type  key_type;
  typedef T                     value_type;

  typedef typename basic_cache<T>::whole_type   whole_type;
  typedef typename basic_cache<T>::dirty_type   dirty_type;

  typedef colony::basic_cache<T>                cache_impl;

  typedef tbb::spin_mutex                       mutex_type;




  cache() {}




  const boost::shared_ptr<value_type>
  operator()(const key_type key) {

    using namespace boost::phoenix;
    using namespace boost::phoenix::arg_names;

    mutex_type& mutex = mutex_map_[key];
    mutex_type::scoped_lock lock( mutex );

    boost::shared_ptr<T> value =
        ValueFactory<T>::NewPointer(key, bind(&Policy::OnRead, policy_, arg1));

    try {

      cache_impl_.read(value);

    } catch (colony::cache_miss_e& e) {

      // Read and then insert. Not the other way around.
      policy_.PreRead(value);

      CV::Instance(value).lock();

      cache_impl_.insert(value);

    }

    return boost::shared_ptr<value_type>(value);
  }




  const boost::shared_ptr<value_type>
  operator[](const key_type key) {

    using namespace boost::phoenix;
    using namespace boost::phoenix::arg_names;

    mutex_type& mutex = mutex_map_[key];
    mutex_type::scoped_lock lock( mutex );

    boost::shared_ptr<T> value =
        ValueFactory<T>::NewPointer(key, bind(&Policy::OnWrite, policy_, arg1));

    try {
      cache_impl_.read(value);
    } catch (colony::cache_miss_e& e) {
      cache_impl_.insert(value);
    }

    cache_impl_.set_dirty(value);

    return boost::shared_ptr<value_type>(value);
  }




  void purge(const key_type key) {

    using namespace boost::phoenix;
    using namespace boost::phoenix::arg_names;

    boost::shared_ptr<T> value =
        ValueFactory<T>::NewPointer(key, bind(&Policy::OnRead, policy_, arg1));

   cache_impl_.erase(value);
   policy_.OnErase(value);
  }


  void purge() {
    cache_impl_.clear();
  }



  void invalidate(const key_type key) {

    using namespace boost::phoenix;
    using namespace boost::phoenix::arg_names;

    boost::shared_ptr<T> value =
        ValueFactory<T>::NewPointer(key, bind(&Policy::OnRead, policy_, arg1));

    try {
      cache_impl_.read_dirty(value);
      policy_.OnFlush(value);
      cache_impl_.dirty_.erase(key);
    } catch(colony::cache_miss_e& e) {}
  }



  void invalidate() {

    // FIXME: Concurrency issues!

    typename dirty_type::const_iterator it;
    for(it = cache_impl_.dirty_.begin(); it != cache_impl_.dirty_.end(); ++it) {

      shared_ptr<T> value = it->second.lock();

      BOOST_ASSERT(value.use_count() > 1);

      policy_.OnFlush(value);

    }

    cache_impl_.dirty_.clear();

    policy_.OnDone();

  }



  std::map<key_type, mutex_type>       mutex_map_;
  Policy                               policy_;
  colony::basic_cache<T>               cache_impl_;

};


} // Namespace


#endif /* CACHE_HPP_ */
