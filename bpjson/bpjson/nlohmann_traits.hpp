//
//  nlohmann_traits.hpp
//  bpjson
//
//  Created by Nikita Ivanov on 15.03.2021.
//

#pragma once
#include <nlohmann/json.hpp>

#define BPJSON_NLOHMANN_BASIC_JSON_TPL_DECL                                                                            \
    template <template <typename, typename, typename...> class ObjectType,                                             \
              template <typename, typename...> class ArrayType, class StringType, class BooleanType,                   \
              class NumberIntegerType, class NumberUnsignedType, class NumberFloatType,                                \
              template <typename> class AllocatorType, template <typename, typename = void> class JSONSerializer>

#define BPJSON_NLOHMANN_BASIC_JSON_TPL                                                                                 \
    nlohmann::basic_json<ObjectType, ArrayType, StringType, BooleanType, NumberIntegerType, NumberUnsignedType,        \
                         NumberFloatType, AllocatorType, JSONSerializer>

namespace bpjson
{
    BPJSON_NLOHMANN_BASIC_JSON_TPL_DECL
    struct json_traits<BPJSON_NLOHMANN_BASIC_JSON_TPL> : basic_cpp_json_traits<BPJSON_NLOHMANN_BASIC_JSON_TPL>
    {
        using json_type = BPJSON_NLOHMANN_BASIC_JSON_TPL;

        ////////////////////////////////////////////////////////////////////

        template <class T>
        static T get_typed_value(const json_type &json)
        {
            return json.template get<T>();
        }

        template <class T>
        static void write_typed_value(json_type &json, const T &value)
        {
            json = value;
        }

        ////////////////////////////////////////////////////////////////////

        template <class S>
        static bool subkey_exists(const json_type &json, S key)
        {
            return json.find(key) != json.end();
        }

        template <class S>
        static json_type &get_existing_subkey(json_type &json, S key)
        {
            return json.at(key);
        }

        template <class S>
        static const json_type &get_existing_subkey(const json_type &json, S key)
        {
            return json.at(key);
        }

        ////////////////////////////////////////////////////////////////////

        template <class S>
        static json_type &get_any_subkey(json_type &json, S key)
        {
            return json[key];
        }

        template <class S>
        static const json_type &get_any_subkey(const json_type &json, S key)
        {
            return json[key];
        }

        ////////////////////////////////////////////////////////////////////

        static void add_array_element(json_type &json, const json_type &element)
        {
            json.push_back(element);
        }

        ////////////////////////////////////////////////////////////////////

        static bool is_null(const json_type &json)
        {
            return json.is_null();
        }

        static void make_null(json_type &json)
        {
            json = {};
        }

        ////////////////////////////////////////////////////////////////////

        static auto get_keys(json_type &json)
        {
            return json.items();
        }

        static auto get_keys(const json_type &json)
        {
            return json.items();
        }

        template <class KV>
        static auto &get_kp_key(const KV &kv)
        {
            return kv.key();
        }

        template <class KV>
        static auto &get_kp_value(const KV &kv)
        {
            return kv.value();
        }
    };
} // namespace bpjson

// Cleanup
#undef BPJSON_NLOHMANN_BASIC_JSON_TPL
#undef BPJSON_NLOHMANN_BASIC_JSON_TPL_DECL
