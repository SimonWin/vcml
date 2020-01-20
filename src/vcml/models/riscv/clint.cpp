/******************************************************************************
 *                                                                            *
 * Copyright 2020 Jan Henrik Weinstock                                        *
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

#include "vcml/models/riscv/clint.h"

namespace vcml { namespace riscv {

    u64 clint::get_cycles() const {
        sc_time delta = sc_time_stamp() - m_time_reset;
        return delta / clock_cycle();
    }

    u32 clint::read_MSIP(unsigned int hart) {
        if (!IRQ_SW.exists(hart))
            return 0;

        return IRQ_SW[hart].read() ? 1 : 0;
    }

    u32 clint::write_MSIP(u32 val, unsigned int hart) {
        if (!IRQ_SW.exists(hart))
            return 0;

        IRQ_SW[hart].write(val);
        return val;
    }

    u64 clint::write_MTIMECMP(u64 val, unsigned int hart) {
        if (!IRQ_TIMER.exists(hart))
            return 0;

        u64 mtime = get_cycles();
        u64 mcomp = val;

        IRQ_TIMER[hart].write(mtime >= mcomp);

        if (mtime < mcomp)
            m_trigger.notify(clock_cycles(mcomp - mtime));

        return val;
    }

    u64 clint::read_MTIME() {
        return get_cycles();
    }

    void clint::update_timer() {
        u64 mnext = ~0ull;
        u64 mtime = get_cycles();

        for (auto it : IRQ_TIMER) {
            auto hart = it.first;
            auto port = it.second;

            u64 mcomp = MTIMECMP.get(hart);
            port->write(mtime >= mcomp);

            if (mcomp > mtime && mcomp < mnext)
                mnext = mcomp;
        }

        if (mnext != ~0ull)
            next_trigger(clock_cycles(mnext - mtime));
    }

    clint::clint(const sc_module_name& nm):
        peripheral(nm),
        m_time_reset(),
        m_trigger("triggerev"),
        MSIP("MSIP", 0x0000, 0),
        MTIMECMP("MTIMECMP", 0x4000, 0),
        MTIME("MTIME", 0xbff8, 0),
        IRQ_SW("IRQ_SW"),
        IRQ_TIMER("IRQ_TIMER"),
        IN("IN") {

        MSIP.sync_always();
        MSIP.allow_read_write();
        MSIP.tagged_read = &clint::read_MSIP;
        MSIP.tagged_write = &clint::write_MSIP;

        MTIMECMP.sync_on_write();
        MTIMECMP.allow_read_write();
        MTIMECMP.tagged_write = &clint::write_MTIMECMP;

        MTIME.sync_on_read();
        MTIME.allow_read();
        MTIME.read = &clint::read_MTIME;

        SC_METHOD(update_timer);
        sensitive << m_trigger;
        dont_initialize();
    }

    clint::~clint() {
        // nothing to do
    }

    void clint::reset() {
        peripheral::reset();

        m_time_reset = sc_time_stamp();
    }

}}
