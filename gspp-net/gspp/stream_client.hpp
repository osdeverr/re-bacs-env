//
//  stream_client.hpp
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
	template<class Protocol, class SPTraits = bacs::sp_default>
    class StreamClient
    {
    public:
        using Socket = typename Protocol::socket;

        StreamClient(Socket&& sock, std::function<bool(bacs::shared_buffer&)> onHandle = {}, std::function<void(const boost::system::error_code&)> onDeath = {})
            : _state(std::make_shared<SharedStateBlock>(std::move(sock), onHandle, onDeath))
        {
        }

        StreamClient() = delete;
        StreamClient(const StreamClient&) = delete;
        StreamClient(StreamClient&& other) = delete;

        auto endpoint() const
        {
            return _state->socket.remote_endpoint();
        }

        void StartReceiveLoop()
        {
            // We're copying here to capture the shared ptr in the handler lambda in order to prolong its lifetime until at least its invocation, this is important
            auto state = _state;

            bacs::async_read_sp_loop<SPTraits>(state->socket,
                [state](const boost::system::error_code& ec, std::size_t bytes_read, bacs::shared_buffer buffer)
                {
                    if (ec.failed())
                    {
                        if (state->on_death)
                            state->on_death(ec);

                        return false;
                    }
                    else
                    {
                        return (state->on_handle) ? state->on_handle(buffer) : true;
                    }
                });
        }

        template<class Packet>
        void Send(const Packet& packet)
        {
            auto ws = PacketSerializer<Packet>::template Serialize<plakpacs::write_stream>(packet);

            std::lock_guard lg{ _state->write_lock };
            _state->write_queue.push(ws.bytes());

            // If it's empty, restart the operation
            if (_state->write_queue.size() == 1)
                _state->WriteAsync(_state->write_queue.front());
        }

        void CloseSocket()
        {
            std::lock_guard lg{ _state->write_lock };

            if (_state->write_queue.empty())
                _state->socket.close();
            else
                _state->shutdown = true;
        }

        ~StreamClient()
        {
            CloseSocket();
        }

    private:
        struct SharedStateBlock : std::enable_shared_from_this<SharedStateBlock>
        {
            Socket socket;

            std::queue<std::vector<uint8_t>> write_queue;
            std::mutex write_lock;

            std::function<bool(bacs::shared_buffer&)> on_handle;
            std::function<void(const boost::system::error_code&)> on_death;

            bool shutdown = false;

            SharedStateBlock(Socket&& rvsocket, std::function<bool(bacs::shared_buffer&)> onHandle, std::function<void(const boost::system::error_code&)> onDeath)
                : socket(std::move(rvsocket)), on_handle(onHandle), on_death(onDeath)
            {}

            ~SharedStateBlock()
            {
                socket.close();
            }

            void WriteAsync(const std::vector<uint8_t>& data)
            {
                // Same deal as in the ctor
                auto state = this->shared_from_this();

                bacs::async_write_sp<SPTraits>(
                    state->socket, data,
                    [state](const boost::system::error_code&, std::size_t)
                    {
                        std::lock_guard lg{ state->write_lock };

                        state->write_queue.pop();

                        if (state->shutdown)
                        {
                            boost::system::error_code ec;
                            state->socket.shutdown(state->socket.shutdown_both, ec);

                            state->socket.close();
                        }

                        if (!state->write_queue.empty())
                            state->WriteAsync(state->write_queue.front());
                    }
                );
            }
        };

        std::shared_ptr<SharedStateBlock> _state;
    };
}
