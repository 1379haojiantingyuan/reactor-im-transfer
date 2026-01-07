#include <fstream>
#include "client.h"
#include "../include/protocol.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

ChatClient::ChatClient() : socket_fd(-1), running(false) {}

ChatClient::~ChatClient() {
    stop();
}

bool ChatClient::connect_to_server(const std::string& ip, int port) {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) return false;

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(socket_fd);
        return false;
    }
    
    running = true;
    return true;
}

void ChatClient::stop() {
    running = false;
    if (socket_fd != -1) {
        close(socket_fd);
        socket_fd = -1;
    }
    if (receiver_thread.joinable()) receiver_thread.join();
    if (heartbeat_thread.joinable()) heartbeat_thread.join();
}

void ChatClient::send_packet(int32_t msg_type, const void* data, size_t len) {
    if (socket_fd == -1) return;

    PacketHeader header;
    header.msg_type = msg_type;
    header.total_len = sizeof(PacketHeader) + len;
    header.crc32 = 0;

    std::vector<char> packet(header.total_len);
    memcpy(packet.data(), &header, sizeof(PacketHeader));
    if (len > 0) {
        memcpy(packet.data() + sizeof(PacketHeader), data, len);
    }

    write(socket_fd, packet.data(), header.total_len);
}

void ChatClient::login(const std::string& user) {
    username = user;
    LoginBody body;
    strncpy(body.username, username.c_str(), sizeof(body.username) - 1);
    send_packet(MSG_LOGIN, &body, sizeof(body));
}

void ChatClient::send_chat_public(const std::string& message) {
    ChatBody body;
    memset(&body, 0, sizeof(body));
    strncpy(body.content, message.c_str(), sizeof(body.content) - 1);
    send_packet(MSG_CHAT_PUBLIC, &body, sizeof(body));
}

void ChatClient::send_chat_private(const std::string& target, const std::string& message) {
    ChatBody body;
    memset(&body, 0, sizeof(body));
    strncpy(body.target_user, target.c_str(), sizeof(body.target_user) - 1);
    strncpy(body.content, message.c_str(), sizeof(body.content) - 1);
    send_packet(MSG_CHAT_PRIVATE, &body, sizeof(body));
}

void ChatClient::send_heartbeat() {
    send_packet(MSG_HEARTBEAT, nullptr, 0);
}

void ChatClient::start_receiver() {
    receiver_thread = std::thread(&ChatClient::receiver_loop, this);
    heartbeat_thread = std::thread(&ChatClient::heartbeat_loop, this);
}

void ChatClient::set_on_message(std::function<void(const std::string&)> cb) {
    on_message = cb;
}

void ChatClient::heartbeat_loop() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        send_heartbeat();
    }
}

void ChatClient::request_file(const std::string& filename) {
    pending_filename = filename;
    FileReqBody body;
    strncpy(body.filename, filename.c_str(), sizeof(body.filename) - 1);
    send_packet(MSG_FILE_REQ, &body, sizeof(body));
}

void ChatClient::receiver_loop() {
    std::vector<char> buffer;
    char temp[4096];

    while (running) {
        ssize_t bytes_read = read(socket_fd, temp, sizeof(temp));
        if (bytes_read <= 0) {
            if (on_message) on_message("Disconnected from server.");
            running = false;
            break;
        }

        buffer.insert(buffer.end(), temp, temp + bytes_read);

        while (buffer.size() >= sizeof(PacketHeader)) {
            PacketHeader header;
            memcpy(&header, buffer.data(), sizeof(PacketHeader));

            if (buffer.size() < (size_t)header.total_len) break;

            // Extract body
            std::string msg_content;
            size_t body_len = header.total_len - sizeof(PacketHeader);
            
            if (header.msg_type == MSG_FILE_DATA) {
               // Handle File Download
               if (!pending_filename.empty() && body_len > 0) {
                   // Append to file (simple implementation, assumes one-shot or sequential packets)
                   // But wait, server sends header then data. 
                   // Actually server sends:
                   // 1. PacketHeader (total_len = Head + FileSize)
                   // 2. File Content (using sendfile)
                   // So the 'body' here IS the file content.
                   
                   std::ofstream outfile(pending_filename, std::ios::binary | std::ios::app); // Append? Or overwrite? 
                   // Design: Server sends ONE generic packet with huge body.
                   // Wait, server logic: header.total_len = header + st_size.
                   // read_buffer will accumulate ALL OF IT if we are not careful.
                   // But if file is 1GB, we can't buffer 1GB in memory vector.
                   // The current client `receiver_loop` buffers EVERYTHING into `buffer` vector.
                   // This is a limitation of current client design for large files.
                   // For now, I will stick to this design as I cannot rewrite the whole buffering logic 
                   // safely without risking breakage. 
                   // I will save the body content to file.
                   
                   if (outfile.is_open()) {
                        outfile.write(buffer.data() + sizeof(PacketHeader), body_len);
                        outfile.close();
                        msg_content = "[File saved to " + pending_filename + "]";
                        pending_filename.clear(); // Clear after save
                   } else {
                        msg_content = "[Error: Could not save file " + pending_filename + "]";
                   }
               }
            } else {
                 if (body_len > 0) {
                    char* body_ptr = buffer.data() + sizeof(PacketHeader);
                    msg_content = std::string(body_ptr, body_len);
                 }
            }

            if (on_message && !msg_content.empty()) on_message(msg_content);

            buffer.erase(buffer.begin(), buffer.begin() + header.total_len);
        }
    }
}
