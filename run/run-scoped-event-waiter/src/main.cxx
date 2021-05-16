#include <array>
#include <iostream>
#include <thread>

#include <pc/event_waiter.hpp>
#include <pc/scoped_event_waiter.hpp>

#ifdef ASIO_STANDALONE
#   include <asio.hpp>
#else
#   include <boost/asio.hpp>
#endif

#ifndef ASIO_STANDALONE
namespace asio = boost::asio;
#endif

static auto constexpr THREAD_COUNT = 2;

using asio::awaitable;
awaitable<void> Run(pc::event_waiter& event_waiter,
                    ::std::size_t     duration = 2,
                    ::std::size_t     count    = 5,
                    ::std::size_t     start    = 5)
{
   using asio::co_spawn;
   using asio::detached;
   using asio::steady_timer;
   using asio::use_awaitable;
   namespace this_coro = asio::this_coro;

   auto executor = co_await this_coro::executor;
   for (std::size_t i = start; i < start + count; ++i)
      co_spawn(
          executor,
          [&event_waiter, duration, i]() -> awaitable<void> {
             pc::scoped_event_waiter waiter(event_waiter);
             co_await waiter();
             std::cout << "\nReceived Signal " << i;
             // Wait for 4 seconds
             // Then notify
             {

                auto         executor = co_await this_coro::executor;
                steady_timer deadline(executor, ::std::chrono::seconds(duration));
                co_await deadline.async_wait(use_awaitable);
             }
             std::cout << "\nNotifying " << i;
          },
          detached);
}

int main()
{
   try
   {
      asio::io_context io_context(THREAD_COUNT);

      std::array<std::thread, THREAD_COUNT - 1> threads;

      asio::signal_set signals(io_context, SIGINT, SIGTERM);
      signals.async_wait([&io_context](auto, auto) {
         std::cout << "Stopping server\n";
         io_context.stop();
      });

      // Create Threads
      for (auto& thread : threads)
         thread = std::thread([&io_context] {
            std::cout << std::this_thread::get_id() << " started\n";
            io_context.run();
            std::cout << std::this_thread::get_id() << " over\n";
         });
      std::cout << std::this_thread::get_id() << " started\n";

      pc::event_waiter event_waiter(io_context);

      using asio::co_spawn;
      using asio::detached;
      co_spawn(io_context, Run(event_waiter, 6, 10, 0), detached);
      co_spawn(io_context, Run(event_waiter, 3, 10, 10), detached);
      co_spawn(io_context, Run(event_waiter, 1, 10, 20), detached);

      // Wait for threads to finish
      {
         // Run the I/O service on the requested number of threads
         io_context.run();

         std::cout << std::this_thread::get_id() << " overed\n";
         for (auto& thread : threads)
            if (thread.joinable())
               thread.join();
      }
   }
   catch (...)
   {
      std::cout << "Exception";
   }
   return EXIT_SUCCESS;
}
