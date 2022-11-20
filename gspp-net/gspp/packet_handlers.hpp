//
//  packet_handlers.hpp
//  gspp-net
//
//  Created by Nikita Ivanov on 24.09.2020.
//  Copyright � 2020 osdever. All rights reserved.
//

#pragma once
#include <plakpacs/plakpacs.hpp>
#include <unordered_map>
#include <memory>

#include "packet_serializer.hpp"

namespace gspp
{
	template<class State, class Header, class IdType, class HeaderIdExtractor, template<class> class SchemaIdExtractor, class ReadStream = plakpacs::read_stream>
	class HandlerSystem
	{
    public:
		enum class HandlerResult
		{
			Continue,
			Disconnect
		};

        template<class Schema>
        struct PacketHandlerFunction
        {
            static HandlerResult Handle(State&, const std::pair<Header, Schema>&);
        };

    protected:
        class IHandler
        {
        public:
            virtual ~IHandler() {}
            virtual HandlerResult HandlePacket(State& state, const Header& header, ReadStream& stream) const = 0;
        };

        template<class Schema>
        class HandlerImpl : public IHandler
        {
        public:
            virtual HandlerResult HandlePacket(State& state, const Header& header, ReadStream& stream) const override
            {
                auto schema = plakpacs::serializer::read<Schema>(stream);
                return PacketHandlerFunction<Schema>::Handle(state, {header, schema});
            }
        };

    public:
        class HandlerManager
        {
        public:
            static HandlerManager& GetInstance()
            {
                static HandlerManager instance;
                return instance;
            }

            void RegisterHandler(IdType type, std::shared_ptr<IHandler> handler)
            {
                if (_handlers.find(type) != _handlers.end())
                {
                    // do something?
                    return;
                }

                _handlers[type] = handler;
            }

            template<class Schema>
            void RegisterHandler()
            {
                IdType type = SchemaIdExtractor<Schema>::Extract();
                RegisterHandler(type, std::make_shared<HandlerImpl<Schema>>());
            }

            template<class RSConvertible>
            HandlerResult HandlePacket(State& state, const RSConvertible& bytes)
            {
                plakpacs::read_stream rs{ bytes };
                return HandlePacket(state, rs);
            }

            HandlerResult HandlePacket(State& state, ReadStream& rs)
            {
                return HandlePacket(state, PacketSerializer<Header>::Deserialize(rs), rs);
            }

            HandlerResult HandlePacket(State& state, const Header& header, ReadStream& rs)
            {
                auto it = _handlers.find(HeaderIdExtractor::Extract(header));

                if (it != _handlers.end())
                    return (*it).second->HandlePacket(state, header, rs);
                else
                    return HandlerResult::Continue; // might wanna throw instead
            }

        private:
            HandlerManager() = default;
            HandlerManager(const HandlerManager&) = delete;
            HandlerManager(HandlerManager&&) = delete;

            std::unordered_map<IdType, std::shared_ptr<IHandler>> _handlers;
        };

        template<class Schema>
        struct HandlerRegistrator
        {
            HandlerRegistrator<Schema>()
            {
                HandlerManager::GetInstance().template RegisterHandler<Schema>();
            }
        };
	};
}
