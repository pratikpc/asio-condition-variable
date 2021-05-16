#pragma once

#include <pc/event_waiter.hpp>

namespace pc
{
   struct scoped_event_waiter
   {
      pc::event_waiter& event_waiter;
      scoped_event_waiter(pc::event_waiter& event_waiter) : event_waiter{event_waiter} {}
      auto operator()()
      {
         return event_waiter();
      }
      ~scoped_event_waiter()
      {
         event_waiter.Notify();
      }
   };
} // namespace pc