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

#ifndef VCML_RSP_H
#define VCML_RSP_H

#include "vcml/common/includes.h"
#include "vcml/common/types.h"
#include "vcml/common/utils.h"
#include "vcml/common/report.h"

#include "vcml/logging/logger.h"

#define VCML_RSP_MAX_PACKET_SIZE (0x4000u)

namespace vcml { namespace debugging {

    class rspserver
    {
    private:
        bool m_echo;
        u16  m_port;

        int  m_fd;
        int  m_fd_server;

        struct sockaddr_in m_server;
        struct sockaddr_in m_client;

        pthread_t m_thread;

        void send_char(char c);
        char recv_char();

        // disabled
        rspserver();
        rspserver(const rspserver&);

    public:
        u16 get_port() const { return m_port; }

        bool is_connected() const { return m_fd > -1; }
        bool is_listening() const { return m_fd_server > -1; }

        void echo(bool e = true) { m_echo = e; }

        rspserver(u16 port);
        virtual ~rspserver();

        void   send_packet(const string& s);
        void   send_packet(const char* format, ...);
        string recv_packet();
        int    recv_signal(unsigned int timeoutms = 0);

        void listen();
        void disconnect();

        void run();

        virtual string handle_command(const string& command);
        virtual void   handle_connect();
        virtual void   handle_disconnect();
    };

}}

#endif