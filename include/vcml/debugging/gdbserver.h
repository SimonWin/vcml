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

#ifndef VCML_GDBSERVER_H
#define VCML_GDBSERVER_H

#include "vcml/common/types.h"
#include "vcml/common/strings.h"
#include "vcml/common/report.h"
#include "vcml/common/bitops.h"

#include "vcml/logging/logger.h"

#include "vcml/debugging/target.h"
#include "vcml/debugging/gdbarch.h"
#include "vcml/debugging/rspserver.h"
#include "vcml/debugging/suspender.h"
#include "vcml/debugging/subscriber.h"

namespace vcml {
namespace debugging {

enum gdb_status {
    GDB_STOPPED,
    GDB_STEPPING,
    GDB_RUNNING,
    GDB_KILLED,
};

enum gdb_signal {
    GDBSIG_TRAP = 5,
    GDBSIG_KILL = 9,
};

class gdbserver : public rspserver, private subscriber, private suspender
{
private:
    target& m_target;
    const gdbarch* m_target_arch;
    string m_target_xml;
    atomic<gdb_status> m_status;
    gdb_status m_default;

    vector<const cpureg*> m_cpuregs;
    unordered_map<u64, const cpureg*> m_allregs;

    void update_status(gdb_status status);

    virtual void notify_step_complete(target& tgt) override;

    virtual void notify_breakpoint_hit(const breakpoint& bp) override;

    virtual void notify_watchpoint_read(const watchpoint& wp,
                                        const range& addr) override;

    virtual void notify_watchpoint_write(const watchpoint& wp,
                                         const range& addr,
                                         u64 newval) override;

    const cpureg* lookup_cpureg(unsigned int gdbno);

    typedef string (gdbserver::*handler)(const char*);
    std::map<char, handler> m_handler;

    enum breakpoint_type {
        GDB_BREAKPOINT_SW = 0,
        GDB_BREAKPOINT_HW = 1,
        GDB_WATCHPOINT_WRITE = 2,
        GDB_WATCHPOINT_READ = 3,
        GDB_WATCHPOINT_ACCESS = 4
    };

    handler find_handler(const char* command);

    string handle_unknown(const char* command);

    string handle_query(const char* command);
    string handle_rcmd(const char* command);
    string handle_xfer(const char* command);
    string handle_step(const char* command);
    string handle_continue(const char* command);
    string handle_detach(const char* command);
    string handle_kill(const char* command);

    string handle_reg_read(const char* command);
    string handle_reg_write(const char* command);
    string handle_reg_read_all(const char* command);
    string handle_reg_write_all(const char* command);
    string handle_mem_read(const char* command);
    string handle_mem_write(const char* command);
    string handle_mem_write_bin(const char* command);

    string handle_breakpoint_set(const char* command);
    string handle_breakpoint_delete(const char* command);

    string handle_exception(const char*);
    string handle_thread(const char*);
    string handle_vcont(const char*);

public:
    enum : size_t {
        PACKET_SIZE = 8 * MiB,
        BUFFER_SIZE = PACKET_SIZE / 2,
    };

    bool is_stopped() const { return m_status == GDB_STOPPED; }
    bool is_stepping() const { return m_status == GDB_STEPPING; }
    bool is_running() const { return m_status == GDB_RUNNING; }
    bool is_killed() const { return m_status == GDB_KILLED; }

    gdbserver() = delete;
    gdbserver(const gdbserver&) = delete;
    gdbserver(u16 port, target& stub, gdb_status status = GDB_STOPPED);
    virtual ~gdbserver();

    virtual string handle_command(const string& command) override;
    virtual void handle_connect(const char* peer) override;
    virtual void handle_disconnect() override;
};

} // namespace debugging
} // namespace vcml

#endif
