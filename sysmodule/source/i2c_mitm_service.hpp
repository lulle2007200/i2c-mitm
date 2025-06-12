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
#pragma once
#include <stratosphere.hpp>

#define AMS_I2C_SESSION_MITM_INTERFACE_INFO(C, H)                                                                                                                                                                                                                      \
    AMS_SF_METHOD_INFO(C, H,  0, Result, SendOld,               (const sf::InBuffer &in_data,             ::ams::i2c::TransactionOption option),                                           (in_data,         option),            hos::Version_Min, hos::Version_5_1_0) \
    AMS_SF_METHOD_INFO(C, H,  1, Result, ReceiveOld,            (const sf::OutBuffer &out_data,           ::ams::i2c::TransactionOption option),                                           (out_data,        option),            hos::Version_Min, hos::Version_5_1_0) \
    AMS_SF_METHOD_INFO(C, H,  2, Result, ExecuteCommandListOld, (const sf::OutBuffer &rcv_buf,            const ams::sf::InPointerArray<::ams::i2c::I2cCommand> &command_list),            (rcv_buf,         command_list),      hos::Version_Min, hos::Version_5_1_0) \
    AMS_SF_METHOD_INFO(C, H, 10, Result, Send,                  (const sf::InAutoSelectBuffer &in_data,   ::ams::i2c::TransactionOption option),                                           (in_data,         option)                                                 ) \
    AMS_SF_METHOD_INFO(C, H, 11, Result, Receive,               (const sf::OutAutoSelectBuffer &out_data, ::ams::i2c::TransactionOption option),                                           (out_data,        option)                                                 ) \
    AMS_SF_METHOD_INFO(C, H, 12, Result, ExecuteCommandList,    (const sf::OutAutoSelectBuffer &rcv_buf,  const ams::sf::InPointerArray<::ams::i2c::I2cCommand> &command_list),            (rcv_buf,         command_list)                                           ) \
    AMS_SF_METHOD_INFO(C, H, 13, Result, SetRetryPolicy,        (s32 max_retry_count, s32 retry_interval_us),                                                                              (max_retry_count, retry_interval_us), hos::Version_6_0_0                  )

AMS_SF_DEFINE_INTERFACE(ams::mitm::i2c, II2cSession, AMS_I2C_SESSION_MITM_INTERFACE_INFO, 0x40154EFE)

#define AMS_I2C_MITM_INTERFACE_INFO(C, H)                                                                                                                                                                                                                                                                                                        \
    AMS_SF_METHOD_INFO(C, H,  0, Result, OpenSessionForDev,     (sf::Out<sf::SharedPointer<ams::mitm::i2c::II2cSession>> out, s32 bus_idx, u16 slave_address, ::ams::i2c::AddressingMode addressing_mode, ::ams::i2c::SpeedMode speed_mode), (out, bus_idx, slave_address, addressing_mode, speed_mode )                                       ) \
    AMS_SF_METHOD_INFO(C, H,  1, Result, OpenSession,           (sf::Out<sf::SharedPointer<ams::mitm::i2c::II2cSession>> out, ::ams::i2c::I2cDevice device),                                                                                 (out, device)                                                                                     ) \
    AMS_SF_METHOD_INFO(C, H,  4, Result, OpenSession2,          (sf::Out<sf::SharedPointer<ams::mitm::i2c::II2cSession>> out, ::ams::impl::DeviceCodeType device_code),                                                                                       (out, device_code),                                         hos::Version_6_0_0                    )

AMS_SF_DEFINE_MITM_INTERFACE(ams::mitm::i2c, II2cMitmInterface, AMS_I2C_MITM_INTERFACE_INFO, 0xE4C9D8F0)

namespace ams::i2c {
    R_DEFINE_ERROR_RESULT(NoOverride, 4);
}

namespace ams::mitm::i2c {

    class I2cSessionService {
    protected:
        std::unique_ptr<::I2cSession> m_session;
        DeviceCode m_device_code;
        ncm::ProgramId m_program_id;
    public:
        I2cSessionService(std::unique_ptr<::I2cSession> session, DeviceCode device_code, ncm::ProgramId program_id);
        virtual ~I2cSessionService();

        Result SendOld(const sf::InBuffer &in_data, ::ams::i2c::TransactionOption option);
        Result ReceiveOld(const sf::OutBuffer &out_data, ::ams::i2c::TransactionOption option);
        Result ExecuteCommandListOld(const sf::OutBuffer &rcv_buf, const sf::InPointerArray<::ams::i2c::I2cCommand> &command_list);
        Result Send(const sf::InAutoSelectBuffer &in_data, ::ams::i2c::TransactionOption option);
        Result Receive(const sf::OutAutoSelectBuffer &out_data, ::ams::i2c::TransactionOption option);
        Result ExecuteCommandList(const sf::OutAutoSelectBuffer &rcv_buf, const sf::InPointerArray<::ams::i2c::I2cCommand> &command_list);
        Result SetRetryPolicy(s32 max_retry_count, s32 retry_interval_us);

    private:
        virtual Result SendOldCb(const sf::InBuffer &in_data, ::ams::i2c::TransactionOption option) { AMS_UNUSED(in_data, option); R_RETURN(::ams::i2c::ResultNoOverride()); }
        virtual Result ReceiveOldCb(const sf::OutBuffer &out_data, ::ams::i2c::TransactionOption option) { AMS_UNUSED(out_data, option); R_RETURN(::ams::i2c::ResultNoOverride()); }
        virtual Result ExecuteCommandListOldCb(const sf::OutBuffer &rcv_buf, const sf::InPointerArray<::ams::i2c::I2cCommand> &command_list) { AMS_UNUSED(rcv_buf, command_list); R_RETURN(::ams::i2c::ResultNoOverride()); }
        virtual Result SendCb(const sf::InAutoSelectBuffer &in_data, ::ams::i2c::TransactionOption option) { AMS_UNUSED(in_data, option); R_RETURN(::ams::i2c::ResultNoOverride()); }
        virtual Result ReceiveCb(const sf::OutAutoSelectBuffer &out_data, ::ams::i2c::TransactionOption option) { AMS_UNUSED(out_data, option); R_RETURN(::ams::i2c::ResultNoOverride()); }
        virtual Result ExecuteCommandListCb(const sf::OutAutoSelectBuffer &rcv_buf, const sf::InPointerArray<::ams::i2c::I2cCommand> &command_list) { AMS_UNUSED(rcv_buf, command_list); R_RETURN(::ams::i2c::ResultNoOverride()); }
        virtual Result SetRetryPolicyCb(s32 max_retry_count, s32 retry_interval_us) { AMS_UNUSED(max_retry_count, retry_interval_us); R_RETURN(::ams::i2c::ResultNoOverride()); }

    protected:
        virtual bool ShouldLog() {
            #ifdef DEBUG
            return true;
            #else
            return false;
            #endif
        };
        int LogPrintHeader(char *buf, size_t buf_size);
        int LogPrintCommand(char *buf, size_t buf_size, const ::ams::i2c::I2cCommand *commands, size_t &num_commands);
        void LogSendReceive(const u8 *data, size_t size, ::ams::i2c::TransactionOption option, bool is_send, Result result);
        void LogSend(const u8 *data, size_t size, ::ams::i2c::TransactionOption option, Result result);
        void LogReceive(const u8 *data, size_t size, ::ams::i2c::TransactionOption option, Result result);
        void LogCommandList(const u8 *recv_data, size_t recv_size, const ::ams::i2c::I2cCommand *commands, size_t num_commands, Result result);
        void LogRetryPolicy(s32 max_retry_count, s32 retry_interval_us, Result result);
    };
    static_assert(IsII2cSession<I2cSessionService>);

    class Bq24193I2cSessionService : public I2cSessionService {
    private:
        static constinit util::Atomic<bool> first_init_done;
    public:
        Bq24193I2cSessionService(std::unique_ptr<::I2cSession> session, DeviceCode device_code, ncm::ProgramId program_id);

    private:
        virtual Result SendCb(const sf::InAutoSelectBuffer &in_data, ::ams::i2c::TransactionOption option);
        Result SetVoltage(u8 voltage_config);
    };

    class I2cMitmService : public sf::MitmServiceImplBase {
    public:
        using sf::MitmServiceImplBase::MitmServiceImplBase;

    public:
        /* determines wether mitm should be enabled for i2c session */
        static bool ShouldMitmSession(DeviceCode device_code);
        static bool ShouldMitmSession(s32 bus_idx, u16 slave_address);

        /* Determines wether i2c mitm should be enabled */
        static bool ShouldMitm(const sm::MitmProcessInfo &);


        Result OpenSessionForDev(sf::Out<sf::SharedPointer<II2cSession>> out, s32 bus_idx, u16 slave_address, ::ams::i2c::AddressingMode addressing_mode, ::ams::i2c::SpeedMode speed_mode);
        Result OpenSession(sf::Out<sf::SharedPointer<II2cSession>> out, ::ams::i2c::I2cDevice device);
        Result OpenSession2(sf::Out<sf::SharedPointer<II2cSession>> out, DeviceCode device_code);
        
    private:
        sf::SharedPointer<II2cSession> GetI2cSessionForDevice(::I2cSession session, DeviceCode device_code);
        sf::SharedPointer<II2cSession> GetI2cSessionForDevice(::I2cSession session, s32 bus_idx, s32 addr);

    };
    static_assert(IsII2cMitmInterface<II2cMitmInterface>);

}
