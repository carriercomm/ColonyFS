/*
 * accessor.hpp
 *
 *  Created on: Sep 21, 2009
 *      Author: vjeko
 */

#ifndef ACCESSOR_HPP_
#define ACCESSOR_HPP_

#include <boost/asio/io_service.hpp>
#include <boost/thread.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include "xmlrpc/harmony.hpp"

#include <boost/timer.hpp>

#include <ctime>


class Timer {

public:

  static void Print() {

    struct timeval time;
    gettimeofday(&time, NULL);

    long seconds  = time.tv_sec;
    long useconds = time.tv_usec;

    std::cout << seconds << "." << useconds << std::endl;

  }


};




class DHT {

public:

  typedef colony::xmlrpc::harmony instance_type;

  static instance_type& Instance() {
    static instance_type dht("harmony-test");
    return dht;
  }

private:

  DHT();

};




class Client {

public:

  typedef colony::intercom::user_harmony instance_type;
  typedef boost::asio::io_service        io_service_type;
  typedef boost::asio::io_service::work  work_type;

  static io_service_type& IoService() {
    static io_service_type io_service;
    static work_type work(io_service);

    return io_service;
  }

  static instance_type& Instance() {
    static instance_type client(IoService(), "harmony-test");
    return client;
  }

  static void Thread() {

	  boost::thread runner1(
		  boost::phoenix::bind(
				  &boost::asio::io_service::run,
				  boost::phoenix::ref(IoService())
		  )
	  );

	  boost::thread runner2(
		  boost::phoenix::bind(
				  &boost::asio::io_service::run,
				  boost::phoenix::ref(IoService())
		  )
	  );

	  boost::thread runner3(
		  boost::phoenix::bind(
				  &boost::asio::io_service::run,
				  boost::phoenix::ref(IoService())
		  )
	  );

  }

private:

  Client();

};



#endif /* ACCESSOR_HPP_ */
