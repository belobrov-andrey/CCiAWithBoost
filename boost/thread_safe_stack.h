///
/// Copyright (c) Anthony Williams. 
///
//  Adapted to boost by Andrey Belobrov.
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef THREAD_SAFE_STACK_H
#define THREAD_SAFE_STACK_H

#include <stack>

#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

template<typename T>
class threadsafe_stack
{
public:
     threadsafe_stack()
     {
     }

     threadsafe_stack( const threadsafe_stack & other )
     {
          boost::lock_guard<boost::mutex> lk( other.mut );
          data_stack = other.data_stack;
     }

private:
     threadsafe_stack& operator=( const threadsafe_stack &); // Disable copy assignment

public:

     typedef T value_type;

     void push( T new_value )
     {
          boost::lock_guard<boost::mutex> lk( mut );
          data_stack.push( new_value );
          data_cond.notify_one();
     }

     void wait_and_pop( T& value )
     {
          boost::unique_lock<boost::mutex> lk( mut );
          data_cond.wait( lk, !boost::bind( &std::stack<T>::empty, boost::ref( data_stack ) ) );
          value = data_stack.top();
          data_stack.pop();
     }

     boost::shared_ptr<T> wait_and_pop()
     {
          boost::unique_lock<boost::mutex> lk( mut );
          data_cond.wait( lk, !boost::bind( &std::stack<T>::empty, boost::ref( data_stack ) ) );
          boost::shared_ptr<T> res( boost::make_shared<T>( data_stack.top() ) );
          data_stack.pop();
          return res;
     }

     bool try_pop( T& value )
     {
          boost::lock_guard<boost::mutex> lk( mut );
          if( data_stack.empty() )
          {
               return false;
          }
          value = data_stack.top();
          data_stack.pop();
          return true;
     }

     boost::shared_ptr<T> try_pop()
     {
          boost::lock_guard<boost::mutex> lk( mut );
          if( data_stack.empty() )
          {
               return boost::shared_ptr<T>();
          }
          boost::shared_ptr<T> res( boost::make_shared<T>( data_stack.top() ) );
          data_stack.pop();
          return res;
     }

     bool empty() const
     {
          boost::lock_guard<boost::mutex> lk( mut );
          return data_stack.empty();
     }
     
private:
     mutable boost::mutex mut;
     std::stack<T> data_stack;
     boost::condition_variable data_cond;
};

#endif // THREAD_SAFE_STACK_H
