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

#ifndef VCML_PROTOCOLS_TLM_STUBS_H
#define VCML_PROTOCOLS_TLM_STUBS_H

#include "vcml/common/types.h"
#include "vcml/common/report.h"
#include "vcml/common/systemc.h"

namespace vcml {

class tlm_initiator_stub : public sc_module,
                           protected tlm::tlm_bw_transport_if<>
{
public:
    tlm::tlm_initiator_socket<64> out;

    tlm_initiator_stub() = delete;
    tlm_initiator_stub(const tlm_initiator_stub&) = delete;
    tlm_initiator_stub(tlm_initiator_stub&&) = delete;

    tlm_initiator_stub(const sc_module_name& name);
    virtual ~tlm_initiator_stub() = default;
    VCML_KIND(tlm_initiator_stub);

protected:
    virtual tlm::tlm_sync_enum nb_transport_bw(tlm_generic_payload& tx,
                                               tlm::tlm_phase& phase,
                                               sc_time& t) override;

    virtual void invalidate_direct_mem_ptr(sc_dt::uint64 start,
                                           sc_dt::uint64 end) override;
};

class tlm_target_stub : public sc_module, protected tlm::tlm_fw_transport_if<>
{
private:
    tlm_response_status m_response;

public:
    tlm::tlm_target_socket<64> in;

    tlm_target_stub() = delete;
    tlm_target_stub(const tlm_target_stub&) = delete;
    tlm_target_stub(tlm_target_stub&&) = delete;

    tlm_target_stub(const sc_module_name& name,
                    tlm_response_status response = TLM_ADDRESS_ERROR_RESPONSE);
    virtual ~tlm_target_stub() = default;
    VCML_KIND(tlm_target_stub);

protected:
    virtual void b_transport(tlm_generic_payload& tx, sc_time& t) override;
    virtual unsigned int transport_dbg(tlm_generic_payload& tx) override;
    virtual bool get_direct_mem_ptr(tlm_generic_payload& tx,
                                    tlm_dmi& dmi) override;
    virtual tlm::tlm_sync_enum nb_transport_fw(tlm_generic_payload& tx,
                                               tlm::tlm_phase& phase,
                                               sc_time& t) override;
};

} // namespace vcml

#endif
