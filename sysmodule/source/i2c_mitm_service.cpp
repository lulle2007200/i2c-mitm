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
#include "i2c_mitm_settings.hpp"
#include "logging.hpp"
#include "i2c_mitm_service.hpp"
#include <switch/services/i2c.h>

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
            DEBUG_LOG("OpenSessionForDev idx: %" PRIu32 ", addr: %" PRIu32 ", ProgID: 0x016%" PRIx64 ", i2c session mitm enabled" PRIu32,
                      bus_idx,
                      slave_address,
                      this->m_client_info.program_id.value);
            ::I2cSession session;
            const u32 in[] = {static_cast<u32>(bus_idx), slave_address, addressing_mode, speed_mode};
            R_TRY(serviceDispatchIn(this->m_forward_service.get(),
                                    0,
                                    in,
                                    .out_num_objects = 1,
                                    .out_objects     = &session.s));

            const sf::cmif::DomainObjectId target_obj_id(serviceGetObjectId(&session.s));

            out.SetValue(sf::CreateSharedObjectEmplaced<II2cSession, I2cSessionService>(std::make_unique<::I2cSession>(session), 0, this->m_client_info.program_id), target_obj_id);

            R_SUCCEED();
        } else {
            DEBUG_LOG("OpenSessionForDev idx: %" PRIu32 ", addr: %" PRIu32 ", ProgID: 0x016%" PRIx64,
                      bus_idx,
                      slave_address,
                      this->m_client_info.program_id.value);
            R_RETURN(sm::mitm::ResultShouldForwardToSession());
        }
    }

    Result I2cMitmService::OpenSession(sf::Out<sf::SharedPointer<II2cSession>> out, ::ams::i2c::I2cDevice device) {
        R_RETURN(this->OpenSession2(out, ConvertToDeviceCode(device)));
    }

    Result I2cMitmService::OpenSession2(sf::Out<sf::SharedPointer<II2cSession>> out, DeviceCode device_code) {
        if (ShouldMitmSession(device_code)) {
            DEBUG_LOG("OpenSession2 dev: %s (0x%" PRIx32 "), ProgID: 0x016%" PRIx64 ", i2c session mitm enabled", DeviceCodeToName(device_code).c_str(), device_code, this->m_client_info.program_id.value);
            ::I2cSession session;
            const u32 in = device_code.GetInternalValue();
            R_TRY(serviceDispatchIn(this->m_forward_service.get(),
                                    4,
                                    in,
                                    .out_num_objects = 1,
                                    .out_objects     = &session.s));

            const sf::cmif::DomainObjectId target_obj_id(serviceGetObjectId(&session.s));

            out.SetValue(sf::CreateSharedObjectEmplaced<II2cSession, I2cSessionService>(std::make_unique<::I2cSession>(session), device_code, this->m_client_info.program_id), target_obj_id);

            R_SUCCEED();
        } else {
            DEBUG_LOG("OpenSession2 dev: %s (0x%" PRIx32 "), ProgID: 0x016%" PRIx64, DeviceCodeToName(device_code).c_str(), device_code, this->m_client_info.program_id.value);
            R_RETURN(sm::mitm::ResultShouldForwardToSession());
        }
    }

    int I2cSessionService::LogPrintHeader(char *buf, size_t buf_size) {
        const u32 dev_id = this->m_device_code.GetInternalValue();
        return util::TSNPrintf(buf, buf_size, "ProgID: 0x016%" PRIx64 ", I2C dev: 0x%08" PRIx32 " (%s): ", this->m_program_id.value, dev_id, DeviceCodeToName(dev_id).c_str());
    }

    bool I2cSessionService::ShouldLog() {
        /* enable/disable logging per device here */
        #ifdef DEBUG
        return true;
        #else
        return false;
        #endif
    }

    void I2cSessionService::LogSendReceive(const u8 *data, size_t size, ::ams::i2c::TransactionOption option, bool is_send, Result result) {
        AMS_UNUSED(option);
        
        if (!this->ShouldLog()) {
            return;
        }

        static constexpr size_t buf_size = 0x400;
        char buf[buf_size];
        int idx = 0;

        idx += this->LogPrintHeader(buf + idx, buf_size - idx);
        idx += util::TSNPrintf(buf + idx, buf_size - idx, "result: 0x%08" PRIx32 ", %-4s, data: [", result.GetValue(), is_send ? "send" : "recv");

        for (size_t i = 0; i < size; i++) {
            idx += util::TSNPrintf(buf + idx, buf_size - idx, "0x%02" PRIx8 "%s", data[i], i + 1 < size ? ", " : "");
        }

        idx += util::TSNPrintf(buf + idx, buf_size - idx, "]");

        DEBUG_LOG("%s", buf);
    }

    void I2cSessionService::LogSend(const u8 *data, size_t size, ::ams::i2c::TransactionOption option, Result result) {
        return this->LogSendReceive(data, size, option, true, result);
    }

    void I2cSessionService::LogReceive(const u8 *data, size_t size, ::ams::i2c::TransactionOption option, Result result) {
        return this->LogSendReceive(data, size, option, false, result);
    }

    int I2cSessionService::LogPrintCommand(char *buf, size_t buf_size, const ::ams::i2c::I2cCommand *commands, size_t &num_commands) {
        int buf_idx = 0;
        size_t idx = 0;
        const util::BitPack8 command = static_cast<util::BitPack8>(commands[idx]);

        switch(command.Get<CommonCommandFormat::CommandId>()) {
        case CommandId_Send:
            {
                idx++;
                const u8 send_size = commands[idx];
                idx++;
                const u8 *send_data = commands + idx;
                idx += send_size - 1;

                buf_idx += util::TSNPrintf(buf + buf_idx, buf_size - buf_idx, "[send, len: 0x02%" PRIx8 ", data: [", send_size);

                for(size_t i = 0; i < send_size; i++) {
                    buf_idx += util::TSNPrintf(buf + buf_idx, buf_size - buf_idx, "0x%02" PRIx8 "%s", send_data[i], i + 1 < send_size ? ", " : "");
                }

                buf_idx += util::TSNPrintf(buf + buf_idx, buf_size - buf_idx, "]]");
            } break;
        case CommandId_Receive:
            {
                idx++;
                const u8 recv_size = commands[idx];

                buf_idx += util::TSNPrintf(buf + buf_idx, buf_size - buf_idx, "[recv, len: 0x%02" PRIx8 "]", recv_size);
            } break;
        case CommandId_Extension:
            {
                idx++;
                const u8 ms = commands[idx];

                buf_idx += util::TSNPrintf(buf + buf_idx, buf_size - buf_idx, "[sleep, ms: 0x%02" PRIx8 "]", ms);
            } break;
        }

        idx++;

        num_commands -= idx;

        return buf_idx;
    }

    void I2cSessionService::LogCommandList(const u8 *recv_data, size_t recv_size, const ::ams::i2c::I2cCommand *commands, size_t num_commands, Result result) {
        AMS_UNUSED(recv_data, recv_size, commands, num_commands, result);
        if (!ShouldLog()) {
            return;
        }

        static constexpr size_t buf_size = 0x400;
        char buf[buf_size];
        int buf_idx = 0;

        buf_idx += this->LogPrintHeader(buf, buf_size);
        buf_idx += util::TSNPrintf(buf + buf_idx, buf_size - buf_idx, "result: 0x%08" PRIx32 ", commands: [", result.GetValue());

        size_t commands_left = num_commands;

        while(commands_left) {
            buf_idx += this->LogPrintCommand(buf + buf_idx, buf_size - buf_idx, commands + num_commands - commands_left, commands_left);
            buf_idx += util::TSNPrintf(buf + buf_idx, buf_size - buf_idx, "%s", commands_left ? ", " : "");
        }

        buf_idx += util::TSNPrintf(buf + buf_idx, buf_size - buf_idx, "]");

        if (recv_data && recv_size) {
            buf_idx += util::TSNPrintf(buf + buf_idx, buf_size - buf_idx, ", recv data: [");
            for(size_t i = 0; i < recv_size; i++) {
                buf_idx += util::TSNPrintf(buf + buf_idx, buf_size - buf_idx, "0x%02" PRIx8 "%s", recv_data[i], i + 1 < recv_size ? ", " : "");
            }
            buf_idx += util::TSNPrintf(buf + buf_idx, buf_size - buf_idx, "]");
        }
        
        DEBUG_LOG("%s", buf);
    }


    I2cSessionService::I2cSessionService(std::unique_ptr<::I2cSession> session, DeviceCode device_code, ncm::ProgramId program_id) : m_session(std::move(session)), m_device_code(device_code), m_program_id(program_id) { }

    I2cSessionService::~I2cSessionService() {
        serviceClose(&this->m_session.get()->s);
    }

    Result I2cSessionService::HandleSendOverride(const u8 *data, size_t size, ::ams::i2c::TransactionOption option) {
        u8 override_voltage = GetConfig().voltage_config;
        /* 0x04: charge voltage control register, 0xca: 4.304V, 0xe2: 4.4V, 0xb2: 4.2V*/
        u8 override_voltage_cmd[2] = {0x04, override_voltage};

        static constexpr size_t buf_size = 0x100;
        char buf[buf_size];
        int buf_idx = 0;

        /* first, check if target device is bq24193 and send data size is 2 */
        if (this->m_device_code != 0x39000001 ||
            size != 2) {
            R_RETURN(::ams::i2c::ResultNoOverride());
        }

        /* handle set charge voltage command */
        if (data[0] == 0x04 && data[1] == 0xb2) {
            /* 0x04: charge voltage control register, 0xb2: 4.2V */
            buf_idx += this->LogPrintHeader(buf, buf_size);
            buf_idx += util::TSNPrintf(buf + buf_idx, buf_size - buf_idx, "Overriding set voltage command, setting voltage to %" PRIi32 "mV", GetConfig().voltage);
            DEBUG_LOG("%s", buf);

            Result result = serviceDispatchIn(&this->m_session.get()->s,
                                              10,
                                              option,
                                              .buffer_attrs = {SfBufferAttr_In | SfBufferAttr_HipcAutoSelect},
                                              .buffers = {{override_voltage_cmd, sizeof(override_voltage_cmd)}});

            this->LogSend(override_voltage_cmd, sizeof(override_voltage_cmd), option, result);

            R_RETURN(result);

        /* handle charge enable command */
        } else if(data[0] == 0x01 && ((data[1] >> 4) & 0x3) == 1) {
            /* 0x01: Power On Config Register, Bit 4:5 = 1: Battery Charge Enabled */
            buf_idx += this->LogPrintHeader(buf, buf_size);
            buf_idx += util::TSNPrintf(buf + buf_idx, buf_size - buf_idx, "Charging is being enabled, also set charge voltage to %" PRIi32 "mV", GetConfig().voltage);
            DEBUG_LOG("%s", buf);

            Result result = serviceDispatchIn(&this->m_session.get()->s,
                                              10,
                                              option,
                                              .buffer_attrs = {SfBufferAttr_In | SfBufferAttr_HipcAutoSelect},
                                              .buffers = {{override_voltage_cmd, sizeof(override_voltage_cmd)}});

            this->LogSend(override_voltage_cmd, sizeof(override_voltage_cmd), option, result);

            /* Still need to send the original command */
            R_TRY(result);
            R_RETURN(::ams::i2c::ResultNoOverride());
        }

        R_RETURN(::ams::i2c::ResultNoOverride());
    }

    Result I2cSessionService::SendOld(const sf::InBuffer &in_data, ::ams::i2c::TransactionOption option){
        Result result = serviceDispatchIn(&this->m_session.get()->s,
                                   0,
                                   option,
                                   .buffer_attrs = {SfBufferAttr_In | SfBufferAttr_HipcMapAlias},
                                   .buffers = {{in_data.GetPointer(), in_data.GetSize()}});

        this->LogSend(in_data.GetPointer(), in_data.GetSize(), option, result);

        R_RETURN(result);
    }
    Result I2cSessionService::ReceiveOld(const sf::OutBuffer &out_data, ::ams::i2c::TransactionOption option){
        Result result = serviceDispatchIn(&this->m_session.get()->s, 
                                          1, 
                                          option,
                                          .buffer_attrs = {SfBufferAttr_Out | SfBufferAttr_HipcMapAlias},
                                          .buffers = {{out_data.GetPointer(), out_data.GetSize()}});
       
        this->LogReceive(out_data.GetPointer(), out_data.GetSize(), option, result);

        R_RETURN(result);
    }
    Result I2cSessionService::ExecuteCommandListOld(const sf::OutBuffer &rcv_buf, const sf::InPointerArray<::ams::i2c::I2cCommand> &command_list){
        Result result = serviceDispatch(&this->m_session.get()->s,
                                        2,
                                        .buffer_attrs= {SfBufferAttr_Out | SfBufferAttr_HipcMapAlias, SfBufferAttr_In | SfBufferAttr_HipcPointer},
                                        .buffers = {{rcv_buf.GetPointer(), rcv_buf.GetSize()}, {command_list.GetPointer(), command_list.GetSize()}});

        this->LogCommandList(rcv_buf.GetPointer(), rcv_buf.GetSize(), command_list.GetPointer(), command_list.GetSize(), result);

        R_RETURN(result);
    }
    Result I2cSessionService::Send(const sf::InAutoSelectBuffer &in_data, ::ams::i2c::TransactionOption option){

        Result result = this->HandleSendOverride(in_data.GetPointer(), in_data.GetSize(), option);

        /* HandleSendOverride did handle the send */
        R_SUCCEED_IF(R_SUCCEEDED(result));
        /* HandleSendOverride did return an error that was not ResultNoOverride -> return the error */
        if (!::ams::i2c::ResultNoOverride::Includes(result)) {
            R_THROW(result);
        }
        /* No override, forward the send request */
        result = serviceDispatchIn(&this->m_session.get()->s,
                                   10,
                                   option,
                                   .buffer_attrs = {SfBufferAttr_In | SfBufferAttr_HipcAutoSelect},
                                   .buffers = {{in_data.GetPointer(), in_data.GetSize()}});

        this->LogSend(in_data.GetPointer(), in_data.GetSize(), option, result);

        R_RETURN(result);
    }
    Result I2cSessionService::Receive(const sf::OutAutoSelectBuffer &out_data, ::ams::i2c::TransactionOption option){
        Result result = serviceDispatchIn(&this->m_session.get()->s, 
                                          11, 
                                          option,
                                          .buffer_attrs = {SfBufferAttr_Out | SfBufferAttr_HipcAutoSelect},
                                          .buffers = {{out_data.GetPointer(), out_data.GetSize()}});

        this->LogReceive(out_data.GetPointer(), out_data.GetSize(), option, result);

        R_RETURN(result);
    }
    Result I2cSessionService::ExecuteCommandList(const sf::OutAutoSelectBuffer &rcv_buf, const sf::InPointerArray<::ams::i2c::I2cCommand> &command_list){

        Result result = serviceDispatch(&this->m_session.get()->s,
                                        12,
                                        .buffer_attrs= {SfBufferAttr_Out | SfBufferAttr_HipcAutoSelect, SfBufferAttr_In | SfBufferAttr_HipcPointer},
                                        .buffers = {{rcv_buf.GetPointer(), rcv_buf.GetSize()}, {command_list.GetPointer(), command_list.GetSize()}});

        this->LogCommandList(rcv_buf.GetPointer(), rcv_buf.GetSize(), command_list.GetPointer(), command_list.GetSize(), result);

        R_RETURN(result);
    }
    Result I2cSessionService::SetRetryPolicy(s32 max_retry_count, s32 retry_interval_us){
        const u32 in[] = {static_cast<u32>(max_retry_count), static_cast<u32>(retry_interval_us)};
        R_RETURN(serviceDispatchIn(&this->m_session.get()->s, 
                                   13, 
                                   in));
    }

}