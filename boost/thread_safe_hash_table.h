///
/// Copyright (c) Anthony Williams. 
//
//  Adapted to boost by Andrey Belobrov.
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef THREAD_SAFE_HASH_TABLE
#define THREAD_SAFE_HASH_TABLE

#include <vector>
#include <functional>
#include <list>
#include <utility>

#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/functional/hash.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>

template<typename Key, typename Value, typename Hash = boost::hash<Key> >
class threadsafe_lookup_table : public boost::noncopyable
{
private:

     class bucket_type
     {
     private:
          typedef std::pair<Key, Value> bucket_value;
          typedef std::list<bucket_value> bucket_data;
          typedef typename bucket_data::iterator bucket_iterator;

          bucket_data data;
          mutable boost::shared_mutex mutex;

          bucket_iterator find_entry_for( Key const& key )
          {
               return std::find_if( data.begin(), data.end(), boost::bind( std::equal_to< Key >(),
                    boost::bind( &bucket_value::first, _1 ), key ) );
          }
     public:
          boost::optional< Value > value_for( Key const& key )
          {
               boost::shared_lock<boost::shared_mutex> lock( mutex );
               bucket_iterator const found_entry = find_entry_for( key );
               return (found_entry == data.end() ) ?
                      boost::optional< Value >() : boost::optional< Value >( found_entry->second );
          }

          bool try_update_mapping( Key const& key, Value const& value )
          {
               boost::unique_lock<boost::shared_mutex> lock( mutex );
               bucket_iterator const found_entry = find_entry_for( key );
               if( found_entry == data.end() )
               {
                    data.push_back( bucket_value( key, value ) );
                    return true;
               }
               else
               {
                    return false;
               }
          }

          void remove_mapping( Key const& key )
          {
               boost::unique_lock<boost::shared_mutex> lock( mutex );
               bucket_iterator const found_entry = find_entry_for( key );
               if( found_entry != data.end() )
               {
                    data.erase( found_entry );
               }
          }
     };

     std::vector<boost::shared_ptr<bucket_type> > buckets;
     Hash hasher;

     bucket_type& get_bucket( Key const& key ) const
     {
          std::size_t const bucket_index = hasher( key ) % buckets.size();
          return *buckets[bucket_index];
     }
public:
     typedef Key key_type;
     typedef Value mapped_type;
     typedef Hash hash_type;

     threadsafe_lookup_table(
          size_t num_buckets = 19, Hash const& hasher_ = Hash() ) :
          buckets( num_buckets ), hasher( hasher_ )
     {
          for( size_t i = 0; i < num_buckets; ++i )
          {
               buckets[i].reset( new bucket_type );
          }
     }

     boost::optional< Value > value_for( Key const& key ) const
     {
          return get_bucket( key ).value_for( key );
     }

     bool try_update_mapping( Key const& key, Value const& value )
     {
          return get_bucket( key ).try_update_mapping( key, value );
     }

     void remove_mapping( Key const& key )
     {
          get_bucket( key ).remove_mapping( key );
     }
};

#endif // ifndef THREAD_SAFE_HASH_TABLE
