#pragma once

#include <type_traits>
#include <boost/asio/associated_allocator.hpp>

namespace async_nats::detail
{

template<class Token>
inline auto allocate_ctx(Token&& token)
{
  using CH = std::decay_t<Token>;
  using AssotiatedAllocator = decltype(boost::asio::get_associated_allocator(token));
  using CH_alloc_t = typename std::allocator_traits<AssotiatedAllocator>::
      template rebind_alloc<CH>;

  CH_alloc_t alloc(boost::asio::get_associated_allocator(token));
  return new(alloc.allocate(1)) CH(std::move(token));
}

template<class Token>
inline void deallocate_ctx(Token* token)
{
  using CH = std::decay_t<Token>;
  using AssotiatedAllocator = decltype(boost::asio::get_associated_allocator(*token));
  using CH_alloc_t = typename std::allocator_traits<AssotiatedAllocator>::
      template rebind_alloc<CH>;

  CH_alloc_t alloc(boost::asio::get_associated_allocator(*token));
  alloc.deallocate(token, 1);
}

}
