#include "../include/business_logic.h"
#include "../include/logger.h"
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include "../include/file_transfer.h"
void BusinessLogic::process_packet(std::shared_ptr<UserContext> user, PacketHeader header, std::vector<char> body, ConnectionMgr& conn_mgr) {
    if (!user) return;

    switch (header.msg_type) {
        case MSG_LOGIN:
            handle_login(user, body, conn_mgr);
            break;
        case MSG_CHAT_PUBLIC:
            handle_chat_public(user, body, conn_mgr);
            break;
        case MSG_CHAT_PRIVATE:
            handle_chat_private(user, body, conn_mgr);
            break;
        case MSG_FILE_REQ:
            if (body.size() >= sizeof(FileReqBody)) {
                FileReqBody* req = (FileReqBody*)body.data();
                FileTransfer::handle_file_request(user->fd, std::string(req->filename));
            }
            break;
        case MSG_HEARTBEAT:
            user->last_heartbeat = time(nullptr);
            // Optional: Send ACK or just silent update
            break;
        default:
            LOG_INFO("Unknown message type: " + std::to_string(header.msg_type));
            break;
    }
}

void BusinessLogic::send_to_fd(int fd, int32_t msg_type, const std::string& data) {
    PacketHeader header;
    header.total_len = sizeof(PacketHeader) + data.size();
    header.msg_type = msg_type;
    header.crc32 = 0;

    // TODO: Verify if write is thread-safe or if we need a write queue.
    // For now, raw write on non-blocking socket.
    
    // Note: Writing to non-blocking socket in loop until done.
    // In real reactor, we should register EPOLLOUT if write blocks.
    // Here simplifying for Phase 3.
    
    std::vector<char> packet(header.total_len);
    memcpy(packet.data(), &header, sizeof(PacketHeader));
    if (!data.empty()) {
        memcpy(packet.data() + sizeof(PacketHeader), data.data(), data.size());
    }

    ssize_t sent = 0;
    while(sent < header.total_len) {
        ssize_t ret = write(fd, packet.data() + sent, header.total_len - sent);
        if (ret < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
            LOG_ERROR("Write failed to fd " + std::to_string(fd));
            break;
        }
        sent += ret;
    }
}

void BusinessLogic::handle_login(std::shared_ptr<UserContext> user, std::vector<char>& body, ConnectionMgr& conn_mgr) {
    // Unsafe cast for simplicity (Phase 3)
    if (body.size() < sizeof(LoginBody)) return;
    
    LoginBody* login = (LoginBody*)body.data();
    user->username = std::string(login->username);
    
    LOG_INFO("User logged in: " + user->username + " (fd: " + std::to_string(user->fd) + ")");
    
    send_to_fd(user->fd, MSG_LOGIN_ACK, "Welcome " + user->username);
}

void BusinessLogic::handle_chat_public(std::shared_ptr<UserContext> user, std::vector<char>& body, ConnectionMgr& conn_mgr) {
    if (body.size() < sizeof(ChatBody)) return;
    ChatBody* chat = (ChatBody*)body.data();
    
    std::string msg = "[" + user->username + "]: " + std::string(chat->content);
    LOG_INFO("Public Chat: " + msg);
    
    auto all_fds = conn_mgr.get_all_fds();
    for (int fd : all_fds) {
        if (fd != user->fd) {
            send_to_fd(fd, MSG_CHAT_PUBLIC, msg);
        }
    }
}

void BusinessLogic::handle_chat_private(std::shared_ptr<UserContext> user, std::vector<char>& body, ConnectionMgr& conn_mgr) {
    if (body.size() < sizeof(ChatBody)) return;
    ChatBody* chat = (ChatBody*)body.data();
    
    std::string target(chat->target_user);
    std::string content(chat->content);
    
    int target_fd = conn_mgr.get_fd_by_username(target);
    if (target_fd != -1) {
         std::string msg = "[Private from " + user->username + "]: " + content;
         send_to_fd(target_fd, MSG_CHAT_PRIVATE, msg);
    } else {
        send_to_fd(user->fd, MSG_ERROR, "User not found: " + target);
    }
}
