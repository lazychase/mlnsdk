#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <net/logger.hpp>

#include <map>

namespace mln::net {
    class HttpServer
    {
    public:
        class SendFunc;
        using Url = std::string;
        using HeaderMap = std::map<std::string, std::string>;
        using HandlerType = std::function<void(HeaderMap&&, std::string&&, SendFunc&)>;
        using HandlerMap = std::map<Url, HandlerType>;
        using RegisterFunc = std::function<void(HandlerType handler)>;

        static HttpServer& instance() { static HttpServer _instance; return _instance; }

        class SendFunc
        {
        public:
            uint32_t _version;
            bool _keepAlive;

            explicit
                SendFunc()
                : _version(11)
                , _keepAlive(false)
            {
            }

            void operator()(uint32_t errorCode) const
            {
                sendError(boost::beast::http::int_to_status(errorCode));
            }

            void operator()(boost::beast::http::status errorCode) const
            {
                sendError(errorCode);
            }

            void operator()(std::string&& body) const
            {
                boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::ok, _version };
                res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(boost::beast::http::field::content_type, "application/json");
                res.keep_alive(_keepAlive);
                res.body() = std::move(body);
                res.prepare_payload();

                send(std::move(res));
            }

            void operator()(HeaderMap&& headerMap, std::string&& body) const
            {
                boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::ok, _version };

                for (const auto& [name, value] : headerMap)
                {
                    res.set(name, value);
                }

                res.keep_alive(_keepAlive);
                res.body() = std::move(body);
                res.prepare_payload();

                send(std::move(res));
            }

            void operator()(boost::beast::http::response<boost::beast::http::string_body>&& msg) const
            {
                send(std::move(msg));
            }

        private:
            void sendError(boost::beast::http::status errorCode) const
            {
                // Respond to GET request
                //boost::beast::http::response<boost::beast::http::string_body> res{
                //    std::piecewise_construct,
                //    std::make_tuple(""),
                //    //std::make_tuple(std::move(body)),
                //    std::make_tuple(errorCode, _version) };
                //res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
                //res.set(boost::beast::http::field::content_type, "application/json");
                //res.content_length(0);
                ////res.content_length(body.size());
                //res.keep_alive(_keepAlive);

                boost::beast::http::response<boost::beast::http::string_body> res{ errorCode, _version };
                res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(boost::beast::http::field::content_type, "text/html");
                res.keep_alive(_keepAlive);
                //res.body() = "{\"ErrorCode\": " + std::to_string(static_cast<typename std::underlying_type_t<boost::beast::http::status>>(errorCode)) + "}";
                res.prepare_payload();

                send(std::move(res));
            }

            virtual void send(boost::beast::http::response<boost::beast::http::string_body>&& msg) const = 0;
        };

        class SendFuncSync : public SendFunc
        {
        public:
            boost::asio::ip::tcp::socket& _socket;
            bool& _close;
            boost::beast::error_code& _ec;

            explicit SendFuncSync(
                boost::asio::ip::tcp::socket& socket,
                bool& close,
                boost::beast::error_code& ec)
                : _socket(socket)
                , _close(close)
                , _ec(ec)
            {
            }

            virtual void send(boost::beast::http::response<boost::beast::http::string_body>&& msg) const
            {
                // Determine if we should close the connection after
                _close = msg.need_eof();

                // We need the serializer here because the serializer requires
                // a non-const file_body, and the message oriented version of
                // http::write only works with const messages.
                boost::beast::http::serializer<false, boost::beast::http::string_body, boost::beast::http::fields> sr{ msg };
                boost::beast::http::write(_socket, sr, _ec);
            }
        };

        class session;
        class SendFuncAsync : public SendFunc
        {
        private:
            session& _session;

        public:
            explicit SendFuncAsync(session& session)
                : _session(session)
            {
            }

            virtual void send(boost::beast::http::response<boost::beast::http::string_body>&& msg) const
            {
                // The lifetime of the message has to extend
                // for the duration of the async operation so
                // we use a shared_ptr to manage it.
                auto sp = std::make_shared<
                    boost::beast::http::message<false, boost::beast::http::string_body, boost::beast::http::fields>>(std::move(msg));

                // Store a type-erased version of the shared
                // pointer in the class to keep it alive.
                _session.res_ = sp;

                // Write the response
                boost::beast::http::async_write(
                    _session.stream_,
                    *sp,
                    boost::beast::bind_front_handler(
                        &session::on_write,
                        _session.shared_from_this(),
                        sp->need_eof()));
            }
        };

        /*
        struct SendFunc
        {
            boost::asio::ip::tcp::socket& _socket;
            bool& _close;
            boost::beast::error_code& _ec;
            uint32_t _version;
            bool _keepAlive;

            explicit
                SendFunc(
                    boost::asio::ip::tcp::socket& socket,
                    bool& close,
                    boost::beast::error_code& ec)
                : _socket(socket)
                , _close(close)
                , _ec(ec)
                , _version(11)
                , _keepAlive(false)
            {
            }

            void operator()(uint32_t errorCode) const
            {
                sendError(boost::beast::http::int_to_status(errorCode));
            }

            void operator()(boost::beast::http::status errorCode) const
            {
                sendError(errorCode);
            }

            void operator()(std::string&& body) const
            {
                //// Respond to GET request
                //boost::beast::http::response<boost::beast::http::string_body> res{
                //    std::piecewise_construct,
                //    std::make_tuple(std::move(body)),
                //    std::make_tuple(boost::beast::http::status::ok, _version) };
                //res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
                //res.set(boost::beast::http::field::content_type, "application/json");
                //res.content_length(body.size());
                //res.keep_alive(_keepAlive);

                boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::ok, _version };
                res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(boost::beast::http::field::content_type, "application/json");
                res.keep_alive(_keepAlive);
                res.body() = std::move(body);
                res.prepare_payload();

                send(std::move(res));
            }

            void operator()(HeaderMap&& headerMap, std::string&& body) const
            {
                boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::ok, _version };

                for (const auto& [name, value] : headerMap)
                {
                    res.set(name, value);
                }

                res.keep_alive(_keepAlive);
                res.body() = std::move(body);
                res.prepare_payload();

                send(std::move(res));
            }

            template<bool isRequest, class Body, class Fields>
            void operator()(boost::beast::http::message<isRequest, Body, Fields>&& msg) const
            {
                send<isRequest, Body, Fields>(std::move(msg));
            }

        private:
            void sendError(boost::beast::http::status errorCode) const
            {
                boost::beast::http::response<boost::beast::http::string_body> res{ errorCode, _version };
                res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(boost::beast::http::field::content_type, "application/json");
                res.keep_alive(_keepAlive);
                //res.body() = "{\"ErrorCode\": " + std::to_string(static_cast<typename std::underlying_type_t<boost::beast::http::status>>(errorCode)) + "}";
                res.prepare_payload();

                send(std::move(res));
            }

            template<bool isRequest, class Body, class Fields>
            void send(boost::beast::http::message<isRequest, Body, Fields>&& msg) const
            {
                // Determine if we should close the connection after
                _close = msg.need_eof();

                // We need the serializer here because the serializer requires
                // a non-const file_body, and the message oriented version of
                // http::write only works with const messages.
                boost::beast::http::serializer<isRequest, Body, Fields> sr{ msg };
                boost::beast::http::write(_socket, sr, _ec);
            }
        };
        //*/

        // Handles an HTTP server connection
        class session : public std::enable_shared_from_this<session>
        {
            friend class SendFuncAsync;

            boost::beast::tcp_stream stream_;
            boost::beast::flat_buffer buffer_;
            std::shared_ptr<std::string const> doc_root_;
            boost::beast::http::request<boost::beast::http::string_body> req_;
            std::shared_ptr<void> res_;
            SendFuncAsync lambda_;
            HandlerMap& handlers_;

        public:
            // Take ownership of the stream
            session(
                boost::asio::ip::tcp::socket&& socket,
                std::shared_ptr<std::string const> const& doc_root,
                HandlerMap& handlers)
                : stream_(std::move(socket))
                , doc_root_(doc_root)
                , lambda_(*this)
                , handlers_(handlers)
            {
            }

            // Start the asynchronous operation
            void run()
            {
                // We need to be executing within a strand to perform async operations
                // on the I/O objects in this session. Although not strictly necessary
                // for single-threaded contexts, this example code is written to be
                // thread-safe by default.
                boost::asio::dispatch(stream_.get_executor(),
                    boost::beast::bind_front_handler(
                        &session::do_read,
                        shared_from_this()));
            }

            void do_read()
            {
                // Make the request empty before reading,
                // otherwise the operation behavior is undefined.
                req_ = {};

                // Set the timeout.
                stream_.expires_after(std::chrono::seconds(30));

                // Read a request
                boost::beast::http::async_read(stream_, buffer_, req_,
                    boost::beast::bind_front_handler(
                        &session::on_read,
                        shared_from_this()));
            }

            void on_read(
                boost::beast::error_code ec,
                std::size_t bytes_transferred)
            {
                boost::ignore_unused(bytes_transferred);

                // This means they closed the connection
                if (ec == boost::beast::http::error::end_of_stream)
                    return do_close();

                if (ec)
                    return fail(ec, "read");

                // Send the response
                handle_request(*doc_root_, handlers_, std::move(req_), lambda_);
            }

            void on_write(
                bool close,
                boost::beast::error_code ec,
                std::size_t bytes_transferred)
            {
                boost::ignore_unused(bytes_transferred);

                if (ec)
                    return fail(ec, "write");

                if (close)
                {
                    // This means we should close the connection, usually because
                    // the response indicated the "Connection: close" semantic.
                    return do_close();
                }

                // We're done with the response so delete it
                res_ = nullptr;

                // Read another request
                do_read();
            }

            void do_close()
            {
                // Send a TCP shutdown
                boost::beast::error_code ec;
                stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

                // At this point the connection is closed gracefully
            }
        };

        // Accepts incoming connections and launches the sessions
        class listener : public std::enable_shared_from_this<listener>
        {
            boost::asio::io_context& ioc_;
            boost::asio::ip::tcp::acceptor acceptor_;
            std::shared_ptr<std::string const> doc_root_;
            HandlerMap& handlers_;

        public: listener(
            boost::asio::io_context& ioc,
            boost::asio::ip::tcp::endpoint endpoint,
            std::shared_ptr<std::string const> const& doc_root,
            HandlerMap& handlers)
            : ioc_(ioc)
            , acceptor_(boost::asio::make_strand(ioc))
            , doc_root_(doc_root)
            , handlers_(handlers)
        {
            boost::beast::error_code ec;

            // Open the acceptor
            acceptor_.open(endpoint.protocol(), ec);
            if (ec)
            {
                fail(ec, "open");
                return;
            }

            // Allow address reuse
            acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
            if (ec)
            {
                fail(ec, "set_option");
                return;
            }

            // Bind to the server address
            acceptor_.bind(endpoint, ec);
            if (ec)
            {
                fail(ec, "bind");
                return;
            }

            // Start listening for connections
            acceptor_.listen(
                boost::asio::socket_base::max_listen_connections, ec);
            if (ec)
            {
                fail(ec, "listen");
                return;
            }
        }

              // Start accepting incoming connections
              void run()
              {
                  do_accept();
              }

        private:
            void do_accept()
            {
                // The new connection gets its own strand
                acceptor_.async_accept(
                    boost::asio::make_strand(ioc_),
                    boost::beast::bind_front_handler(
                        &listener::on_accept,
                        shared_from_this()));
            }

            void on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket)
            {
                if (ec)
                {
                    fail(ec, "accept");
                }
                else
                {
                    // Create the session and run it
                    std::make_shared<session>(
                        std::move(socket),
                        doc_root_,
                        handlers_)->run();
                }

                // Accept another connection
                do_accept();
            }
        };

    private:
        void AcceptSync(const uint16_t port, std::shared_ptr<boost::asio::io_context> ioc) {
            try {
                auto const address = boost::asio::ip::make_address("0.0.0.0");
                auto const doc_root = std::make_shared<std::string>("");

                boost::asio::ip::tcp::acceptor acceptor{ *ioc.get(), {address, port} };

                //RegistURL();

                using boost::asio::ip::tcp;

                while (true) {
                    tcp::socket socket{ *ioc.get() };
                    acceptor.accept(socket);

                    std::thread{ std::bind(
                        &HttpServer::do_session,
                        std::move(socket),
                        doc_root,
                        _handlers) }.detach();
                }
            }
            catch (const std::exception& e)
            {
                LOGE("RestServer, AcceptSync(), Exception msg:{}", e.what());
            }

        }
        void AcceptAsync(const uint16_t port, std::shared_ptr<boost::asio::io_context> ioc, const int threads) {
            auto const address = boost::asio::ip::make_address("0.0.0.0");
            auto const doc_root = std::make_shared<std::string>("");

            // Create and launch a listening port
            std::make_shared<listener>(
                *ioc.get(),
                boost::asio::ip::tcp::endpoint{ address, port },
                doc_root,
                _handlers)->run();

            // Run the I/O service on the requested number of threads
            std::vector<std::thread> v;
            v.reserve(static_cast<std::vector<std::thread, std::allocator<std::thread>>::size_type>(threads) - 1);
            for (auto i = threads - 1; i > 0; --i)
                v.emplace_back(
                    [&ioc]
                    {
                        ioc->run();
                    });
            ioc->run();
        }

        static void do_session(boost::asio::ip::tcp::socket& socket
            , std::shared_ptr<std::string const> const& doc_root
            , const HandlerMap& handlers) {

            bool close = false;
            boost::beast::error_code ec;

            // This buffer is required to persist across reads
            boost::beast::flat_buffer buffer;

            // This lambda is used to send messages
            SendFuncSync send{ socket, close, ec };

            for (;;)
            {
                // Read a request
                boost::beast::http::request<boost::beast::http::string_body> req;
                boost::beast::http::read(socket, buffer, req, ec);
                if (ec == boost::beast::http::error::end_of_stream)
                    break;
                if (ec)
                    return fail(ec, "read");

                // Send the response
                handle_request(*doc_root, handlers, std::move(req), send);
                if (ec)
                    return fail(ec, "write");
                if (close)
                {
                    // This means we should close the connection, usually because
                    // the response indicated the "Connection: close" semantic.
                    break;
                }
            }

            // Send a TCP shutdown
            socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

            // At this point the connection is closed gracefully
        }

        static void fail(boost::beast::error_code ec, char const* what) {
            LOGE("failed RestServer(). error:{}, msg:{}", ec.message(), what);
        }

        // Append an HTTP rel-path to a local filesystem path.
        // The returned path is normalized for the platform.        
        static std::string path_cat(boost::beast::string_view base, boost::beast::string_view path) {
            if (base.empty())
                return std::string(path);
            std::string result(base);
#ifdef BOOST_MSVC
            char constexpr path_separator = '\\';
            if (result.back() == path_separator)
                result.resize(result.size() - 1);
            result.append(path.data(), path.size());
            for (auto& c : result)
                if (c == '/')
                    c = path_separator;
#else
            char constexpr path_separator = '/';
            if (result.back() == path_separator)
                result.resize(result.size() - 1);
            result.append(path.data(), path.size());
#endif
            return result;
        }

        template<class Body, class Allocator, class Send>
        static void handle_request(boost::beast::string_view doc_root
            , const HandlerMap& handlers
            , boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>&& req
            , Send& send);

    public:
        bool startSync(const uint16_t port, std::shared_ptr<boost::asio::io_context> ioc = nullptr) {
            if (0 >= port)
            {
                return false;
            }

            if (!ioc)
            {
                ioc = std::make_shared<boost::asio::io_context>();
            }

            std::thread{ std::bind(&HttpServer::AcceptSync, this, port, ioc) }.detach();

            return true;
        }
        bool startAsync(const uint16_t port, std::shared_ptr<boost::asio::io_context> ioc = nullptr, const int threads = 3) {
            if (0 >= port)
            {
                return false;
            }

            if (!ioc)
            {
                ioc = std::make_shared<boost::asio::io_context>();
            }

            std::thread{ std::bind(&HttpServer::AcceptAsync, this, port, ioc, threads) }.detach();

            return true;

        }

        bool registerURL(const std::string& url, HandlerType handler) {
            if (url.size() > 0) {
                if (true == _handlers.emplace(url, handler).second) {
                    LOGI("registed rest-api. url:{}", url);
                    return true;
                }
            }

            LOGE("registration failed. rest-api. url:{}", url);
            return false;
        }

        RegisterFunc GetRegisterFunc(const std::string& url) {
            auto func = [&url](HandlerType handler)
            {
                HttpServer::instance().registerURL(url, handler);
            };

            return func;
        }

    private:
        HandlerMap _handlers;

        //static std::function<boost::beast::http::response<boost::beast::http::string_body>
        //    (boost::beast::http::request<boost::beast::http::string_body, boost::beast::http::fields>& req,
        //        std::string& path,
        //        boost::beast::string_view msg)> _func;
    };

    // This function produces an HTTP response for the given
    // request. The type of the response object depends on the
    // contents of the request, so the interface requires the
    // caller to pass a generic lambda for receiving the response.
    template<class Body, class Allocator, class Send>
    void HttpServer::handle_request(boost::beast::string_view doc_root
        , const HandlerMap& handlers
        , boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>&& req
        , Send& send)
    {
        namespace beast = boost::beast;         // from <boost/beast.hpp>
        namespace http = beast::http;           // from <boost/beast/http.hpp>

        std::string path = path_cat(doc_root, req.target());

        // Returns a bad request response
        auto const bad_request =
            [&req, &path](beast::string_view why)
        {
            http::response<http::string_body> res{ http::status::bad_request, req.version() };
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = std::string(why);
            res.prepare_payload();

            LOGE("<restsvc:op> bad request url:{} requestString:{}", path, req.body());

            return res;
        };

        // Returns a not found response
        auto const not_found =
            [&req, &path](beast::string_view target)
        {
            http::response<http::string_body> res{ http::status::not_found, req.version() };
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "The resource '" + std::string(target) + "' was not found.";
            res.prepare_payload();

            LOGE("<restsvc:op> not_found url:{} requestString:{}", path, req.body());

            return res;
        };

        // Returns a server error response
        auto const server_error =
            [&req, &path](beast::string_view what)
        {
            http::response<http::string_body> res{ http::status::internal_server_error, req.version() };
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "An error occurred: '" + std::string(what) + "'";
            res.prepare_payload();

            LOGE("<restsvc:op> server_error url:{} requestString:{}", path, req.body());

            return res;
        };

        // Make sure we can handle the method
        //if (req.method() != http::verb::get) {
        //    return send(bad_request("Unknown HTTP-method"));
        //}

        // Request path must be absolute and not contain "..".
        if (req.target().empty() ||
            req.target()[0] != '/' ||
            req.target().find("..") != beast::string_view::npos)
            return send(bad_request("Illegal request-target"));

        auto it = handlers.find(path);
        if (handlers.end() == it) {
            return send(not_found(req.target()));

            //HttpServer::_func =
            //    //auto const not_found =
            //    [](boost::beast::http::request<boost::beast::http::string_body, boost::beast::http::fields>& req,
            //        std::string& path,
            //        boost::beast::string_view msg)
            //{
            //    http::response<http::string_body> res{ http::status::not_found, req.version() };
            //    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            //    res.set(http::field::content_type, "text/html");
            //    res.keep_alive(req.keep_alive());
            //    res.body() = "The resource '" + std::string(msg) + "' was not found.";
            //    res.prepare_payload();
            //
            //    LOGE("<restsvc:op> not_found url:{} requestString:{}", path, req.body());
            //
            //    return res;
            //};
            //
            //return send(HttpServer::_func(req, path, req.target()));
        }

        auto lower = [](std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
            return s;
        };

        std::map<std::string, std::string> headerMap;
        LOGT("<restsvc:op> header:", path, req.body());
        for (auto& header : req.base())
        {
            headerMap.emplace(lower(header.name_string().to_string()), lower(header.value().to_string()));
            LOGT("{}: {}", header.name_string().to_string(), header.value().to_string());
        }

        std::string reqBodyString = req.body();
        if (true == reqBodyString.empty()) {
            reqBodyString = "{}";
        }

        try {
            LOGT("<restsvc:op> request url:{} requestBody:{}", path, req.body());

            send._version = req.version();
            send._keepAlive = req.keep_alive();

            it->second(std::move(headerMap), std::move(reqBodyString), send);
        }
        catch (std::exception e) {
            return send(server_error(e.what()));
        }
    }

}
