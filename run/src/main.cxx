#include <array>
#include <iostream>
#include <thread>

#include <pc/condition_variable.hpp>

#ifdef ASIO_STANDALONE
#   include <asio.hpp>
#else
#   include <boost/asio.hpp>
#endif

#ifndef ASIO_STANDALONE
namespace asio = boost::asio;
#endif

static auto constexpr THREAD_COUNT = 10;

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

      pc::condition_variable<> cv(io_context);
      using asio::awaitable;
      using asio::co_spawn;
      using asio::detached;
      using asio::steady_timer;
      using asio::use_awaitable;

      // Wait for notification,
      // Then perform action
      for (std::size_t i = 0; i < THREAD_COUNT; ++i)
         co_spawn(
             io_context,
             [&cv]() -> awaitable<void> {
                std::cout << "\nWaiting for call ";
                co_await cv();
                std::cout << "\nReceived Signal";
             },
             detached);

      // Send notification to all
      co_spawn(
          io_context,
          [&cv]() -> awaitable<void> {
             namespace this_coro = asio::this_coro;

             auto executor = co_await this_coro::executor;
             std::cout << "\nNotification process started. Wait for a few seconds ";
             steady_timer deadline(executor, ::std::chrono::seconds(10));
             co_await deadline.async_wait(use_awaitable);
             co_await cv.Notify();
             std::cout << "\n Notification Sent ";
          },
          detached);

      // Send a single notification
      co_spawn(
          io_context,
          [&cv]() -> awaitable<void> {
             namespace this_coro = asio::this_coro;

             auto executor = co_await this_coro::executor;
             std::cout
                 << "\nSingle Notification process started. Wait for a few seconds ";
             steady_timer deadline(executor, ::std::chrono::seconds(3));
             co_await deadline.async_wait(use_awaitable);
             co_await cv.NotifyOne();
             std::cout << "\nSingle Notification Sent ";
          },
          detached);

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
