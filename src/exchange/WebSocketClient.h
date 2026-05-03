#pragma once
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <vector>

namespace crypto {

class WebSocketClient {
public:
    using MessageCallback = std::function<void(const std::string&)>;

    WebSocketClient(const std::string& host, const std::string& port,
                    const std::string& path);
    ~WebSocketClient();

    void setMessageCallback(MessageCallback cb);
    void connect();
    void disconnect();
    bool isConnected() const;
    void send(const std::string& message);

private:
    std::string host_;
    std::string port_;
    std::string path_;
    MessageCallback msgCb_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> shouldRun_{false};
    std::thread workerThread_;
    std::mutex sendMutex_;

    // Pending outbound messages (e.g. subscribe payloads) queued before the
    // WebSocket handshake completes. Drained by the worker thread after
    // handshake and on every reconnect so subscriptions survive reconnects.
    std::vector<std::string> pendingSends_;

    void workerLoop();
};

} // namespace crypto
