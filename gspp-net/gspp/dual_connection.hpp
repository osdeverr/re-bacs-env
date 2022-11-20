//
//  dual_connection.hpp
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

#include "componentable.hpp"

namespace gspp
{
    namespace detail
    {
#ifdef GSPP_DEBUG_DONT_CATCH_HANDLER_EXCEPTIONS
        // This makes sure the exception does _NOT_ get caught in the handler and gets passed on.
        // This is kinda cleaner than just wrapping the entire catch into an #ifdef I think...
        class GenericHandlerException : public std::exception
        {};
#else
        using GenericHandlerException = std::exception;
#endif
    }
    
	template<class StreamClient, class DatagramClient>
	class DualConnection : public Componentable
	{		
	public:
		DualConnection(uint32_t id)
			: _id(id), _streamClient(nullptr), _dgClient(nullptr)
		{}

		template<class HandlerSystem>
		void SetupStreamClient(
			typename StreamClient::Socket&& socket,
			std::function<void(DualConnection&, typename HandlerSystem::HandlerResult)> onHandle = {},
			std::function<void(DualConnection&, const boost::system::error_code& ec)> onDeath = {},
			std::function<void(DualConnection&, const std::exception& e)> onHandleException = {})
		{
			_streamClient =
				std::make_shared<StreamClient>(
					std::move(socket),
					[this, onHandle, onHandleException](bacs::shared_buffer& buffer)
					{
						try
						{
							auto result = HandlerSystem::HandlerManager::GetInstance().HandlePacket(*this, buffer);

							if (onHandle)
								onHandle(*this, result);

							return (result == HandlerSystem::HandlerResult::Continue) && (!_killed);
						}
                        catch (const detail::GenericHandlerException& e)
						{
							// The exception is sometimes used in debugging. It's in our best interests to keep it named so we use this to silence the warnings
							(void)e;
                            
#ifdef GSPP_DEBUG_RETHROW_HANDLER_EXCEPTIONS
                            throw;
#endif

							if (onHandleException)
								onHandleException(*this, e);

							this->ScheduleDisconnect();

							if (onHandle)
								onHandle(*this, HandlerSystem::HandlerResult::Disconnect);

							// false means "yeah, disconnect him right away" in this context...
							return false;
						}
					},
					[this, onDeath](const boost::system::error_code& ec)
					{
						if (onDeath)
							onDeath(*this, ec);
					}
					);
		}

		void StartReceiveLoop()
		{
			if(_streamClient)
				_streamClient->StartReceiveLoop();
		}

	    void ConnectDG(std::shared_ptr<DatagramClient> udp)
		{
			_dgClient = udp;
		}

		void ScheduleDisconnect()
		{
			_killed = true;
		}

		void Close()
		{
			if (_streamClient)
				_streamClient->CloseSocket();
		}

		uint32_t id() const
		{
			return _id;
		}

		bool killed() const
		{
			return _killed;
		}

		std::shared_ptr<StreamClient> stream()
		{
			return _streamClient;
		}

		std::shared_ptr<DatagramClient> dg()
		{
			return _dgClient;
		}

		// Compatibility
		auto tcp()
		{
			return stream();
		}

		auto udp()
		{
			return dg();
		}

	private:
		uint32_t _id;
		bool _killed = false;
		std::shared_ptr<StreamClient> _streamClient = nullptr;
		std::shared_ptr<DatagramClient> _dgClient = nullptr;
	};
}
