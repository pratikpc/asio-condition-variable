#pragma once

#include <mutex>

#include <chrono>

#ifdef ASIO_STANDALONE
#   include <asio/steady_timer.hpp>

#   include <asio/awaitable.hpp>
#   include <asio/co_spawn.hpp>
#   include <asio/redirect_error.hpp>
#   include <asio/use_awaitable.hpp>

#else
#   include <boost/asio/steady_timer.hpp>

#   include <boost/asio/awaitable.hpp>
#   include <boost/asio/co_spawn.hpp>
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
      struct none_t
      {
      };
      using asio::awaitable;
      using asio::use_awaitable;
   } // namespace
   template <typename T = void>
   struct condition_variable : public asio::steady_timer
   {
      using type = std::remove_cvref_t<T>;

    private:
      std::conditional_t<!std::is_void_v<type>, type, none_t> value;
      std::mutex                                              mutex;

    public:
      template <typename ExecutionContext>
      condition_variable(
          ExecutionContext& context,
          decltype(value)&& value =
              {}) requires(std::is_convertible_v<ExecutionContext&,
                                                 asio::execution_context&>) :
          asio::steady_timer{context,
                             std::chrono::high_resolution_clock::time_point::max()},
          value{std::move(value)}
      {
      }

      awaitable<::std::size_t> Notify() requires(std::is_void_v<type>)
      {
         return signal();
      }

      template <typename Convertible = type>
      awaitable<::std::size_t>
          Notify(Convertible&& p_value) requires(!std::is_void_v<type> &&
                                                 std::is_convertible_v<Convertible, type>)
      {
         {
            std::scoped_lock lock(mutex);
            value = p_value;
         }
         return signal();
      }

      awaitable<::std::size_t> NotifyOne()
      {
         return signal_once();
      }

      awaitable<type> operator()()
      {
         return call();
      }

      awaitable<type> call()
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

         if constexpr (!std::is_void_v<type>)
         {
            co_return value;
         }
      }

    private:
      awaitable<::std::size_t> signal()
      {
         return asio::co_spawn(
             get_executor(),
             [&]() -> asio::awaitable<::std::size_t> { co_return cancel(); },
             use_awaitable);
      }

      awaitable<::std::size_t> signal_once()
      {
         return asio::co_spawn(
             get_executor(),
             [&]() -> asio::awaitable<::std::size_t> { co_return cancel_one(); },
             use_awaitable);
      }
   };
} // namespace pc