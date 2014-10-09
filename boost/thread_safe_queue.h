///
/// Copyright (c) Anthony Williams. 
///
//  Adapted to boost by Andrey Belobrov.
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <queue>

#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>

template<typename T>
class threadsafe_queue
{
public:
     threadsafe_queue()
     {
     }

     threadsafe_queue( const threadsafe_queue& other )
     {
          boost::lock_guard<boost::mutex> lk( other.mut );
          data_queue = other.data_queue;
     }

private:
     threadsafe_queue& operator=( const threadsafe_queue &); // Disable copy assignment

public:

     typedef T value_type;

     void push( T new_value )
     {
          boost::lock_guard<boost::mutex> lk( mut );
          data_queue.push( new_value );
          data_cond.notify_one();
     }

     void wait_and_pop( T& value )
     {
          boost::unique_lock<boost::mutex> lk( mut );
          data_cond.wait( lk, !boost::bind( &std::queue<T>::empty, boost::ref( data_queue ) );
          value = data_queue.front();
          data_queue.pop();
     }

     boost::shared_ptr<T> wait_and_pop()
     {
          boost::unique_lock<boost::mutex> lk( mut );
          data_cond.wait( lk, !boost::bind( &std::queue<T>::empty, boost::ref( data_queue ) ) );
          boost::shared_ptr<T> res( boost::make_shared<T>( data_queue.front() ) );
          data_queue.pop();
          return res;
     }

     bool try_pop( T& value )
     {
          boost::lock_guard<boost::mutex> lk( mut );
          if( data_queue.empty() )
          {
               return false;
          }
          value = data_queue.front();
          data_queue.pop();
          return true;
     }

     boost::shared_ptr<T> try_pop()
     {
          boost::lock_guard<boost::mutex> lk( mut );
          if( data_queue.empty() )
          {
               return boost::shared_ptr<T>();
          }
          boost::shared_ptr<T> res( boost::make_shared<T>( data_queue.front() ) );
          data_queue.pop();
          return res;
     }

     bool empty() const
     {
          boost::lock_guard<boost::mutex> lk( mut );
          return data_queue.empty();
     }
private:
     mutable boost::mutex mut;
     std::queue<T> data_queue;
     boost::condition_variable data_cond;
};

#endif // ifndef THREAD_SAFE_QUEUE_H
