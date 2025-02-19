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

#include "vcml/setup.h"

#define PRINT(...) fprintf(stderr, ##__VA_ARGS__)

namespace vcml {

static void exit_usage(const char* arg0, int code) {
    PRINT("Usage: %s [program arguments]\n", arg0);
    PRINT("  -l | --log [file]         Enable logging to <file>|stdout\n");
    PRINT("       --log-debug          Activate debug logging\n");
    PRINT("       --log-delta          Include delta cycle in logs\n");
    PRINT("  -t | --trace [file]       Enable tracing to <file>|stdout\n");
    PRINT("  -f | --config-file <file> Read configuration from <file>\n");
    PRINT("  -c | --config  <x>=<y>    Set property <x> to value <y>\n");
    PRINT("  -h | --help               Print this message\n");
    exit(code);
}

bool setup::parse_command_line(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        m_args.push_back(arg);

        if (!strcmp(arg, "--help") | !strcmp(arg, "-h")) {
            exit_usage(argv[0], EXIT_SUCCESS);

        } else if (!strcmp(arg, "--log-debug")) {
            m_log_debug = !m_log_debug;

        } else if (!strcmp(arg, "--log-delta")) {
            publisher::print_delta_cycle = !publisher::print_delta_cycle;

        } else if (!strcmp(arg, "--log") || !strcmp(arg, "-l")) {
            if (i >= argc - 1 || *argv[i + 1] == '-')
                m_log_stdout = !m_log_stdout;
            else
                stl_add_unique(m_log_files, string(argv[++i]));

        } else if (!strcmp(arg, "--trace") || !strcmp(arg, "-t")) {
            if (i >= argc - 1 || *argv[i + 1] == '-')
                m_trace_stdout = !m_trace_stdout;
            else
                stl_add_unique(m_trace_files, string(argv[++i]));

        } else if (!strcmp(arg, "--config-file") || !strcmp(arg, "-f")) {
            if (i >= argc - 1 || *argv[i + 1] == '-') {
                PRINT("Error: %s expects <file> argument\n", arg);
                return false;
            }

            if (!file_exists(argv[i + 1])) {
                PRINT("Error: config file not found '%s'\n", argv[i + 1]);
                return false;
            }

            m_config_files.push_back(argv[++i]);
        }
    }

    return true;
}

setup* setup::s_instance = nullptr;

setup::setup(int argc, char** argv):
#ifdef VCML_DEBUG
    m_log_debug(true),
#else
    m_log_debug(false),
#endif
    m_log_stdout(false),
    m_trace_stdout(false),
    m_log_files(),
    m_trace_files(),
    m_config_files(),
    m_publishers(),
    m_brokers() {
    VCML_ERROR_ON(s_instance != nullptr, "setup already created");
    s_instance = this;

    if (!parse_command_line(argc, argv))
        exit_usage(argv[0], EXIT_FAILURE);

    log_level min = LOG_ERROR;
    log_level max = m_log_debug ? LOG_DEBUG : LOG_INFO;

    for (const string& file : m_log_files) {
        publisher* pub = new log_file(file);
        pub->set_level(min, max);
        m_publishers.push_back(pub);
    }

    if (m_log_stdout || m_log_files.empty()) {
        publisher* pub = new log_term(true);
        pub->set_level(min, max);
        m_publishers.push_back(pub);
    }

    for (const string& file : m_trace_files) {
        tracer* t = new tracer_file(file);
        m_tracers.push_back(t);
    }

    if (m_trace_stdout) {
        tracer* t = new tracer_term(true);
        m_tracers.push_back(t);
    }

    m_brokers.push_back(new broker_arg(argc, argv));
    m_brokers.push_back(new broker_env());

    for (const string& file : m_config_files)
        m_brokers.push_back(new broker_file(file));
}

setup::~setup() {
    s_instance = nullptr;

    for (auto broker : m_brokers)
        delete broker;

    for (auto tracer : m_tracers)
        delete tracer;

    for (auto pub : m_publishers)
        delete pub;
}

setup* setup::instance() {
    return s_instance;
}

int main(int argc, char** argv) {
#ifndef VCML_DEBUG
    // disable deprecated warning for release builds
    sc_core::sc_report_handler::set_actions("/IEEE_Std_1666/deprecated",
                                            sc_core::SC_DO_NOTHING);
#endif

    setup env(argc, argv);

    int res = EXIT_FAILURE;

    try {
        res = sc_core::sc_elab_and_sim(argc, argv);
    } catch (vcml::report& rep) {
        log.error(rep);
    } catch (std::exception& ex) {
        log.error(ex);
    }

    // at this point sc_is_running is false and no new critical sections
    // should be entered, but we need to give those who are still waiting
    // to execute a final chance to run
    thctl_flush();

    return res;
}

} // namespace vcml

extern "C" int main(int argc, char** argv) __attribute__((weak));
extern "C" int main(int argc, char** argv) {
    return vcml::main(argc, argv);
}
