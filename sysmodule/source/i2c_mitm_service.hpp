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
#include <stratosphere/i2c/i2c_types.hpp>

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


namespace ams::mitm::i2c {

    class I2cSessionService {
    private:
        std::unique_ptr<::I2cSession> m_session;
        DeviceCode m_device_code;
    public:
        I2cSessionService(std::unique_ptr<::I2cSession> session, DeviceCode device_code);
        virtual ~I2cSessionService();

        Result SendOld(const sf::InBuffer &in_data, ::ams::i2c::TransactionOption option);
        Result ReceiveOld(const sf::OutBuffer &out_data, ::ams::i2c::TransactionOption option);
        Result ExecuteCommandListOld(const sf::OutBuffer &rcv_buf, const sf::InPointerArray<::ams::i2c::I2cCommand> &command_list);
        Result Send(const sf::InAutoSelectBuffer &in_data, ::ams::i2c::TransactionOption option);
        Result Receive(const sf::OutAutoSelectBuffer &out_data, ::ams::i2c::TransactionOption option);
        Result ExecuteCommandList(const sf::OutAutoSelectBuffer &rcv_buf, const sf::InPointerArray<::ams::i2c::I2cCommand> &command_list);
        Result SetRetryPolicy(s32 max_retry_count, s32 retry_interval_us);

    private:
        bool ShouldLog();
        void LogSendReceive(const u8 *data, size_t size, ::ams::i2c::TransactionOption option, bool is_send);
        void LogSend(const u8 *data, size_t size, ::ams::i2c::TransactionOption option);
        void LogReceive(const u8 *data, size_t size, ::ams::i2c::TransactionOption option);
        void LogCommandList(const u8 *recv_data, size_t recv_size, const ::ams::i2c::I2cCommand *commands, size_t num_commands);

        Result HandleSendOverride(const u8 *data, size_t size, ::ams::i2c::TransactionOption);
    };
    static_assert(IsII2cSession<I2cSessionService>);

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

    };
    static_assert(IsII2cMitmInterface<II2cMitmInterface>);

}
