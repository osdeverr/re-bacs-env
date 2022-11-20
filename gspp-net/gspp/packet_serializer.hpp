//
//  stream_client.hpp
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

namespace gspp
{
	// Specialized by library users
	template<class Packet, class = std::void_t<>>
	struct PacketSerializer;
}
