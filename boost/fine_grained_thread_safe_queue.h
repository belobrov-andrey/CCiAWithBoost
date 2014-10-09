///
/// Copyright (c) Anthony Williams. 
///
//  Adapted to boost by Andrey Belobrov.
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef FINE_GRAINED_THREAD_SAFE_QUEUE
#define FINE_GRAINED_THREAD_SAFE_QUEUE

#include <memory>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/bind/bind.hpp>

template<typename T>
class threadsafe_queue : private boost::noncopyable
{
private:
     struct node
     {
          boost::shared_ptr<T> data;
          std::auto_ptr< node > next;
     };

     boost::mutex head_mutex;
     std::auto_ptr< node > head;
     boost::mutex tail_mutex;
     node* tail;
     boost::condition_variable data_cond;
public:

     typedef T value_type;

     threadsafe_queue() :
          head( new node ),
          tail( head.get() )
     {
     }

     boost::shared_ptr<T> try_pop();
     bool try_pop( T& value );
     boost::shared_ptr<T> wait_and_pop();
     void wait_and_pop( T& value );
     void push( T new_value );
     bool empty();
private:
     node* get_tail()
     {
          boost::lock_guard<boost::mutex> tail_lock( tail_mutex );
          return tail;
     }

     std::auto_ptr<node> pop_head()
     {
          std::auto_ptr<node> old_head = head;
          head = old_head->next;
          return old_head;
     }

     bool empty_impl()
     {
          return head.get() == get_tail();
     }

     boost::unique_lock<boost::mutex> wait_for_data()
     {
          boost::unique_lock<boost::mutex> head_lock( head_mutex );

          data_cond.wait( head_lock, !boost::bind( &threadsafe_queue<T>::empty_impl, this ) );

          return boost::move( head_lock );
     }

     std::auto_ptr<node> wait_pop_head()
     {
          boost::unique_lock<boost::mutex> head_lock( wait_for_data() );
          return pop_head();
     }

     std::auto_ptr<node> wait_pop_head( T& value )
     {
          boost::unique_lock<boost::mutex> head_lock( wait_for_data() );
          value = *head->data;
          return pop_head();
     }

     std::auto_ptr<node> try_pop_head()
     {
          boost::lock_guard< boost::mutex > head_lock( head_mutex );
          if( head.get() == get_tail() )
          {
               return std::auto_ptr<node>();
          }
          return pop_head();
     }

     std::auto_ptr<node> try_pop_head( T& value )
     {
          boost::lock_guard<boost::mutex> head_lock( head_mutex );
          if( head.get() == get_tail() )
          {
               return std::auto_ptr<node>();
          }
          value = *head->data;
          return pop_head();
     }
};

template<typename T>
bool threadsafe_queue<T>::empty()
{
     boost::lock_guard<boost::mutex> head_lock( head_mutex );
     return empty_impl();
}

template<typename T>
void threadsafe_queue<T>::push( T new_value )
{
     boost::shared_ptr<T> new_data = boost::make_shared<T>( new_value );

     std::auto_ptr<node> p( new node );
     {
          boost::lock_guard<boost::mutex> tail_lock( tail_mutex );
          tail->data = new_data;
          node* const new_tail = p.get();
          tail->next = p;
          tail = new_tail;
     }

     data_cond.notify_one();
}

template<typename T>
void threadsafe_queue<T>::wait_and_pop( T& value )
{
     std::auto_ptr<node> const old_head = wait_pop_head( value );
}

template<typename T>
boost::shared_ptr<T> threadsafe_queue<T>::wait_and_pop()
{
     std::auto_ptr<node> const old_head = wait_pop_head();
     return old_head->data;
}

template<typename T>
bool threadsafe_queue<T>::try_pop( T& value )
{
     std::auto_ptr< node > const old_head = try_pop_head( value );
     return old_head.get() != 0;
}

template<typename T>
boost::shared_ptr<T> threadsafe_queue<T>::try_pop()
{
     std::auto_ptr<node> old_head = try_pop_head();
     return old_head.get() != 0 ? old_head->data : boost::shared_ptr<T>();
}

#endif FINE_GRAINED_THREAD_SAFE_QUEUE