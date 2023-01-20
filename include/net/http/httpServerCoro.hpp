#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/config.hpp>
#include <boost/json.hpp>

#include <memory>

#include <net/logger.hpp>


#ifndef SEND_INE
#ifdef _WIN32
#define SEND_INE(req)	sender(req, true);
#else
#define SEND_INE(req)	sender(req);
#endif
#endif//#ifndef SEND_INE

namespace mln::net {

    // Report a failure
    static void fail(boost::beast::error_code ec, char const* what)
    {
        if (ec.value() == boost::system::errc::operation_not_permitted) {
            if (!std::strcmp("read", what)) {
                return;
            }
        }

        LOGE("failed httpServerCoro. msg:{}, errorCode:{}", what, ec.value());
    }

    // This is the C++11 equivalent of a generic lambda.
// The function object is used to send an HTTP message.
    struct send_lambda
    {
        boost::beast::tcp_stream& stream_;
        bool& close_;
        boost::beast::error_code& ec_;
        boost::asio::yield_context yield_;

        send_lambda(
            boost::beast::tcp_stream& stream,
            bool& close,
            boost::beast::error_code& ec,
            boost::asio::yield_context yield)
            : stream_(stream)
            , close_(close)
            , ec_(ec)
            , yield_(yield)
        {
        }

        template<bool isRequest, class Body, class Fields>
        void
            operator()(boost::beast::http::message<isRequest, Body, Fields>&& msg) const
        {
            // Determine if we should close the connection after
            close_ = msg.need_eof();

            // We need the serializer here because the serializer requires
            // a non-const file_body, and the message oriented version of
            // http::write only works with const messages.
            boost::beast::http::serializer<isRequest, Body, Fields> sr{ msg };
            boost::beast::http::async_write(stream_, sr, yield_[ec_]);
        }

        template<bool isRequest, class Body, class Fields>
        void
            operator()(boost::beast::http::message<isRequest, Body, Fields>&& msg, const bool syncParam) const
        {
            // Determine if we should close the connection after
            close_ = msg.need_eof();

            // We need the serializer here because the serializer requires
            // a non-const file_body, and the message oriented version of
            // http::write only works with const messages.
            boost::beast::http::serializer<isRequest, Body, Fields> sr{ msg };
            boost::beast::http::write(stream_, sr, ec_);
        }
    };

    
    using HeaderMap = std::map<std::string, std::string>;
    /*using HttpCoroHandlerType = std::function<void(HeaderMap&&, std::string&&, SendFunc&)>;*/
    using HttpCoroHandlerType = std::function<void(HeaderMap&&, std::string&&, send_lambda&&, bool, unsigned int)>;
    //using HttpCoroHandlerType = std::function<void(HeaderMap&&, boost::json::value&&, send_lambda&&, bool, unsigned int)>;
    
    using HttpCoroHandlerMap = std::map<std::string, HttpCoroHandlerType>;
    

    // Return a reasonable mime type based on the extension of a file.
    static boost::beast::string_view
        mime_type(boost::beast::string_view path)
    {
        using boost::beast::iequals;
        auto const ext = [&path]
        {
            auto const pos = path.rfind(".");
            if (pos == boost::beast::string_view::npos)
                return boost::beast::string_view{};
            return path.substr(pos);
        }();
        if (iequals(ext, ".htm"))  return "text/html";
        if (iequals(ext, ".html")) return "text/html";
        if (iequals(ext, ".php"))  return "text/html";
        if (iequals(ext, ".css"))  return "text/css";
        if (iequals(ext, ".txt"))  return "text/plain";
        if (iequals(ext, ".js"))   return "application/javascript";
        if (iequals(ext, ".json")) return "application/json";
        if (iequals(ext, ".xml"))  return "application/xml";
        if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
        if (iequals(ext, ".flv"))  return "video/x-flv";
        if (iequals(ext, ".png"))  return "image/png";
        if (iequals(ext, ".jpe"))  return "image/jpeg";
        if (iequals(ext, ".jpeg")) return "image/jpeg";
        if (iequals(ext, ".jpg"))  return "image/jpeg";
        if (iequals(ext, ".gif"))  return "image/gif";
        if (iequals(ext, ".bmp"))  return "image/bmp";
        if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
        if (iequals(ext, ".tiff")) return "image/tiff";
        if (iequals(ext, ".tif"))  return "image/tiff";
        if (iequals(ext, ".svg"))  return "image/svg+xml";
        if (iequals(ext, ".svgz")) return "image/svg+xml";
        return "application/text";
    }

    // Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
    static std::string
        path_cat(
            boost::beast::string_view base,
            boost::beast::string_view path)
    {
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

    // This function produces an HTTP response for the given
        // request. The type of the response object depends on the
        // contents of the request, so the interface requires the
        // caller to pass a generic lambda for receiving the response.
    template<
        class Body, class Allocator,
        class Send>
    void
        handle_request(
            boost::beast::string_view doc_root,
            boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>&& req,
            Send&& sender,
            HttpCoroHandlerMap &httpCoroHandlerMap
        )
    {
        // Returns a bad request response
        auto const bad_request =
            [&req](boost::beast::string_view why)
        {
            boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::bad_request, req.version() };
            res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
            //res.set(boost::beast::http::field::content_type, "text/html");
            res.set(boost::beast::http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.body() = std::string(why);
            res.prepare_payload();
            return res;
        };

        // Returns a not found response
        auto const not_found =
            [&req](boost::beast::string_view target)
        {
            boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::not_found, req.version() };
            res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
            //res.set(boost::beast::http::field::content_type, "text/html");
            res.set(boost::beast::http::field::content_type, "text/html");
            res.keep_alive(req.keep_alive());
            res.body() = "The resource '" + std::string(target) + "' was not found.";
            res.prepare_payload();
            return res;
        };

        // Returns a server error response
        auto const server_error =
            [&req](boost::beast::string_view what)
        {
            boost::beast::http::response<boost::beast::http::string_body> res{ boost::beast::http::status::internal_server_error, req.version() };
            res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
            //res.set(boost::beast::http::field::content_type, "text/html");
            res.set(boost::beast::http::field::content_type, "application/json");
            res.keep_alive(req.keep_alive());
            res.body() = "An error occurred: '" + std::string(what) + "'";
            res.prepare_payload();
            return res;
        };


        // Make sure we can handle the method
        if (req.method() != boost::beast::http::verb::get &&
            req.method() != boost::beast::http::verb::post)
            return sender(bad_request("not using HTTP-method"));

        // Request path must be absolute and not contain "..".
        if (req.target().empty() ||
            req.target()[0] != '/' ||
            req.target().find("..") != boost::beast::string_view::npos)
            return sender(bad_request("Illegal request-target"));

        // Build the path to the requested file
        std::string path = path_cat(doc_root, req.target());

        auto it = httpCoroHandlerMap.find(path);
        if (httpCoroHandlerMap.end() == it) {
            return sender(not_found(req.target()));
        }

        // Çì´õ¸Ê »ý¼º
        auto lower = [](std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
            return s;
        };

        std::map<std::string, std::string> headerMap;
        for (auto& header : req.base())
        {
            headerMap.emplace(lower(header.name_string().to_string()), lower(header.value().to_string()));
            LOGT("{}: {}", header.name_string().to_string(), header.value().to_string());
        }

        std::string reqBodyString = req.body();
        if (true == reqBodyString.empty()) {
            reqBodyString = "{}";
        }

        headerMap.emplace("__path__", path);

        it->second(std::move(headerMap), std::move(reqBodyString), std::move(sender), req.keep_alive(), req.version());
    }

   

    class HttpServerCoro 
    {
    public:
        static HttpServerCoro& instance() { static HttpServerCoro _instance; return _instance; }

        bool startAsync(const uint16_t port, std::shared_ptr<boost::asio::io_context> ioc = nullptr, const int threads = 3) {
            if (0 >= port)
            {
                return false;
            }

            if (!ioc)
            {
                ioc = std::make_shared<boost::asio::io_context>();
            }

            std::thread{ std::bind(&HttpServerCoro::acceptAsync, this, port, ioc, threads) }.detach();

            return true;
        }

        bool registHandler(const std::string& url, HttpCoroHandlerType handler) {
            if (url.size() > 0) {
                if (true == httpCoroHandlers.emplace(url, handler).second) {
                    LOGI("registed rest-api. url:{}", url);
                    return true;
                }
            }

            LOGE("registration failed. rest-api. url:{}", url);
            return false;
        }

    public:
        void acceptAsync(const uint16_t port, std::shared_ptr<boost::asio::io_context> ioc, const int threads) {

            auto const address = boost::asio::ip::make_address("0.0.0.0");
            auto const doc_root = std::make_shared<std::string>("");

            boost::asio::spawn(*ioc.get(),
                std::bind(
                    &do_listen,
                    ioc,
                    boost::asio::ip::tcp::endpoint{ address, port },
                    doc_root,
                    std::placeholders::_1));

            // Run the I/O service on the requested number of threads
            std::vector<std::thread> v;
            v.reserve(threads - 1);
            for (auto i = threads - 1; i > 0; --i)
                v.emplace_back(
                    [&ioc]
                    {
                        ioc->run();
                    });
            ioc->run();

            for (std::thread& t : v) {
                t.join();
            }
            v.clear();

            LOGI("mlnnet httpServerCoro stop..");
        }

    private:

        // Accepts incoming connections and launches the sessions
        static
            void do_listen(std::shared_ptr<boost::asio::io_context> ioc,
                boost::asio::ip::tcp::endpoint endpoint,
                std::shared_ptr<std::string const> const& doc_root,
                boost::asio::yield_context yield)
        {
            boost::beast::error_code ec;

            // Open the acceptor
            boost::asio::ip::tcp::acceptor acceptor(*ioc.get());
            acceptor.open(endpoint.protocol(), ec);
            if (ec)
                return fail(ec, "open");

            // Allow address reuse
            acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
            if (ec)
                return fail(ec, "set_option");

            // Bind to the server address
            acceptor.bind(endpoint, ec);
            if (ec)
                return fail(ec, "bind");

            // Start listening for connections
            acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
            if (ec)
                return fail(ec, "listen");

            while (!ioc->stopped()) {
                boost::asio::ip::tcp::socket socket(*ioc.get());
                acceptor.async_accept(socket, yield[ec]);
                if (ec)
                    fail(ec, "accept");
                else
                    boost::asio::spawn(
                        acceptor.get_executor(),
                        std::bind(
                            &do_session,
                            boost::beast::tcp_stream(std::move(socket)),
                            doc_root,
                            std::placeholders::_1));
            }
        }


        

        // Handles an HTTP server connection
        static
            void do_session(boost::beast::tcp_stream& stream,
                std::shared_ptr<std::string const> const& doc_root,
                boost::asio::yield_context yield)
        {
            bool close = false;
            boost::beast::error_code ec;

            // This buffer is required to persist across reads
            boost::beast::flat_buffer buffer;

            // This lambda is used to send messages
            send_lambda lambda{ stream, close, ec, yield };

            for (;;)
            {
                // Set the timeout.
                stream.expires_after(std::chrono::seconds(30));

                // Read a request
                boost::beast::http::request<boost::beast::http::string_body> req;
                boost::beast::http::async_read(stream, buffer, req, yield[ec]);
                if (ec == boost::beast::http::error::end_of_stream)
                    break;
                if (ec)
                    return fail(ec, "read");

                // Send the response
                handle_request(*doc_root, std::move(req), lambda, httpCoroHandlers);

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
            stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

            // At this point the connection is closed gracefully


            stream.socket().close(ec);
        }

    private:

        inline static HttpCoroHandlerMap httpCoroHandlers;
    };
}//namespace mln::net {