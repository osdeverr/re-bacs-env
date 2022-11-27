//
//  bpjson.hpp
//  bpjson
//
//  Created by Nikita Ivanov on 19.09.2020.
//  Copyright � 2020 osdever. All rights reserved.
//

#pragma once
#include <bpacs/bpacs.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <map>

namespace bpjson
{
    template<class Json>
    struct json_traits;
    
    template<class Json>
    struct basic_json_traits
    {
        using json_type = Json;
    };
    
    template<class Json>
    struct basic_cpp_json_traits : basic_json_traits<Json>
    {
        using json_type = Json;
        
        template<class S>
        static json_type& get_any_subkey(json_type& json, S key)
        {
            return json[key];
        }
        
        template<class S>
        static const json_type& get_any_subkey(const json_type& json, S key)
        {
            return json[key];
        }
        
        static const void copy(json_type& to, const json_type& from)
        {
            to = from;
        }
    };
    
    template<std::size_t KeyIndex, std::size_t ValueIndex>
    struct basic_structured_binding_kv_traits
    {
        template<class KV>
        static auto& get_kp_key(const KV& kv)
        {
            return std::get<KeyIndex>(kv);
        }
        
        template<class KV>
        static auto& get_kp_value(const KV& kv)
        {
            return std::get<ValueIndex>(kv);
        }
    };
    
	template<class Json, class T, typename = std::void_t<>>
	struct json_walker
	{
		static void read(const Json& json, T& to)
		{
            to = json_traits<Json>::template get_typed_value<T>(json);
		}

		static void write(Json& json, const T& from)
		{
            json_traits<Json>::template write_typed_value<T>(json, from);
		}
	};

	template<class Json>
	struct json_serializer
	{
		template<class T>
		static void read_to(const Json& json, T& to)
		{
			if constexpr (bpacs::has_bp_reflection<T>::value)
				read_object(json, to);
			else
				json_walker<Json, T>::read(json, to);
		}

		template<class T>
		static void write_from(Json& json, const T& from)
		{
			if constexpr (bpacs::has_bp_reflection<T>::value)
				write_object(json, from);
			else
				json_walker<Json, T>::write(json, from);
		}

		template<class T>
		static void read_object(const Json& json, T& object)
		{
			bpacs::iterate_object(
				object,
				[&json](auto field)
				{
					try
					{
						json_serializer<Json>::read_to(json_traits<Json>::get_existing_subkey(json, field.name()), field.value());
					}
					catch(const std::exception& e)
					{
						throw std::runtime_error(std::string("bpjson::json_serializer.read_object: field '") + field.name() + "' caught exception - " + e.what());
					}
					catch(...)
					{
						throw std::runtime_error(std::string("bpjson::json_serializer.read_object: field '") + field.name() + "' caught unknown exception");
					}
				}
			);
		}

		template<class T>
		static void write_object(Json& json, const T& object)
		{
			bpacs::iterate_object(
				object,
				[&json](auto field)
				{
                    try
                    {
                        json_serializer<Json>::write_from(json_traits<Json>::get_any_subkey(json, field.name()), field.value());
                    }
                    catch(const std::exception& e)
                    {
                        throw std::runtime_error(std::string("bpjson::json_serializer.write_object: field '") + field.name() + "' caught exception - " + e.what());
                    }
                    catch(...)
                    {
                        throw std::runtime_error(std::string("bpjson::json_serializer.read_object: field '") + field.name() + "' caught unknown exception");
                    }

				}
			);
		}

		template<class T>
		static T read(const Json& json)
		{
			T value;
			read_to(json, value);
			return value;
		}

		template<class T>
		static Json write(const T& value)
		{
			Json json;
			write_from(json, value);
			return json;
		}
	};

    template<class Json>
    struct json_walker<Json, std::string>
    {
        static void read(const Json& json, std::string& to)
        {
            to = json_traits<Json>::template get_typed_value<std::string>(json);
        }
        
        static void write(Json& json, const std::string& from)
        {
            json_traits<Json>::template write_typed_value<std::string>(json, from);
        }
    };
    
	template<class T, typename = std::void_t<>>
	class container_appender
	{
	public:
		using value_type = std::decay_t<decltype(*std::begin(std::declval<T>()))>;

		container_appender(T& container)
			: _container(&container)
		{}

		void append(const value_type& value)
		{
			_container->push_back(value);
		}
	private:
		T* _container;
	};

	template<class T>
	class container_appender<T, std::void_t<decltype(std::tuple_size<T>::value)>>
	{
	public:
		using value_type = std::decay_t<decltype(*std::begin(std::declval<T>()))>;

		container_appender(T& container)
			: _container(&container)
		{}

		void append(const value_type& value)
		{
			if (index < std::tuple_size_v<T>)
				(*_container)[index++] = value;
		}
	private:
		T* _container;
		size_t index = 0;
	};

	template<class Json, class T>
	struct json_walker<Json, T, std::void_t<decltype(std::begin(std::declval<T>())), decltype(std::end(std::declval<T>()))>>
	{
		using value_type = std::decay_t<decltype(*std::begin(std::declval<T>()))>;

		static void read(const Json& json, T& to)
		{
			container_appender appender{ to };
			for (auto& item : json)
				appender.append(json_serializer<Json>::template read<value_type>(item));
		}

		static void write(Json& json, const T& from)
		{
			for (auto& item : from)
				json_traits<Json>::add_array_element(json, json_serializer<Json>::template write<value_type>(item));
		}
	};

	template<class Json, class T>
	struct json_walker<Json, std::optional<T>>
	{
		static void read(const Json& json, std::optional<T>& to)
		{
            if (!json_traits<Json>::is_null(json))
				to = json_serializer<Json>::template read<T>(json);
			else
				to = std::nullopt;
		}

		static void write(Json& json, const std::optional<T>& from)
		{
			if (from.has_value())
				json_serializer<Json>::write_from(json, *from);
			else
                json_traits<Json>::make_null(json);
		}
	};

	template<class Json>
	struct json_walker<Json, Json>
	{
		static void read(const Json& json, Json& to)
		{
            json_traits<Json>::copy(to, json);
		}

		static void write(Json& json, const Json& from)
		{
            json_traits<Json>::copy(json, from);
		}
	};

	template<class Json, class T>
	struct json_walker<Json, std::map<std::string, T>>
	{
		static void read(const Json& json, std::map<std::string, T>& to)
		{
			for (auto& j : json_traits<Json>::get_keys(json))
            {
                auto key = json_traits<Json>::get_kp_key(j);
                auto subkey = json_traits<Json>::get_existing_subkey(json, key);
                json_serializer<Json>::read_to(json_traits<Json>::get_kp_value(j), to[key]);
            }
		}

		static void write(Json& json, const std::map<std::string, T>& from)
		{
			for (auto& pair : from)
				json_serializer<Json>::write_from(json_traits<Json>::get_any_subkey(json, pair.first), pair.second);
		}
	};
}