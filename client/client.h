#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>

class ChatClient {
public:
    ChatClient();
    ~ChatClient();

    bool connect_to_server(const std::string& ip, int port);
    void login(const std::string& username);
    void send_chat_public(const std::string& message);
    void send_chat_private(const std::string& target, const std::string& message);
    void request_file(const std::string& filename);
    void send_heartbeat();

    // Callback for when a message is received (to update UI)
    void set_on_message(std::function<void(const std::string&)> cb);
    
    // Starts the receiver thread
    void start_receiver();
    void stop();

private:
    int socket_fd;
    std::string username;
    std::atomic<bool> running;
    std::thread receiver_thread;
    std::thread heartbeat_thread;
    std::function<void(const std::string&)> on_message;
    
    std::string pending_filename;

    void receiver_loop();
    void heartbeat_loop();
    void send_packet(int32_t msg_type, const void* data, size_t len);
};

#endif // CLIENT_H
