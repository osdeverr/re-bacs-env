//
//  datagram_client.hpp
//  gspp-net
//
//  Created by Nikita Ivanov on 24.09.2020.
//  Copyright © 2020 osdever. All rights reserved.
//

#pragma once
#include <bacs/bacs.hpp>
#include <plakpacs/plakpacs.hpp>
#include <functional>
#include <queue>
#include <mutex>
#include <boost/asio.hpp>

#include "packet_serializer.hpp"
#include "datagram_connection.hpp"

namespace gspp
{
	template<class Protocol, class SPTraits = bacs::sp_default, size_t RecvSize = 0xFFFF>
	class DatagramClient
	{
	public:
		using Connection = DatagramConnection<Protocol, SPTraits, RecvSize>;
		using Endpoint = typename Connection::Endpoint;

		DatagramClient(std::shared_ptr<Connection> connection, Endpoint ep)
			: _conn(connection), _ep(ep)
		{}

		const auto& connection() const
		{
			return _conn;
		}

		Endpoint endpoint() const
		{
			return _ep;
		}

		template<class Packet>
		void Send(const Packet& packet)
		{
			_conn->Send(_ep, packet);
		}

	private:
		std::shared_ptr<Connection> _conn;
		Endpoint _ep;
	};
}
