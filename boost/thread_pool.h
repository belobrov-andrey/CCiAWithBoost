///
/// Copyright (c) Anthony Williams. 
//
//  Adapted to boost by Andrey Belobrov.
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef THREAD_POOL_H__
#define THREAD_POOL_H__

#include "threadsafe_queue.h"

#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/thread/future.hpp>
#include <boost/utility/result_of.hpp>

class thread_pool
{
ublic:
     thread_pool( int thread_count = boost::thread::hardware_concurrency() )
     {
          for( unsigned i = 0; i < thread_count; ++i )
          {
               woringThreadGroup_.create_thread( boost::bind( &thread_pool::worker_thread, this ) );
          }
     }

     ~thread_pool()
     {
          woringThreadGroup_.interrupt_all();
          woringThreadGroup_.join_all();
     }

     template< typename FunctionType >
     boost::unique_future< typename boost::result_of< FunctionType()>::type >    
          submit( FunctionType f )
     {
          typedef typename boost::result_of< FunctionType() >::type result;
          typedef boost::packaged_task <result > packaged_task;
          typedef boost::shared_ptr< boost::packaged_task< result > > packaged_task_ptr;

          packaged_task_ptr taskPtr( new packaged_task( f ) ); // packaged_task is move only, using shared_ptr 
          boost::unique_future< result > res( taskPtr->get_future() );

          // Thanks Pavel Shevaev ( http://efiquest.org ) for this tecnique of storing packaged_tasks
          work_queue.push( boost::bind( &boost::packaged_task< result >::operator(), taskPtr ) );

          return boost::move( res );
     }
     threadsafe_queue< boost::function< void() > > work_queue;
     boost::thread_group woringThreadGroup_;

private:
     void worker_thread()
     {
          try
          {
               while( true )
               {
                    boost::function< void() > task;
                    work_queue.wait_and_pop( task );
                    task();
               }
          }
          catch( const boost::thread_interrupted& /*e*/ )
          {
          }
     }

};

#endif // THREAD_POOL_H__