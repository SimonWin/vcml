/******************************************************************************
 *                                                                            *
 * Copyright 2018 Jan Henrik Weinstock                                        *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 *     http://www.apache.org/licenses/LICENSE-2.0                             *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 *                                                                            *
 ******************************************************************************/

#include "vcml/common/thctl.h"
#include "vcml/common/report.h"
#include "vcml/common/systemc.h"

namespace vcml {

struct thctl {
    const thread::id sysc_thread;
    atomic<thread::id> curr_owner;

    mutex sysc_mutex;
    atomic<int> nwaiting;
    condition_variable_any cvar;

    thctl();
    ~thctl() = default;

    bool is_sysc_thread() const;
    bool is_in_critical() const;

    void notify();
    void block();

    void enter_critical();
    void exit_critical();

    void suspend();
    void flush();

    static thctl& instance();
};

thctl::thctl():
    sysc_thread(std::this_thread::get_id()),
    curr_owner(sysc_thread),
    sysc_mutex(),
    nwaiting(0),
    cvar() {
    sysc_mutex.lock();
}

bool thctl::is_sysc_thread() const {
    return std::this_thread::get_id() == sysc_thread;
}

bool thctl::is_in_critical() const {
    return std::this_thread::get_id() == curr_owner;
}

void thctl::notify() {
    cvar.notify_all();
}

void thctl::block() {
    VCML_ERROR_ON(is_sysc_thread(), "cannot block SystemC thread");
    lock_guard<mutex> l(sysc_mutex);
}

void thctl::enter_critical() {
    if (is_sysc_thread())
        VCML_ERROR("SystemC thread must not enter critical sections");
    if (is_in_critical())
        VCML_ERROR("thread already in critical section");
    if (!sim_running())
        return;

    int prev = nwaiting++;
    if (!sysc_mutex.try_lock()) {
        if (prev == 0)
            on_next_update([]() -> void { thctl_suspend(); });
        sysc_mutex.lock();
    }

    curr_owner = std::this_thread::get_id();
}

void thctl::exit_critical() {
    if (curr_owner != std::this_thread::get_id())
        VCML_ERROR("thread not in critical section");

    if (!sim_running())
        return;

    curr_owner = thread::id();
    sysc_mutex.unlock();

    if (--nwaiting == 0)
        notify();
}

void thctl::suspend() {
    VCML_ERROR_ON(!is_sysc_thread(), "this is not the SystemC thread");
    VCML_ERROR_ON(!is_in_critical(), "thread not in critical section");

    do {
        cvar.wait(sysc_mutex);
    } while (nwaiting > 0);

    curr_owner = sysc_thread;
}

void thctl::flush() {
    if (nwaiting > 0)
        suspend();
}

thctl& thctl::instance() {
    static thctl singleton;
    return singleton;
}

// need to make sure thctl gets created on the main (aka SystemC) thread
thctl& g_thctl = thctl::instance();

bool thctl_is_sysc_thread() {
    return thctl::instance().is_sysc_thread();
}

bool thctl_is_in_critical() {
    return thctl::instance().is_in_critical();
}

void thctl_notify() {
    thctl::instance().notify();
}

void thctl_block() {
    thctl::instance().block();
}

void thctl_enter_critical() {
    thctl::instance().enter_critical();
}

void thctl_exit_critical() {
    thctl::instance().exit_critical();
}

void thctl_suspend() {
    thctl::instance().suspend();
}

void thctl_flush() {
    thctl::instance().flush();
}

} // namespace vcml
