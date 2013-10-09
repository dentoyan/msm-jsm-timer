// Copyright 2013 Joshua Dentoyan
// showing how to to use timers in MSM
// distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// vending_machine.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <cctype>                       
#include <cstdlib>
#include <string>
#include <stdlib.h>

#include "jsm.h"


namespace msm = boost::msm;
namespace mpl = boost::mpl;


namespace {

// events
struct exitnow {};

struct select
{
    select(int n_): n(n_) {  }
    int n;
};

struct coin
{
    coin(int c_): c(c_) {  }
    int c;
};

struct timeout {};

struct pulse {};


// front-end: define the FSM structure
struct vending_machine_ : public msm::front::state_machine_def<vending_machine_>
{

    struct item {
	char const  *name;              // name of yummy item
	int         price;              // cost in cents
    };
    
    static const item m_items[]; // array of items
    
    int num_items() const {
        int i = 0;
        for ( item const *p = m_items; p->name; ++p, ++i );
	return i;
    }

    int m_credit;                // running total of amount deposited
    bool exit;

    // timer instances
    jsm::timer<timeout> tm_timeout;
    jsm::timer<pulse>   tm_pulse;
    
    template <class JSM>
    vending_machine_(JSM &m)
            : tm_timeout(m)
            , tm_pulse(m)
    {
        m_credit = 0;
        exit = false;
    }

    // The list of FSM states
    struct Idle : public msm::front::state<>
    {
        // every (optional) entry/exit methods get the event passed.
        template <class Event,class FSM>
        void on_entry(Event const&,FSM &fsm)
        {
            using namespace std;
            cout << "entering: Idle" << endl;
            cout << "\nVending Machine:\n---------------\n";
	    int i = 0;
            for ( item const *p = &fsm.m_items[0]; p->name; ++p, ++i )
                cout << char('A' + i)
                     << ". " << p->name << " (" << dec
                     << p->price << ")\n";
            fsm.m_credit = 0;
            fsm.tm_timeout.stop();
	    fsm.tm_pulse.start(3000, true);
        }
        template <class Event,class FSM>
        void on_exit(Event const&, FSM &fsm)
        {
            std::cout << "leaving: Idle" << std::endl;
	    fsm.tm_pulse.stop();
        }
    };

    struct Collect : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const& ,FSM &fsm)
        {
            std::cout << "entering: Collect" << std::endl;
	    
        }
        template <class Event,class FSM>
        void on_exit(Event const&, FSM&fsm)
        {
           std::cout << "leaving: Collect" << std::endl;
        }
    };

    struct Exit : public msm::front::state<>
    {
        template <class Event,class FSM>
        void on_entry(Event const& ,FSM &fsm)
        {
            std::cout << "entering: Exit" << std::endl;
            fsm.exit = true;
        }
        template <class Event,class FSM>
        void on_exit(Event const&,FSM& )
        {
            std::cout << "leaving: Exit" << std::endl;
        }
    };


    // the initial state of the SM. Must be defined
    typedef Idle initial_state;

    // transition actions
    void t_collect(coin const&c)
    {
        m_credit += c.c;
        std::cout << "Credit: " << m_credit << " cents\n";
        tm_timeout.start(5 * 1000);
    }
    void t_pulse(pulse const&)
    {
        std::cout << "pulse\n";
    }
    void t_select(select const &s)
    {
        std::cout << "Enjoy your " << m_items[s.n].name << ".\n";
        int change = m_credit - m_items[s.n].price;
        if ( change > 0 )
            std::cout << "Change: " << change << " cents.\n";
    }
    void t_idle(timeout const &)
    {
        std::cout << "Timeout. \n";
        int change = m_credit;
        if ( change > 0 )
            std::cout << "Get back your money of: " << change << " cents.\n";
        m_credit = 0;
    }

    // guard conditions
    bool good_coin(coin const& evt)
    {
        using namespace std;
        cout << "checking coin " << dec << evt.c << "\n";
        // precondition: must be valid denomination
        switch ( evt.c ) {
        case 5:
        case 10:
        case 25:
        case 50:
            return true;
        case 1:
            cerr << "No pennies, please.\n";
            return false;
        default:
            cerr << "No foreign coins!\n";
            return false;
        }
    }
    
    bool good_select(select const& evt)
    {
        using namespace std;
        // precondition: selection must be within range
        if ( evt.n < 0 || evt.n >= num_items()) {
            cerr << "Invalid selection\n";
            return false;
        }

        // precondition: must have deposited sufficient amount
        if ( m_credit >= m_items[ evt.n ].price )
            return true;

        cerr << "Please deposit another "
             << m_items[ evt.n ].price - m_credit << " cents.\n";

        return false;
    }

    typedef vending_machine_ p; // makes transition table cleaner
    
    // Transition table for machine
    struct transition_table : mpl::vector<
                //    Start     Event         Next      Action				 Guard
                //  +---------+-------------+---------+---------------------+----------------------+
                _row < Idle   ,  exitnow ,     Exit >,
                 row < Idle   ,  coin        , Collect,  &p::t_collect      ,&p::good_coin         >,
               a_row < Idle   ,  pulse,        Idle,  &p::t_pulse                               >,

                _row < Collect,  exitnow     , Exit >,
                 row < Collect,  select      , Idle,     &p::t_select       ,&p::good_select       >,
                 row < Collect,  coin        , Collect,  &p::t_collect      ,&p::good_coin         >,
               a_row < Collect,  timeout     , Idle,     &p::t_idle                                >
                //  +---------+-------------+---------+---------------------+----------------------+
                > {};

    // Replaces the default no-transition response.
    template <class FSM,class Event>
    void no_transition(Event const& e, FSM&,int state)
    {
        std::cout << "no transition from state " << state
                  << " on event " << typeid(e).name() << std::endl;
    }

};

vending_machine_::item const vending_machine_::m_items[] =
{
    { "Cheeze Puffs",   65 },
    { "Chocolate Bar",  60 },
    { "Corn Chips",     80 },
    { "Popcorn",        90 },
    { "Potato Chips",   80 },
    { "Pretzels",       60 },
    { 0,                0  },
};



void process(jsm::state_machine<vending_machine_> &sm)
{
    vending_machine_ &m = sm.machine();
    while ( !std::cin.eof() && !m.exit) {
        using namespace std;
        //
        // Simple user-interface just to get events into the machine.
        //
        cout << "\nEnter coins (5,10,25,50) or selection (A-"
             << char( 'A' + m.num_items() - 1 ) << ") or x to exit: ";

        char buf[16];
        cin.getline( buf, sizeof buf );

        if ( isdigit( *buf ) )
            sm(coin( ::atoi( buf )));
        else if (*buf == 'x')
            sm(exitnow());
        else if ( isalpha( *buf ) )
            sm(select( toupper( *buf ) - 'A' ));
        else
            cerr << "Invalid input\n";
    }
}

}


int main()
{
    jsm::state_machine<vending_machine_> sm;
    sm.set_debug(false);
    sm.start();
    process(sm);
    sm.stop();
    return 0;
}

