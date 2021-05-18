#pragma once

#include <mutex>

#include <chrono>

#ifdef ASIO_STANDALONE
#   include <asio/steady_timer.hpp>

#   include <asio/awaitable.hpp>
#   include <asio/co_spawn.hpp>
#   include <asio/detached.hpp>
#   include <asio/redirect_error.hpp>
#   include <asio/use_awaitable.hpp>

#else
#   include <boost/asio/steady_timer.hpp>

#   include <boost/asio/awaitable.hpp>
#   include <boost/asio/co_spawn.hpp>
#   include <boost/asio/detached.hpp>
#   include <boost/asio/redirect_error.hpp>
#   include <boost/asio/use_awaitable.hpp>
#endif

namespace pc
{
   namespace
   {
#ifndef ASIO_STANDALONE
      namespace asio = boost::asio;
      using boost::system::error_code;
#else
      using asio::error_code;
#endif
      using asio::awaitable;
      using asio::detached;
      using asio::use_awaitable;
   } // namespace

   struct event_waiter : private asio::steady_timer
   {
    private:
      ::std::size_t counter;
      ::std::mutex  mutex;

    public:
      template <typename ExecutionContext>
      event_waiter(ExecutionContext& context, ::std::size_t counter = 0) requires(
          std::is_convertible_v<ExecutionContext&, asio::execution_context&>) :
          asio::steady_timer{context,
                             std::chrono::high_resolution_clock::time_point::max()},
          counter{counter}
      {
      }

      void Notify()
      {
         auto const empty = notify();
         if (!empty)
            signal_once(detached);
      }
      awaitable<void> NotifyAsync()
      {
         auto const empty = notify();
         if (!empty)
            co_await signal_once(use_awaitable);
      }

      awaitable<void> operator()()
      {
         return call();
      }
      awaitable<void> call()
      {
         bool empty = true;
         {
            std::scoped_lock lock(mutex);
            empty = (counter == 0);
            ++counter;
         }
         if (!empty)
            co_await wait();
      }

    private:
      awaitable<void> wait()
      {
         error_code ec;
         using asio::redirect_error;
         co_await async_wait(redirect_error(use_awaitable, ec));

         // Upon cancel, error_code is set
         // to abort
         if (ec != asio::error::operation_aborted)
         {
            // If not aborted, throw exception
            asio::detail::throw_error(ec);
         }
      }
      template <typename Token>
      inline auto signal_once(Token const& token)
      {
         return asio::co_spawn(
             get_executor(),
             [&]() -> awaitable<void> {
                cancel_one();
                co_return;
             },
             token);
      }
      inline bool notify()
      {
         std::scoped_lock lock(mutex);
         if (counter != 0)
         {
            counter = counter - 1;
            return false;
         }
         return true;
      }
   };
} // namespace pc