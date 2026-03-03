#include "WebSocketClient.h"
#include "../core/Logger.h"

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <stdexcept>
#include <chrono>

namespace beast     = boost::beast;
namespace websocket = boost::beast::websocket;
namespace net       = boost::asio;
namespace ssl       = boost::asio::ssl;
using tcp           = net::ip::tcp;

namespace crypto {

WebSocketClient::WebSocketClient(const std::string& host,
                                  const std::string& port,
                                  const std::string& path)
    : host_(host), port_(port), path_(path) {}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

void WebSocketClient::setMessageCallback(MessageCallback cb) {
    msgCb_ = std::move(cb);
}

void WebSocketClient::connect() {
    shouldRun_ = true;
    workerThread_ = std::thread(&WebSocketClient::workerLoop, this);
}

void WebSocketClient::disconnect() {
    shouldRun_ = false;
    connected_ = false;
    if (workerThread_.joinable()) workerThread_.join();
}

bool WebSocketClient::isConnected() const { return connected_; }

void WebSocketClient::send(const std::string& message) {
    // Implementation would write to beast ws stream
    (void)message;
}

void WebSocketClient::workerLoop() {
    while (shouldRun_) {
        try {
            net::io_context ioc;
            ssl::context ctx(ssl::context::tlsv12_client);
            ctx.set_default_verify_paths();

            tcp::resolver resolver(ioc);
            auto results = resolver.resolve(host_, port_);

            auto stream = std::make_unique<
                websocket::stream<beast::ssl_stream<tcp::socket>>>(ioc, ctx);

            net::connect(stream->next_layer().next_layer(), results);
            stream->next_layer().handshake(ssl::stream_base::client);

            stream->set_option(websocket::stream_base::decorator(
                [this](websocket::request_type& req) {
                    req.set(boost::beast::http::field::host, host_);
                    req.set(boost::beast::http::field::user_agent, "CryptoTrader/1.0");
                }));
            stream->handshake(host_, path_);
            connected_ = true;
            Logger::get()->info("WebSocket connected to {}:{}{}", host_, port_, path_);

            while (shouldRun_) {
                beast::flat_buffer buffer;
                beast::error_code ec;
                stream->read(buffer, ec);
                if (ec) {
                    Logger::get()->warn("WebSocket read error: {}", ec.message());
                    break;
                }
                if (msgCb_) {
                    msgCb_(beast::buffers_to_string(buffer.data()));
                }
            }
            connected_ = false;
            beast::error_code ec;
            stream->close(websocket::close_code::normal, ec);

        } catch (const std::exception& e) {
            connected_ = false;
            Logger::get()->warn("WebSocket exception: {}", e.what());
        }

        if (shouldRun_) {
            Logger::get()->info("WebSocket reconnecting in 5s...");
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

} // namespace crypto
