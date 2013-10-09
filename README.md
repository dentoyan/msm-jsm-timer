msm-jsm-timer A Timer Extension for Boost Meta State Machine (MSM)
==================================================================

We propose a timer extension jsm::timer to boost::msm for state-based modeling of discrete event systems. A jsm::timer means delay time (i.e., timeout value) of a state and then, a specified event occurs automatically after timeout.
The jsm::timer class provides repetitive and single-shot timers. It uses boost::asio for timer services.


Usage
------

Every timer has four functions:
- void start(size_t timeout_, bool pulse_ = false): Starts the timer with a timeout of 'timeout_' milliseconds, and schedules the first notification.
- void start(): Restarts the timer 
- void stop(): Stops the timer.
- void wait(): Wait for timer expiration.


To use it, create a jsm::timer instance for a predefined event structure, and call start().


Example - Pulse Timer
-------------------------

``` c++

#include "jsm.h"

struct pulse {};

struct machine_ : public msm::front::state_machine_def<machine_>
{

  jsm::timer<pulse>   tm_pulse;

  template <class JSM>  machine_(JSM &m): tm_pulse(m)
  {
      tm_pulse.start(1000, true);
      // event pulse will be triggered every 1000 ms
  }

};

```


License
-------

jsm is distributed under the Boost Software License, Version 1.0. 





