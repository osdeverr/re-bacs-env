//
//  bacs.hpp
//  bacs
//
//  Created by Nikita Ivanov on 05.09.2020.
//  Copyright © 2020 osdever. All rights reserved.
//

#pragma once
#include <boost/asio.hpp>
#include <thread>
#include <utility>

#ifndef BACS_CONFIG_HOSTNAME
#define BACS_CONFIG_HOSTNAME "127.0.0.1"
#endif

#ifndef BACS_CONFIG_HOSTPORT
#define BACS_CONFIG_HOSTPORT 1551
#endif

namespace bacs
{
    constexpr auto NetHostName = BACS_CONFIG_HOSTNAME;
    constexpr auto NetHostPort = BACS_CONFIG_HOSTPORT;
    
    /// @brief Encapsules an IO context's run loop thread.
    template<class IOContext>
    class io_worker
    {
    public:
        io_worker(IOContext& context)
        : _context(context)
        {}
        
        io_worker(const io_worker&) = delete;
        io_worker(io_worker&&) = default;
        
        void run()
        {
            _worker.join();
        }
        
        ~io_worker()
        {
            _context.get().stop();
            _worker.join();
        }
        
    private:
        std::reference_wrapper<IOContext> _context;
        std::thread _worker = std::thread([this]
                                          {
                                              _context.get().run();
                                          });
        
    };

    /// @brief Encapsulates an IO context's run loop thread pool.
    template<class IOContext>
    class io_worker_pool
    {
    public:
        io_worker_pool(IOContext& context, std::size_t num_threads)
        : _context(context)
        {
            for (std::size_t i = 0; i < num_threads; i++)
                _workers.push_back(
                    std::thread([this]
                        {
                            _context.get().run();
                        })
                );
        }

        io_worker_pool(const io_worker_pool&) = delete;
        io_worker_pool(io_worker_pool&&) = default;

        ~io_worker_pool()
        {
            _context.get().stop();

            for (auto& worker : _workers)
                worker.join();
        }

    private:
        std::reference_wrapper<IOContext> _context;
        std::vector<std::thread> _workers;

    };
    
    class shared_buffer
    {
    public:
        shared_buffer(std::size_t size)
        : _data(new char[size]), _size(size)
        {}
        
        shared_buffer(const shared_buffer&) = default;
        shared_buffer(shared_buffer&&) = default;
        
        void* data()
        {
            return _data.get();
        }
        
        const void* data() const
        {
            return _data.get();
        }
        
        std::size_t size() const
        {
            return _size;
        }
        
        uint8_t* begin()
        {
            return (uint8_t*) _data.get();
        }
        
        const uint8_t* begin() const
        {
            return (uint8_t*) _data.get();
        }
        
        uint8_t* end()
        {
            return begin() + _size;
        }
        
        const uint8_t* end() const
        {
            return begin() + _size;
        }
        
    private:
        std::shared_ptr<void> _data;
        std::size_t _size;
    };

    struct sp_default
    {
        using size_type = std::uint32_t;

        static void in(size_type&)
        {}

        static void out(size_type&)
        {}
    };
    
    template<class SPTraits = sp_default, class Socket, class Container, class Handler>
    void async_write_sp(Socket& socket, const Container& container, Handler&& handler)
    {
        using size_type = typename SPTraits::size_type;

        auto size = (size_type)(std::size(container) * sizeof(*std::data(container)));
        SPTraits::out(size);

        auto pSize = std::make_shared<size_type>(size);
        auto pContainer = std::make_shared<Container>(container);
        
        boost::asio::async_write(socket,
                                 boost::asio::const_buffer(pSize.get(), sizeof(*pSize)),
                                 [&socket, pContainer, handler] (const boost::system::error_code& ec, std::size_t bytes_transferred)
                                 {
                                     if(ec.failed())
                                     {
                                         handler(ec, bytes_transferred);
                                         return;
                                     }
                                     
                                     boost::asio::async_write(socket,
                                                              boost::asio::const_buffer(std::data(*pContainer), std::size(*pContainer)),
                                                              [handler](const boost::system::error_code& ec, std::size_t bytes_transferred)
                                                              {
                                                                  handler(ec, bytes_transferred);
                                                              });
                                 });
    }
    
    template<class SPTraits = sp_default, class Socket, class Handler>
    void async_read_sp(Socket& socket, Handler&& handler)
    {
        using size_type = typename SPTraits::size_type;
        auto pSize = std::make_shared<size_type>();
        
        boost::asio::async_read(socket,
                                boost::asio::mutable_buffer(pSize.get(), sizeof(*pSize)),
                                [&socket, pSize, handler] (const boost::system::error_code& ec, std::size_t bytes_read)
                                {
                                    if(ec.failed())
                                    {
                                        handler(ec, bytes_read, shared_buffer{0});
                                        return;
                                    }
                                    
                                    try
                                    {
                                        auto size = *pSize;
                                        SPTraits::in(size);
                                        shared_buffer buf{ size };

                                        boost::asio::async_read(socket,
                                            boost::asio::mutable_buffer(buf.data(), buf.size()),
                                            [buf, handler](const boost::system::error_code& ec, std::size_t bytes_read)
                                            {
                                                handler(ec, bytes_read, buf);
                                            });
                                    }
                                    catch (...)
                                    {
                                        handler(ec, bytes_read, shared_buffer{ 0 });
                                        return;
                                    }
                                });
    }
    
    template<class SPTraits = sp_default, class Socket, class Handler>
    void async_read_sp_loop(Socket& socket, Handler&& handler)
    {
        async_read_sp<SPTraits>(socket,
                      [&socket, handler](const boost::system::error_code& ec, std::size_t bytes_read, bacs::shared_buffer buffer)
                      {
                          if(handler(ec, bytes_read, buffer))
                              async_read_sp_loop<SPTraits>(socket, handler);
                      }
                      );
    }
    
    template<class Acceptor, class Socket, class Handler>
    void async_accept_loop(Acceptor& acceptor, Socket& socket, Handler&& handler)
    {
        acceptor.async_accept(socket,
                              [&acceptor, &socket, handler](const boost::system::error_code& ec)
                              {
                                  handler(ec);
                                  
                                  if(!ec.failed())
                                      async_accept_loop(acceptor, socket, handler);
                              });
    }
}

#undef BACS_CONFIG_HOSTPORT
#undef BACS_CONFIG_HOSTNAME
