/*
 * aggregator.hpp
 *
 *  Created on: May 16, 2009
 *      Author: vjeko
 */

#ifndef AGGREGATOR_HPP_
#define AGGREGATOR_HPP_

#include "../debug.hpp"
#include "../synchronization.hpp"

#include "../xmlrpc/values.hpp"
#include "../xmlrpc/attribute.hpp"
#include "../xmlrpc/harmony.hpp"

#include "../parsers/user_parser.hpp"

#include "../cache/cache.hpp"

#include "../storage/chunk_data.hpp"
#include "../storage/chunk_metadata.hpp"

#include "../intercom/user_harmony.hpp"

#include "../cache/policy/DataOnFlush.hpp"
#include "../cache/policy/MetadataOnFlush.hpp"

#include <algorithm>
#include <sys/stat.h>

#include <boost/asio/io_service.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/exception.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace colony {

typedef boost::tuple<
      boost::asio::io_service,
      colony::intercom::user_harmony,
      colony::xmlrpc::harmony
    > bridge_type;


typedef boost::error_info<struct metadata_error, std::string> key_info_t;

class resource_error : public boost::exception {};


template<typename T>
class sink_local_impl {

public:

  typedef typename T::key_type           key_type;
  typedef typename T::value_type         value_type;
  typedef typename T::mapped_type        mapped_type;

  sink_local_impl() {};

  virtual ~sink_local_impl() {};

  shared_ptr<T> operator[](const key_type& key) {
    return cache_[key];
  }

  shared_ptr<T> operator()(const key_type& key) {
    return cache_(key);
  }

  inline void purge(const key_type& key) {
    cache_.purge(key);
  }

  inline void purge() {
    cache_.purge();
  }

  inline void invalidate() {
    cache_.invalidate();
  }

  inline void invalidate(const key_type key) {
    cache_.invalidate(key);
  }



private:

  cache<T, MetadataOnFlush<T> >       cache_;
};





template<
  typename T,
  template <typename T> class Implementation = sink_local_impl
> class aggregator {

public:

  typedef typename T::key_type           key_type;
  typedef typename T::value_type         value_type;
  typedef typename T::mapped_type        mapped_type;




  aggregator() {};

  virtual ~aggregator() {};

  shared_ptr<T> operator[](const key_type key) {
    return implementation_[key];
  }

  shared_ptr<T> operator()(const key_type key) {
    return implementation_(key);
  }

  inline void commit(shared_ptr<T> pair) {
    implementation_.commit(pair);
  }

  inline void purge(const key_type& key) {
    implementation_.purge(key);
  }

  inline void purge() {
    implementation_.purge();
  }

  inline void invalidate() {
    implementation_.invalidate();
  }


  inline void invalidate(const key_type key) {
    implementation_.invalidate(key);
  }




private:

  Implementation<T>                   implementation_;
};



// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Data
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++



template<> class aggregator<colony::storage::chunk_data> {

public:

  typedef colony::storage::chunk_data     value_type;


  aggregator() :
      sink_log_( RLOG_CHANNEL( "sink/data" ) ) {

    try {

    } catch (colony::xmlrpc::key_missing_error& e) {
      rError("no chunkservers found...");
    }

  }


  void write(
      const boost::filesystem::path filepath,
      const char* buffer,
      const size_t size,
      const size_t offset) {

    using colony::storage::chunk_data;

    using namespace boost::phoenix::arg_names;

    copy(
        filepath,
        buffer,
        boost::phoenix::bind< shared_ptr<chunk_data> >(&aggregator::create_if_missing, this, arg1, arg2),
        boost::phoenix::bind(&aggregator::write_to_chunk, this, arg1, arg2, arg3, arg4, arg5),
        size,
        offset);

  }




  void read(
      const boost::filesystem::path filepath,
      char* buffer,
      const size_t size,
      const size_t offset) {

    using colony::storage::chunk_data;

    using namespace boost::phoenix::arg_names;

    copy(
        filepath,
        buffer,
        boost::phoenix::bind< shared_ptr<chunk_data> >(&aggregator::raise_if_missing, this, arg1, arg2),
        boost::phoenix::bind(&aggregator::read_from_chunk, this, arg1, arg2, arg3, arg4, arg5),
        size,
        offset);

  }




  void truncate(
      const boost::filesystem::path filepath,
      const size_t new_size,
      const size_t old_size) {

    using colony::storage::chunk_data;


    const size_t chunk_index_start = new_size / CHUNK_SIZE;
    const size_t chunk_index_end = old_size / CHUNK_SIZE;

    shared_ptr<chunk_data> chunk = raise_if_missing(filepath, chunk_index_start);

    const size_t chunk_size = new_size % CHUNK_SIZE;
    chunk_data::data_type& chunk_buffer = *(chunk->data_ptr_);
    chunk_buffer.resize(chunk_size);

    for (size_t count = chunk_index_start + 1; count <= chunk_index_end; count++) {
      const std::string key = filepath.string() + boost::lexical_cast<std::string>(count);
      // TODO: Erase the chunk!
    }

  }




  void rename(
      const boost::filesystem::path oldpath,
      const boost::filesystem::path newpath,
      size_t total_size) {

    using colony::storage::chunk_data;

    const size_t chunk_index_start = 0;
    const size_t chunk_index_end = total_size / CHUNK_SIZE;

    for (size_t count = chunk_index_start; count <= chunk_index_end; count++) {
      const std::string old_key = oldpath.string() + boost::lexical_cast<std::string>(count);
      const std::string new_key = newpath.string() + boost::lexical_cast<std::string>(count);
      //implementation_[new_key] = implementation_[old_key];
      //implementation_.erase(old_key);
    }

  }




  void erase(const boost::filesystem::path filepath, size_t total_size) {

    using colony::storage::chunk_data;

    const size_t chunk_index_start = 0;
    const size_t chunk_index_end = total_size / CHUNK_SIZE;

    for (size_t count = chunk_index_start; count <= chunk_index_end; count++) {
      const std::string key = filepath.string() + boost::lexical_cast<std::string>(count);
      // TODO: Erase the chunk!
    }

  }


  void purge(std::string filename) {
    BOOST_FOREACH(int i, index_map_[filename]) {
      storage::chunk_data::key_type key = boost::make_tuple(filename, i);
      cache_.purge(key);
    }
  }


  void invalidate(std::string filename) {
    BOOST_FOREACH(int i, index_map_[filename]) {
      storage::chunk_data::key_type key = boost::make_tuple(filename, i);
      cache_.invalidate(key);
    }
  }


  void invalidate() {
    cache_.invalidate();
  }


  void purge() {
    cache_.purge();
  }



private:




  template<
    typename Buffer,
    typename MissingKeyPolicy,
    typename ActionPolicy
  > void copy(
      const boost::filesystem::path filepath,
      Buffer buffer,
      MissingKeyPolicy key_policy,
      ActionPolicy action_policy,
      const size_t size,
      size_t offset) {

    using colony::storage::chunk_data;

    size_t buffer_start = 0;
    size_t buffer_end = size;

    const size_t chunk_index_start = offset / CHUNK_SIZE;
    const size_t chunk_index_end = (size + offset) / CHUNK_SIZE;

    rLog(sink_log_, "Buffer Offset: %zu", offset);
    rLog(sink_log_, "Buffer Size: %zu", size);
    rLog(sink_log_, "Chunk Index Start: %zu", chunk_index_start);
    rLog(sink_log_, "Chunk Index End: %zu", chunk_index_end);

    for (size_t count = chunk_index_start; count <= chunk_index_end; count++) {

      const size_t remaining_size = buffer_end - buffer_start;
      const size_t chunk_start = offset % CHUNK_SIZE;
      const size_t chunk_end = ((buffer_start + remaining_size) > CHUNK_SIZE) ? CHUNK_SIZE : chunk_start + remaining_size;
      const size_t chunk_delta = chunk_end - chunk_start;

      rLog(sink_log_, "Chunk Start: %zu", chunk_start);
      rLog(sink_log_, "Chunk End: %zu", chunk_end);
      rLog(sink_log_, "Remaining Size: %zu", remaining_size);
      rLog(sink_log_, "Chunk Delta: %zu", chunk_delta);

      shared_ptr<chunk_data> chunk = key_policy(filepath, count);

      action_policy(chunk, buffer, chunk_start, buffer_start, chunk_delta);

      buffer_start += chunk_delta;
      offset = 0;
    }

  }




  void write_to_chunk(
      shared_ptr<colony::storage::chunk_data> destination,
      const char* source,
      size_t destination_offset,
      size_t source_offset,
      size_t chunk_delta
      ) {

    colony::storage::chunk_data::data_type& chunk_buffer = *(destination->data_ptr_);

    const size_t required_size = destination_offset + chunk_delta;
    if (chunk_buffer.size() < required_size) chunk_buffer.resize(required_size);

    memcpy(&chunk_buffer[destination_offset], source + source_offset, chunk_delta);

  }




  void read_from_chunk(
      shared_ptr<colony::storage::chunk_data> source,
      char* destination,
      size_t source_offset,
      size_t destination_offset,
      size_t chunk_delta
      ) {

    BOOST_ASSERT(source->data_ptr_->size() >= chunk_delta);

    colony::storage::chunk_data::data_type& chunk_buffer = *(source->data_ptr_);
    memcpy(destination + destination_offset, &chunk_buffer[source_offset], chunk_delta);

  }




  shared_ptr<colony::storage::chunk_data> create_if_missing(
      const boost::filesystem::path filepath,
      const size_t count) {

    colony::storage::basic_metadata::key_type key(filepath.string(), count);

    shared_ptr<colony::storage::chunk_data> result = cache_[key];
    index_map_[result->uid_].insert(result->cuid_);

    return result;

  }




  shared_ptr<colony::storage::chunk_data> raise_if_missing(
      const boost::filesystem::path filepath,
      const size_t count) {

    colony::storage::basic_metadata::key_type key(filepath.string(), count);

    return cache_(key);;

  }



  boost::unordered_map<std::string, std::set<int> >                     index_map_;
  cache<
    value_type,
    DataOnFlush<value_type>
  >                                              cache_;
  rlog::RLogChannel                             *sink_log_;
};




} // Namespace.
#endif /* AGGREGATOR_HPP_ */
