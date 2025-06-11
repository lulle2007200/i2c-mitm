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
#include "i2c_mitm_module.hpp"
#include "i2c_mitm_service.hpp"
#include "logging.hpp"
#include <stratosphere.hpp>

namespace ams::mitm::i2c {

    namespace {

        enum PortIndex {
            PortIndex_I2cMitm,
            PortIndex_I2cPcvMitm,
            PortIndex_Count,
        };

        constexpr sm::ServiceName g_i2c_mitm_service_name     = sm::ServiceName::Encode("i2c");
        constexpr sm::ServiceName g_i2c_pcv_mitm_service_name = sm::ServiceName::Encode("i2c:pcv");

        /* TODO: ??? */
        struct ServerOptions {
            static constexpr size_t PointerBufferSize   = 0x1000;
            static constexpr size_t MaxDomains          = 0;
            static constexpr size_t MaxDomainObjects    = 0;
            static constexpr bool CanDeferInvokeRequest = false;
            static constexpr bool CanManageMitmServers  = true;
        };

        constexpr size_t MaxSessions = 0x10;

        class ServerManager final : public sf::hipc::ServerManager<PortIndex_Count, ServerOptions, MaxSessions> {
            private:
                virtual Result OnNeedsToAccept(int port_index, Server *server) override;
        };

        ServerManager g_server_manager;

        Result ServerManager::OnNeedsToAccept(int port_index, Server *server) {
            /* Acknowledge the mitm session. */
            std::shared_ptr<::Service> fsrv;
            sm::MitmProcessInfo client_info;
            server->AcknowledgeMitmSession(std::addressof(fsrv), std::addressof(client_info));

            switch (port_index) {
                case PortIndex_I2cMitm:
                    DEBUG_LOG("i2c mitm accept");
                    return this->AcceptMitmImpl(server, sf::CreateSharedObjectEmplaced<II2cMitmInterface, I2cMitmService>(decltype(fsrv)(fsrv), client_info), fsrv);
                case PortIndex_I2cPcvMitm:
                    DEBUG_LOG("i2c:pcv mitm accept");
                    return this->AcceptMitmImpl(server, sf::CreateSharedObjectEmplaced<II2cMitmInterface, I2cMitmService>(decltype(fsrv)(fsrv), client_info), fsrv);
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        constexpr s32 ThreadPriority = 9;
        constexpr size_t ThreadStackSize = 0x2000;
        alignas(os::ThreadStackAlignment) constinit u8 g_thread_stack[ThreadStackSize];
        constinit os::ThreadType g_thread;
        
        void I2cMitmThreadFunction(void *) {
            R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<I2cMitmService>(PortIndex_I2cMitm, g_i2c_mitm_service_name)));
            R_ABORT_UNLESS((g_server_manager.RegisterMitmServer<I2cMitmService>(PortIndex_I2cPcvMitm, g_i2c_pcv_mitm_service_name)));
            g_server_manager.LoopProcess();
        }

    }

    void Launch() {
        R_ABORT_UNLESS(os::CreateThread(&g_thread,
            I2cMitmThreadFunction,
            nullptr,
            g_thread_stack,
            ThreadStackSize,
            ThreadPriority
        ));

        os::SetThreadNamePointer(&g_thread, "I2cMitmThread");
        os::StartThread(&g_thread);
    }

    void WaitFinished() {
        os::WaitThread(&g_thread);
    }

}