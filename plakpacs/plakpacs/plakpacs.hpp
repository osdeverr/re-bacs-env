//
//  plakpacs.hpp
//  bpacs
//
//  Created by Nikita Ivanov on 02.09.2020.
//  Copyright © 2020 osdever. All rights reserved.
//

#pragma once
#include <bpacs/bpacs.hpp>
#include <vector>
#include <list>
#include <array>
#include <string>
#include <optional>
#include <stdexcept>

namespace plakpacs
{
    /// @brief A helper type to define a size-prefixed container. Does nothing by itself, forwarding everything to the actual container class; used as a tag by the serialization system.
    /// @tparam T The actual container type. Works for anything with a @code value_type.
    template<class T>
    class sp_container : public T
    {
    public:
        /// @brief Used to distinguish SP containers from regular containers in SFINAE.
        using is_sp_container = void;
        
        /// @brief Forwards non-initializer-list construction to the container.
        template<class... Args>
        sp_container(Args&&... args)
        : T(std::forward<Args>(args)...)
        {}
        
        /// @brief Forwards initializer-list construction to the container.
        sp_container(std::initializer_list<typename T::value_type> args)
        : T(args)
        {}
    };
    
    /// @brief An alias to a size-prefixed vector type.
    /// @tparam T The value type of this vector
    /// @tparam A The allocator for this vector (default: std::allocator<T>)
    template<class T, class A = typename std::allocator<T>>
    using sp_vector = sp_container<std::vector<T, A>>;

    /// @brief An alias to a size-prefixed list type.
    /// @tparam T The value type of this list
    /// @tparam A The allocator for this list (default: std::allocator<T>)
    template<class T, class A = typename std::allocator<T>>
    using sp_list = sp_container<std::list<T>>;

    /// @brief An alias to a size-prefixed safe array type.
    /// @tparam T The value type of this array
    /// @tparam N The size of this array
    template<class T, size_t N>
    using sp_array = sp_container<std::array<T, N>>;
    
    /// @brief An alias to a size-prefixed string type.
    /// @tparam CharT The character type
    /// @tparam Traits The char traits type
    /// @tparam Alloc The allocator type
    template<class CharT = char, class Traits = std::char_traits<CharT>, class Alloc = std::allocator<CharT>>
    using sp_basic_string = sp_container<std::basic_string<CharT, Traits, Alloc>>;
    
    using sp_string = sp_basic_string<char>;
    using sp_wstring = sp_basic_string<wchar_t>;

    /// @brief A simple SFINAE check whether a type is an SP container.
    /// @tparam T The type to check
    template<class T, typename = std::void_t<>>
    struct is_sp_container : std::false_type
    {};
    
    /// @brief An is_sa_container<T> specialization for SP containers.
    /// @tparam T The type to check
    template<class T>
    struct is_sp_container<T, std::void_t<typename T::is_sp_container>> : std::true_type
    {};
    
    /// @brief A helper class to put constraints on a type. Provides a method to check if the current instance satisfies the constraints.
    /// @tparam T The constrained type
    /// @tparam Cs The constraints to apply (may be empty)
    template<class T, class... Cs>
    class constrained : public T
    {
    public:
        /// @brief Forwards non-initializer-list construction to the constrained type.
        template<class... Args>
        constrained(Args&&... args)
        : T(std::forward<Args>(args)...)
        {}
        
        bool check_pp_constraints() const
        {
            const auto& obj = static_cast<const T&>(*this);
            return (Cs::check(obj) && ...);
        }
        
        /*
        /// @brief Forwards initializer-list construction to the container.
        constrained(std::initializer_list<typename T::value_type> args)
        : T(args)
        {}
         */
    };
    
    /// @brief A constraint to compare the size of a container to a number.
    /// @tparam N The number to test the container size against
    /// @tparam Comparison The comparison class to use (std::less, std::greater, etc...)
    template<std::size_t N, class Comparison>
    struct container_size_constraint
    {
        template<class T>
        static bool check(const T& container)
        {
            auto result = Comparison{}(std::size(container), N);
            return result;
        }
    };
    
    /// @brief A constraint for the minimum size of a container.
    /// @tparam N The minimum size to test against
    template<std::size_t N>
    using min_container_size = container_size_constraint<N, std::greater_equal<>>;
    
    /// @brief A constraint for the maximum size of a container.
    /// @tparam N The maximum size to test against
    template<std::size_t N>
    using max_container_size = container_size_constraint<N, std::less_equal<>>;
    
    /// @brief A constraint for the exact size of a container.
    /// @tparam N The exact size to test against
    template<std::size_t N>
    using exact_container_size = container_size_constraint<N, std::equal_to<>>;
    
    /// @brief A constraint for limiting the size of a container to a defined range.
    /// @tparam Min The minimum size to test agsinst
    /// @tparam Max The maximum size to test agsinst
    /// @tparam MinComparison The minimum size comparison class
    /// @tparam MaxComparison The maximum size comparison class
    template<std::size_t Min,
             std::size_t Max,
             class MinComparison = std::greater_equal<>,
             class MaxComparison = std::less_equal<>
            >
    struct container_size_limit
    {
        using min_constraint = container_size_constraint<Min, MinComparison>;
        using max_constraint = container_size_constraint<Max, MaxComparison>;
        
        template<class T>
        static bool check(const T& container)
        {
            return min_constraint::check(container) && max_constraint::check(container);
        }
    };
    
    /// @brief A basic "binary walker" type with static methods to read and write a value of a certain type from a stream. This default implementation forwards both write and read operations to the stream; to define more complex behavior, specialize this template with your type.
    /// @tparam Stream The stream type to use
    /// @tparam T The type values of which to read/write
    template<class Stream, class T, typename = std::void_t<>>
    struct binary_walker
    {
        /// @brief Writes a value to a stream. The default implementation forwards all write operations to the stream without doing anything extra.
        /// @param stream The stream to write to
        /// @param value A const reference to the value to write
        static void write(Stream& stream, const T& value)
        {
            stream.write(value);
        }
        
        /// @brief Reads a value from a stream. The default implementation forwards all read operations to the stream without doing anything extra.
        /// @param stream The stream to read from
        /// @param value A reference to read a value to
        static void read(Stream& stream, T& value)
        {
            stream.read(value);
        }
    };

    /// @brief A simple serializer class to read and write values & objects from streams.
    struct serializer
    {
        /// @brief Writes a value to a stream. Writes an object recursively if it's possible, AKA if the object is BPACS-reflectable.
        /// @tparam Stream The stream type to use
        /// @tparam T The type values of which to write
        /// @param stream The stream to write to
        /// @param value A const reference to the value to write
        template<class Stream, class T>
        static void write(Stream& stream, const T& value)
        {
            if constexpr(bpacs::has_bp_reflection<T>::value)
                write_object(stream, value);
            else
                binary_walker<Stream, T>::write(stream, value);
        }
        
        /// @brief Writes an object to a stream recursively, iterating through its BPACS-visible fields and writing them.
        /// @tparam Stream The stream type to use
        /// @tparam T The type objects of which to write
        /// @param stream The stream to write to
        /// @param object A const reference to the object to write
        template<class Stream, class T>
        static void write_object(Stream& stream, const T& object)
        {
            static_assert(bpacs::has_bp_reflection<T>::value == true, "write_object only supports BPACS-reflectable objects");
            
            bpacs::iterate_object(object,
                                  [&](auto field)
                                  {
                                      try
                                      {
                                          write(stream, field.value());
                                      }
                                      catch(const std::exception& e)
                                      {
                                          throw std::runtime_error(std::string("plakpacs::serializer.write_object: field '") + field.holder() + "." + field.name() + "' caught exception - " + e.what());
                                      }
                                      catch(...)
                                      {
                                          throw std::runtime_error(std::string("plakpacs::serializer.write_object: field '") + field.holder() + "." + field.name() + "' caught unknown exception");
                                      }
                                  });
        }
        
        /// @todo Nothing is there yet...
        template<class Stream, class T>
        static void read(Stream& stream, T& value)
        {
            if constexpr(bpacs::has_bp_reflection<T>::value)
                read_object(stream, value);
            else
                binary_walker<Stream, T>::read(stream, value);
        }
        
        /// @todo Nothing is there yet...
        template<class T, class Stream>
        static T read(Stream& stream)
        {
            T value;
            read(stream, value);
            return value;
        }
        
        /// @brief Reads an object from a stream recursively, iterating through its BPACS-visible fields and reading them.
        /// @tparam Stream The stream type to use
        /// @tparam T The type objects of which to read
        /// @param stream The stream to read from
        /// @param object A reference to the object to read into
        template<class Stream, class T>
        static void read_object(Stream& stream, T& object)
        {
            static_assert(bpacs::has_bp_reflection<T>::value == true, "read_object only supports BPACS-reflectable objects");
            
            bpacs::iterate_object(object,
                                  [&](auto field)
                                  {
                                      try
                                      {
                                          read(stream, field.value());
                                      }
                                      catch(const std::exception& e)
                                      {
                                          throw std::runtime_error(std::string("plakpacs::serializer.read_object: field '") + field.holder() + "." + field.name() + "' caught exception - " + e.what());
                                      }
                                      catch(...)
                                      {
                                          throw std::runtime_error(std::string("plakpacs::serializer.read_object: field '") + field.holder() + "." + field.name() + "' caught unknown exception");
                                      }
                                  });
        }
    };
    
    /// @brief Specializes binary_walker for C-style strings.
    /// @tparam Stream The stream type to use
    template<class Stream>
    struct binary_walker<Stream, const char*>
    {
        /// @brief Converts the C-style string pointer to an std::string and writes the resulting string to the stream.
        static void write(Stream& stream, const char* const& sz)
        {
            serializer::write(stream, std::string(sz));
        }
        
        static void read(Stream& stream, const char*& value)
        {
            throw std::runtime_error("Can't read C-style strings in plakpacs: please switch to the appropriate C++ counterpart");
        }
    };
    
    
    /// @brief Specializes binary_walker for C++ strings. Prevents them being treated as containers and written character-by-character which is obviously slow.
    /// @tparam Stream The stream type to use
    template<class Stream>
    struct binary_walker<Stream, std::string>
    {
        /// @brief Writes the string "as-is" to the stream.
        static void write(Stream& stream, const std::string& string)
        {
            stream.write(string.begin(), string.end());
            stream.write('\0');
        }
        
        static void read(Stream& stream, std::string& value)
        {
            while(auto c = stream.template read<char>())
            {
                value += c;
            }
        }
    };
    
    /// @brief Appends values to a container.
    template<class T, typename = std::void_t<>>
    class container_appender
    {
    public:
        using value_type = typename T::value_type;
        
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
    
    /// @brief A container_appender specialization for fixed-sized containers.
    template<class T>
    class container_appender<T, std::void_t<decltype(std::tuple_size<T>::value)>>
    {
    public:
        using value_type = typename T::value_type;
        
        container_appender(T& container)
        : _container(&container)
        {}
        
        void append(const value_type& value)
        {
            if(index < std::tuple_size<T>::value)
                (*_container)[index++] = value;
        }
    private:
        T* _container;
        size_t index = 0;
    };

    /// @brief Specializes binary_walker for @b all containers supporting std::begin/end. This includes all STL containers and every other class which can be iterated, with the exception of std::string.
    /// @tparam Stream The stream type to use
    /// @tparam T The container type
    template<class Stream, class T>
    struct binary_walker<Stream, T, std::void_t<decltype(std::begin(std::declval<T>())), decltype(std::end(std::declval<T>()))>>
    {
        /// @brief Writes the container's elements to the stream one-by-one.
        static void write(Stream& stream, const T& container)
        {
            for(auto& value : container)
                serializer::write(stream, value);
        }
        
        // The template parameter constrains reading non-SP containers to those whose size is known at compile-time
        static void read(Stream& stream, T& container)
        {
            constexpr auto N = std::tuple_size<T>::value;
            container_appender appender{container};
            for(size_t i = 0; i < N; i++)
            {
                typename T::value_type value;
                serializer::read(stream, value);
                appender.append(value);
            }
        }
    };
    
    /// @brief Specializes binary_walker for size-prefixed containers.
    template<class Stream, class T>
    struct binary_walker<Stream, sp_container<T>>
    {
        /// @brief Writes the container's size to the stream, then writes the container itself.
        static void write(Stream& stream, const sp_container<T>& container)
        {
            serializer::write<Stream, std::uint32_t>(stream, (std::uint32_t) std::size(container));
            serializer::write<Stream, T>(stream, container);
        }
        
        static void read(Stream& stream, sp_container<T>& container)
        {
            std::uint32_t size = stream.template read<std::uint32_t>();

            // Hack
            if (size > 65536)
                throw std::runtime_error("plakpacs::binary_walker<Stream, sp_container<T>>.read() => Invalid container size...");

            container_appender appender{container};
            for(size_t i = 0; i < size; i++)
            {
                typename T::value_type value;
                serializer::read(stream, value);
                appender.append(value);
            }

            // VERY BAD HACK: Silently ignore the last character of any std::string SP containers. They were not read at all before.
            // This horrendous bug has been sitting UNNOTICED for almost precisely one year, in theory preventing the reading of ANY sp_strings.
            if (std::is_same_v<T, std::string>)
                (void) serializer::read<char>(stream);
        }
    };
    
    /// @brief Specializes binary_walker for optional values.
    template<class Stream, class T>
    struct binary_walker<Stream, std::optional<T>>
    {
        /// @brief Writes a boolean value to the stream indicating whether the optional has a value. If it does have a value, writes its value to the stream as well.
        static void write(Stream& stream, const std::optional<T>& optional)
        {
            bool has = optional.has_value();
            serializer::write<Stream, bool>(stream, has);
            if(has)
                serializer::write<Stream, T>(stream, *optional);
        }
        
        static void read(Stream& stream, std::optional<T>& optional)
        {
            // HACK: we might wanna remove this later...
            if (!stream.can_read())
                return;

            bool has = stream.template read<bool>();
            if(has)
            {
                T value;
                serializer::read(stream, value);
                optional = value;
            }
        }
    };
    
    template<class Stream, class T, class... Cs>
    struct binary_walker<Stream, constrained<T, Cs...>>
    {
        static void write(Stream& stream, const constrained<T, Cs...>& value)
        {
            serializer::write<Stream, T>(stream, value);
        }
        
        static void read(Stream& stream, constrained<T, Cs...>& value)
        {
            serializer::read<Stream, T>(stream, value);
            
            if(!value.check_pp_constraints())
                throw std::runtime_error("Constraint not satisfied");
        }
    };
    
    class write_stream
    {
    public:
        write_stream()
        {
            _bytes.reserve(128);
        }

        template<class T>
        void write(const T& value)
        {
            std::array<uint8_t, sizeof(T)> vbytes;
            *(T*) vbytes.data() = value;
            
            _bytes.insert(_bytes.end(), vbytes.begin(), vbytes.end());
        }
        
        template<class Iter>
        void write(Iter begin, Iter end)
        {
            _bytes.insert(_bytes.end(), begin, end);
        }

        void write(unsigned char value)
        {
            _bytes.push_back(value);
        }

        void write(char value)
        {
            _bytes.push_back(value);
        }
        
        const std::vector<uint8_t>& bytes() const
        {
            return _bytes;
        }
        
    private:
        std::vector<uint8_t> _bytes;
    };
    
    class read_stream
    {
    public:
        template<class Iter>
        read_stream(Iter begin, Iter end)
        : _bytes(begin, end)
        {}
        
        template<class Container>
        read_stream(const Container& container)
        : _bytes(std::begin(container), std::end(container))
        {}
        
        template<class T>
        void read(T& value)
        {
            if constexpr (std::is_empty_v<T>)
            {
                value = T{};
                return;
            }

            if (!can_read_num(sizeof(T)))
                throw std::runtime_error("plakpacs::read_stream.read<T>() => Can't read past the end of the stream");

            value = *(T*) (_bytes.data() + _position);
            _position += sizeof(T);
        }
        
        template<class T>
        T read()
        {
            T value;
            read(value);
            return value;
        }

        bool can_read_num(std::size_t num) const
        {
            return _position <= _bytes.size() - num;
        }
        
        bool can_read() const
        {
            return can_read_num(1);
        }
        
        const std::vector<uint8_t>& bytes() const
        {
            return _bytes;
        }
        
        size_t position() const
        {
            return _position;
        }
        
    private:
        std::vector<uint8_t> _bytes;
        size_t _position = 0;
    };
}
