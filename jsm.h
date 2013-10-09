//
// === JSM ===
//
// Just in State Machine
//
//
// MSM State Machine with timer support
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Joshua Dentoyan
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef _JSM_H_
#define _JSM_H_

#include <iostream>
#include <typeinfo>
#include <string>

#include <boost/assert.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

// back-end
#include <boost/msm/back/state_machine.hpp>
//front-end
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/back/common_types.hpp>


namespace jsm
{


template<class Event>
class timer
{
    public:
        template <class JSM>
	timer(JSM &m);
	
	typedef Event EventType;

	void start(size_t timeout_, bool pulse_ = false);
	void start();
	void stop();
	void wait();

    protected:
	void timerAction(const boost::system::error_code& error);

	boost::asio::deadline_timer timer_;
	bool running;
	size_t timeout;
	bool pulse;
	
        boost::function<void (Event const &)> slot;
};


template<class Event> template <class JSM>
inline
timer<Event>::timer(JSM &m)
    : timer_(m.get().timer_service())
    , running(false)
    , timeout(0)
    , pulse(false)
{
    typedef typename JSM::type M;
    slot = boost::bind(static_cast<void (M::*)(Event const &)>(&M::operator()), m, _1);
}

template<class Event>
inline
void timer<Event>::start(size_t timeout_, bool pulse_)
{
    stop();
    timeout = timeout_;
    pulse = pulse_;
    timer_.expires_from_now(boost::posix_time::milliseconds(timeout));
    running = true;
    timer_.async_wait( boost::bind( &timer::timerAction, this, _1 ) );
}

template<class Event>
inline
void timer<Event>::start()
{
    start(timeout, pulse);
}

template<class Event>
inline
void timer<Event>::stop()
{
    if (!running)
        return;
    timer_.cancel();
    running = false;
}

template<class Event>
inline
void timer<Event>::wait()
{
    timer_.wait();
}

template<class Event>
inline
void timer<Event>::timerAction(const boost::system::error_code& error)
{
    if (error)
        return;
    
    slot(EventType());
    if (pulse && running) {
        timer_.expires_at(timer_.expires_at() + boost::posix_time::milliseconds(timeout) );
        timer_.async_wait(boost::bind( &timer::timerAction, this, _1 ));
    }
}


class timer_context: private boost::noncopyable
{
    public:
        timer_context();

        void wait(size_t ms);
        void join();
	boost::asio::io_service &timer_service() {
	    return service;
	}
	
    protected:
        boost::asio::io_service service;
        boost::asio::io_service::work work;
        boost::thread thread;
};

inline
timer_context::timer_context()
    : work(service)
    , thread(boost::bind(&boost::asio::io_service::run, &service))
{

}

inline
void timer_context::join()
{
    thread.join();
}

inline
void timer_context::wait(size_t ms)
{
    boost::this_thread::sleep(boost::posix_time::milliseconds(ms));
}



template<class Machine>
class state_machine: public timer_context
{
    public:
        typedef Machine M;
        typedef boost::msm::back::state_machine<M> b_machine;

        state_machine();

        template<class Ev>
        void operator() (const Ev &event);

        Machine &machine() {
            return *m;
        }

        void set_debug(bool debug_) {
            debug = debug_;
        }

        void start() {
            m->start();
        }

        void stop() {
            timer_service().stop();
            join();
        }

        void print_state() {
            int cs = m->current_state()[0];
            std::cout << "current_state: " << cs << std::endl;
        }

    private:
        boost::shared_ptr<b_machine> m;
        boost::mutex mutex;
        bool debug;
};


template<class Machine> template<class Ev>
inline void state_machine<Machine>::operator() (const Ev &event)
{
    boost::mutex::scoped_lock scoped_lock(mutex);
    using namespace std;
    if (debug)
        cout << "process: " << typeid(event).name() << endl;
    m->process_event(event);
    if (debug)
        print_state();
}


template<class Machine>
inline
state_machine<Machine>::state_machine(): debug(false)
{
    m = boost::shared_ptr<b_machine>(new b_machine(boost::ref(*this)));
}

}

#endif

