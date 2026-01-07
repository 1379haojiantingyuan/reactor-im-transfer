#ifndef BUSINESS_LOGIC_H
#define BUSINESS_LOGIC_H

#include <vector>
#include <memory>
#include "protocol.h"
#include "connection_mgr.h"

class BusinessLogic {
public:
    static void process_packet(std::shared_ptr<UserContext> user, PacketHeader header, std::vector<char> body, ConnectionMgr& conn_mgr);

private:
    static void handle_login(std::shared_ptr<UserContext> user, std::vector<char>& body, ConnectionMgr& conn_mgr);
    static void handle_chat_public(std::shared_ptr<UserContext> user, std::vector<char>& body, ConnectionMgr& conn_mgr);
    static void handle_chat_private(std::shared_ptr<UserContext> user, std::vector<char>& body, ConnectionMgr& conn_mgr);
    static void send_to_fd(int fd, int32_t msg_type, const std::string& data);
};

#endif // BUSINESS_LOGIC_H
