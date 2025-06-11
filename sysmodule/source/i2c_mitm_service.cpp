/*
 * Copyright (c) 2024 ndeadly
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "logging.hpp"
#include <vapours/results/fs_results.hpp>
#include <vapours/results/results_common.hpp>
#include "i2c_mitm_service.hpp"

namespace ams::i2c {
    R_DEFINE_ERROR_RESULT(NoOverride, 4);
}

namespace ams::mitm::i2c {

    enum CommandId {
        CommandId_Send      = 0,
        CommandId_Receive   = 1,
        CommandId_Extension = 2,
        CommandId_Count     = 3,
    };

    enum SubCommandId {
        SubCommandId_Sleep = 0,
    };

    struct CommonCommandFormat {
        using CommandId    = util::BitPack8::Field<0, 2>;
        using SubCommandId = util::BitPack8::Field<2, 6>;
    };

    struct ReceiveCommandFormat {
        using StartCondition = util::BitPack8::Field<6, 1, bool>;
        using StopCondition  = util::BitPack8::Field<7, 1, bool>;
        using Size           = util::BitPack8::Field<0, 8>;
    };

    struct SendCommandFormat {
        using StartCondition = util::BitPack8::Field<6, 1, bool>;
        using StopCondition  = util::BitPack8::Field<7, 1, bool>;
        using Size           = util::BitPack8::Field<0, 8>;
    };

    struct SleepCommandFormat {
        using MicroSeconds = util::BitPack8::Field<0, 8>;
    };


    constexpr std::string DeviceCodeToName(DeviceCode device_code) {
        switch(device_code.GetInternalValue()) {
        case 0x350000C9:
            return "ClassicController";
        case 0x35000033:
            return "Ftm3bd56";
        case 0x3E000001:
            return "Tmp451 or Nct72";
        case 0x33000001:
            return "Alc5639";
        case 0x3B000001:
            return "Max77620Rtc";
        case 0x3A000001:
            return "Max77620Pmic";
        case 0x3A000003:
            return "Max77621Cpu";
        case 0x3A000004:
            return "Max77621Gpu";
        case 0x39000001:
            return "Bq24193";
        case 0x39000033:
            return "Max17050";
        case 0x040000C9:
            return "Bm92t30mwv";
        case 0x3F000401:
            return "Ina226Vdd15v0Hb";
        case 0x3F000001:
            return "Ina226VsysCpuDs or Ina226VddCpuAp (SdevMariko)";
        case 0x3F000002:
            return "Ina226VsysGpuDs or Ina226VddGpuAp (SdevMariko)";
        case 0x3F000003:
            return "Ina226VsysDdrDs or Ina226VddDdr1V1Pmic (SdevMariko)";
        case 0x3F000402:
            return "Ina226VsysAp";
        case 0x3F000403:
            return "Ina226VsysBlDs";
        case 0x35000047:
            return "Bh1730";
        case 0x3F000404:
            return "Ina226VsysCore or Ina226VddCoreAp (SdevMariko)";
        case 0x3F000405:
            return "Ina226Soc1V8 or Ina226VddSoc1V8 (SdevMariko)";
        case 0x3F000406:
            return "Ina226Lpddr1V8 or Ina226Vdd1V8 (SdevMariko)";
        case 0x3F000407:
            return "Ina226Reg1V32";
        case 0x3F000408:
            return "Ina226Vdd3V3Sys";
        case 0x34000001:
            return "HdmiDdc";
        case 0x34000002:
            return "HdmiScdc";
        case 0x34000003:
            return "HdmiHdcp";
        case 0x3A000005:
            return "Fan53528";
        case 0x3A000002:
            return "Max77812Pmic";
        case 0x3A000006:
            return "Max77812Pmic";
        case 0x3F000409:
            return "Ina226VddDdr0V6 (SdevMariko)";
        case 0x36000001:
            return "MillauNfc";
        case 0x3A000007:
            return "Max77801";
        default:
            return "Unknown";
        }
    }

    bool I2cMitmService::ShouldMitmSession(DeviceCode device_code) {
        /* Only mitm i2c session for bq24193 */
        return device_code.GetInternalValue() == 0x39000001;
    }

    bool I2cMitmService::ShouldMitmSession(s32 bus_idx, u16 slave_address) {
        AMS_UNUSED(bus_idx);
        AMS_UNUSED(slave_address);
        return false;
    }

    bool I2cMitmService::ShouldMitm(const sm::MitmProcessInfo &process_info) {
        AMS_UNUSED(process_info);
        return true;
    }


    Result I2cMitmService::OpenSessionForDev(sf::Out<sf::SharedPointer<II2cSession>> out, s32 bus_idx, u16 slave_address, ::ams::i2c::AddressingMode addressing_mode, ::ams::i2c::SpeedMode speed_mode) {
        if (ShouldMitmSession(bus_idx, slave_address)) {
            DEBUG_LOG("OpenSessionForDev idx: %" PRIu32 ", addr: %, i2c session mitm enabled" PRIu32);
            ::I2cSession session;
            const u32 in[] = {static_cast<u32>(bus_idx), slave_address, addressing_mode, speed_mode};
            R_TRY(serviceDispatchIn(this->m_forward_service.get(),
                                    0,
                                    in,
                                    .out_num_objects = 1,
                                    .out_objects     = &session.s));

            const sf::cmif::DomainObjectId target_obj_id(serviceGetObjectId(&session.s));

            out.SetValue(sf::CreateSharedObjectEmplaced<II2cSession, I2cSessionService>(std::make_unique<::I2cSession>(session), 0), target_obj_id);

            R_SUCCEED();
        } else {
            DEBUG_LOG("OpenSessionForDev idx: %" PRIu32 ", addr: %" PRIu32);
            R_RETURN(sm::mitm::ResultShouldForwardToSession());
        }
    }

    Result I2cMitmService::OpenSession(sf::Out<sf::SharedPointer<II2cSession>> out, ::ams::i2c::I2cDevice device) {
        R_RETURN(this->OpenSession2(out, ConvertToDeviceCode(device)));
    }

    Result I2cMitmService::OpenSession2(sf::Out<sf::SharedPointer<II2cSession>> out, DeviceCode device_code) {
        if (ShouldMitmSession(device_code)) {
            DEBUG_LOG("OpenSession2 dev: %s (0x%" PRIx32 "), i2c session mitm enabled", DeviceCodeToName(device_code).c_str(), device_code);
            ::I2cSession session;
            const u32 in = device_code.GetInternalValue();
            R_TRY(serviceDispatchIn(this->m_forward_service.get(),
                                    4,
                                    in,
                                    .out_num_objects = 1,
                                    .out_objects     = &session.s));

            const sf::cmif::DomainObjectId target_obj_id(serviceGetObjectId(&session.s));

            out.SetValue(sf::CreateSharedObjectEmplaced<II2cSession, I2cSessionService>(std::make_unique<::I2cSession>(session), device_code), target_obj_id);

            R_SUCCEED();
        } else {
            DEBUG_LOG("OpenSession2 dev: %s (0x%" PRIx32 ")", DeviceCodeToName(device_code).c_str(), device_code);
            R_RETURN(sm::mitm::ResultShouldForwardToSession());
        }
    }

    bool I2cSessionService::ShouldLog() {
        /* enable/disable logging per device here */
        return true;
    }

    void I2cSessionService::LogSendReceive(const u8 *data, size_t size, ::ams::i2c::TransactionOption option, bool is_send) {
        if (!this->ShouldLog()) {
            return;
        }

        util::BitPack32 bp_option = static_cast<util::BitPack32>(option);

        DEBUG_DATA_DUMP(data, size, "dev: %s (0x%" PRIx32 "), dir: %s, start_cond: %d, stop_cond: %d, data:",
                        DeviceCodeToName(this->m_device_code.GetInternalValue()).c_str(),
                        this->m_device_code.GetInternalValue(),
                        is_send ? "send" : "receive",
                        bp_option.Get<util::BitPack32::Field<0, 1>>(),
                        bp_option.Get<util::BitPack32::Field<1, 1>>());
    }
    void I2cSessionService::LogSend(const u8 *data, size_t size, ::ams::i2c::TransactionOption option) {
        return this->LogSendReceive(data, size, option, true);
    }
    void I2cSessionService::LogReceive(const u8 *data, size_t size, ::ams::i2c::TransactionOption option) {
        return this->LogSendReceive(data, size, option, false);
    }

    void I2cSessionService::LogCommandList(const u8 *recv_data, size_t recv_size, const ::ams::i2c::I2cCommand *commands, size_t num_commands) {
        if (!ShouldLog()) {
            return;
        }

        char buf[0x400];
        int idx = 0;

        const util::BitPack8 *bp_commands = reinterpret_cast<const util::BitPack8*>(commands);

        idx += util::TSNPrintf(buf + idx, sizeof(buf) - idx, "dev: %s (0x%" PRIx32 "), commands: [",
                               DeviceCodeToName(this->m_device_code.GetInternalValue()).c_str(),
                               this->m_device_code.GetInternalValue());

        for(size_t i = 0; i < num_commands; i++) {
            const util::BitPack8 &command = bp_commands[i];
            switch(command.Get<CommonCommandFormat::CommandId>()) {
            case CommandId_Send:
                {
                    i++;
                    const u8 send_size = commands[i];
                    i++;
                    const u8 *send_data = commands + i;
                    i += send_size - 1;

                    idx += util::TSNPrintf(buf, sizeof(buf) - idx, "[ send, len: 0x%" PRIx8 ", start_cond: %d, stop_cond: %d, data: ", 
                                           send_size,
                                           command.Get<SendCommandFormat::StartCondition>(),
                                           command.Get<SendCommandFormat::StopCondition>());

                    for(int j = 0; j < send_size; j++) {
                        idx += util::TSNPrintf(buf + idx, sizeof(buf) - idx, "%02x ", 
                                               send_data[j]);
                    }

                    idx += util::TSNPrintf(buf + idx, sizeof(buf) - idx, "]");
                } break;
            case CommandId_Receive:
                {
                    i++;
                    const u8 recv_size = commands[i];

                    idx += util::TSNPrintf(buf + idx, sizeof(buf) - idx, "[ recv, len: 0x%02" PRIx8 " ]", recv_size);
                } break;
            case CommandId_Extension:
                {
                    i++;
                    const u8 ms = commands[i];

                    idx += util::TSNPrintf(buf + idx, sizeof(buf) - idx, "[ sleep, ms: 0x%02" PRIx8 " ]", ms);
                } break;
            }

            if (i + 1 < num_commands) {
                idx += util::TSNPrintf(buf + idx, sizeof(buf) - idx, ", ");
            }
        }

        idx += util::TSNPrintf(buf + idx, sizeof(buf) - idx, "], recv_data:");

        DEBUG_DATA_DUMP(recv_data, recv_size, "%s", buf);
    }


    I2cSessionService::I2cSessionService(std::unique_ptr<::I2cSession> session, DeviceCode device_code) : m_session(std::move(session)), m_device_code(device_code) { }

    I2cSessionService::~I2cSessionService() {
        serviceClose(&this->m_session.get()->s);
    }

    Result I2cSessionService::HandleSendOverride(const u8 *data, size_t size, ::ams::i2c::TransactionOption option) {
        /* first, check if target device is bq24193 and data to send is 0x04 (charge voltage register) 0xb2 (4,2V charge voltage) */
        if (this->m_device_code != 0x39000001) {
            R_RETURN(::ams::i2c::ResultNoOverride());
        }

        if (size != 2) {
            R_RETURN(::ams::i2c::ResultNoOverride());
        }

        if (data[0] != 0x04 || data[1] != 0xb2) {
            R_RETURN(::ams::i2c::ResultNoOverride());
        }

        DEBUG_LOG("Setting bq24193 charge voltage to 4.304V");

        /* 0x04: voltage control register, 0xe2: 4.4V */
        static constexpr u8 buf[2] = {0x04, 0xca};

        R_RETURN(serviceDispatchIn(&this->m_session.get()->s,
                                   10,
                                   option,
                                   .buffer_attrs = {SfBufferAttr_In | SfBufferAttr_HipcAutoSelect},
                                   .buffers = {{buf, sizeof(buf)}}));
    }

    Result I2cSessionService::SendOld(const sf::InBuffer &in_data, ::ams::i2c::TransactionOption option){
        this->LogSend(in_data.GetPointer(), in_data.GetSize(), option);

        R_RETURN(serviceDispatchIn(&this->m_session.get()->s,
                                   0,
                                   option,
                                   .buffer_attrs = {SfBufferAttr_In | SfBufferAttr_HipcMapAlias},
                                   .buffers = {{in_data.GetPointer(), in_data.GetSize()}}));
    }
    Result I2cSessionService::ReceiveOld(const sf::OutBuffer &out_data, ::ams::i2c::TransactionOption option){
        R_RETURN(serviceDispatchIn(&this->m_session.get()->s, 
                                   1, 
                                   option,
                                   .buffer_attrs = {SfBufferAttr_Out | SfBufferAttr_HipcMapAlias},
                                   .buffers = {{out_data.GetPointer(), out_data.GetSize()}}));
       
        this->LogReceive(out_data.GetPointer(), out_data.GetSize(), option);
    }
    Result I2cSessionService::ExecuteCommandListOld(const sf::OutBuffer &rcv_buf, const sf::InPointerArray<::ams::i2c::I2cCommand> &command_list){
        R_RETURN(serviceDispatch(&this->m_session.get()->s,
                                 2,
                                 .buffer_attrs= {SfBufferAttr_Out | SfBufferAttr_HipcMapAlias, SfBufferAttr_In | SfBufferAttr_HipcPointer},
                                 .buffers = {{rcv_buf.GetPointer(), rcv_buf.GetSize()}, {command_list.GetPointer(), command_list.GetSize()}}));

        this->LogCommandList(rcv_buf.GetPointer(), rcv_buf.GetSize(), command_list.GetPointer(), command_list.GetSize());
    }
    Result I2cSessionService::Send(const sf::InAutoSelectBuffer &in_data, ::ams::i2c::TransactionOption option){
        this->LogSend(in_data.GetPointer(), in_data.GetSize(), option);

        Result result = this->HandleSendOverride(in_data.GetPointer(), in_data.GetSize(), option);

        /* HandleSendOverride did handle the send */
        R_SUCCEED_IF(R_SUCCEEDED(result));
        /* HandleSendOverride did return an error that was not ResultNoOverride -> return the error */
        if (!::ams::i2c::ResultNoOverride::Includes(result)) {
            R_THROW(result);
        }
        /* No override, forward the send request */
        R_RETURN(serviceDispatchIn(&this->m_session.get()->s,
                                   10,
                                   option,
                                   .buffer_attrs = {SfBufferAttr_In | SfBufferAttr_HipcAutoSelect},
                                   .buffers = {{in_data.GetPointer(), in_data.GetSize()}}));
    }
    Result I2cSessionService::Receive(const sf::OutAutoSelectBuffer &out_data, ::ams::i2c::TransactionOption option){
        R_RETURN(serviceDispatchIn(&this->m_session.get()->s, 
                                   11, 
                                   option,
                                   .buffer_attrs = {SfBufferAttr_Out | SfBufferAttr_HipcAutoSelect},
                                   .buffers = {{out_data.GetPointer(), out_data.GetSize()}}));

        this->LogReceive(out_data.GetPointer(), out_data.GetSize(), option);
    }
    Result I2cSessionService::ExecuteCommandList(const sf::OutAutoSelectBuffer &rcv_buf, const sf::InPointerArray<::ams::i2c::I2cCommand> &command_list){
        R_RETURN(serviceDispatch(&this->m_session.get()->s,
                                 12,
                                 .buffer_attrs= {SfBufferAttr_Out | SfBufferAttr_HipcAutoSelect, SfBufferAttr_In | SfBufferAttr_HipcPointer},
                                 .buffers = {{rcv_buf.GetPointer(), rcv_buf.GetSize()}, {command_list.GetPointer(), command_list.GetSize()}}));

        this->LogCommandList(rcv_buf.GetPointer(), rcv_buf.GetSize(), command_list.GetPointer(), command_list.GetSize());
    }
    Result I2cSessionService::SetRetryPolicy(s32 max_retry_count, s32 retry_interval_us){
        const u32 in[] = {static_cast<u32>(max_retry_count), static_cast<u32>(retry_interval_us)};
        R_RETURN(serviceDispatchIn(&this->m_session.get()->s, 
                                   13, 
                                   in));
    }

}