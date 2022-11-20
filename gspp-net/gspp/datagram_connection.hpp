//
//  datagram_connection.hpp
//  gspp-net
//
//  Created by Nikita Ivanov on 24.09.2020.
//  Copyright � 2020 osdever. All rights reserved.
//

#pragma once
#include <bacs/bacs.hpp>
#include <plakpacs/plakpacs.hpp>
#include <functional>
#include <queue>
#include <mutex>
#include <boost/asio.hpp>

#include "packet_serializer.hpp"

namespace gspp
{
    template<class Protocol, class SPTraits = bacs::sp_default, size_t RecvSize = 0xFFFF>
    class DatagramConnection
    {
    public:
        using Socket = typename Protocol::socket;
        using Endpoint = typename Protocol::endpoint;
        using RecvBuffer = std::array<uint8_t, RecvSize>;

        DatagramConnection(Socket&& sock, std::function<void(Endpoint, const std::vector<uint8_t>&, std::size_t)> onHandle = {})
            : _state(std::make_shared<SharedStateBlock>(std::move(sock)))
        {
            auto state = _state;
            state->RecvLoopAsync(
                [state, onHandle](const boost::system::error_code& error, std::size_t bytes_transferred, Endpoint ep)
                {
                    if (!error.failed())
                        onHandle(ep, state->recv_buffer, bytes_transferred);
                }
            );
        }

        DatagramConnection() = delete;
        DatagramConnection(const DatagramConnection&) = delete;
        DatagramConnection(DatagramConnection&& other) = delete;

        template<class Packet>
        void Send(Endpoint ep, const Packet& packet)
        {
            auto ws = PacketSerializer<Packet>::template Serialize<plakpacs::write_stream>(packet);
            _state->SendAsync({ ep, ws.bytes() });
        }

        ~DatagramConnection()
        {
            _state->socket.close();
        }

    private:
        struct SharedStateBlock : std::enable_shared_from_this<SharedStateBlock>
        {
            Socket socket;

            struct PendingSendOp
            {
                Endpoint ep;
                std::vector<uint8_t> bytes;
            };

            std::vector<uint8_t> recv_buffer;

            SharedStateBlock(Socket&& rvsocket)
                : socket(std::move(rvsocket))
            {
                recv_buffer.resize(RecvSize);
            }

            template<class F>
            void RecvLoopAsync(F&& handler)
            {
                auto state = this->shared_from_this();
                auto ep = std::make_shared<Endpoint>();

                state->socket.async_receive_from(
                    boost::asio::buffer(recv_buffer.data(), recv_buffer.size()),
                    *ep, 0,
                    [state, ep, handler](const boost::system::error_code& error, std::size_t bytes_transferred)
                    {
                        handler(error, bytes_transferred, *ep);
                        state->RecvLoopAsync(handler);
                    }
                );
            }

            void SendAsync(const PendingSendOp& data)
            {
                // Same deal as in the ctor
                auto state = this->shared_from_this();
                auto ep = data.ep;

                using size_type = typename SPTraits::size_type;
                auto size = (size_type)data.bytes.size();
                SPTraits::out(size);

                auto ws = std::make_shared<plakpacs::write_stream>();
                ws->write(size);
                ws->write(data.bytes.begin(), data.bytes.end());

                state->socket.async_send_to(
                    boost::asio::const_buffer(ws->bytes().data(), ws->bytes().size()),
                    ep,
                    [state, ws](const boost::system::error_code&, std::size_t)
                    {
                    }
                );
            }
        };

        std::shared_ptr<SharedStateBlock> _state;
    };
}
