#ifndef CONNECTION_MGR_H
#define CONNECTION_MGR_H

#include <map>
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <ctime>
#include "protocol.h"

struct UserContext {
    int fd;
    std::string username;
    std::vector<char> read_buffer; // Accumulator for sticky packets
    time_t last_heartbeat;
    
    UserContext(int socket_fd) : fd(socket_fd), last_heartbeat(time(nullptr)) {}
};

class ConnectionMgr {
public:
    void add_connection(int fd) {
        std::lock_guard<std::mutex> lock(map_mutex);
        connections[fd] = std::make_shared<UserContext>(fd);
    }

    void remove_connection(int fd) {
        std::lock_guard<std::mutex> lock(map_mutex);
        connections.erase(fd);
    }

    std::shared_ptr<UserContext> get_user_by_fd(int fd) {
        std::lock_guard<std::mutex> lock(map_mutex);
        auto it = connections.find(fd);
        if (it != connections.end()) {
            return it->second;
        }
        return nullptr;
    }

    int get_fd_by_username(const std::string& username) {
        std::lock_guard<std::mutex> lock(map_mutex);
        for (const auto& kv : connections) {
            if (kv.second->username == username) {
                return kv.first;
            }
        }
        return -1;
    }

    // Helper to get all connected users (for broadcast)
    std::vector<int> get_all_fds() {
        std::lock_guard<std::mutex> lock(map_mutex);
        std::vector<int> fds;
        for (const auto& kv : connections) {
            fds.push_back(kv.first);
        }
        return fds;
    }

    // Returns list of timed-out fds
    std::vector<int> check_timeouts(int timeout_seconds) {
        std::lock_guard<std::mutex> lock(map_mutex);
        std::vector<int> dead_fds;
        time_t now = time(nullptr);
        
        for (const auto& kv : connections) {
            if (now - kv.second->last_heartbeat > timeout_seconds) {
                dead_fds.push_back(kv.first);
            }
        }
        return dead_fds;
    }

private:
    std::map<int, std::shared_ptr<UserContext>> connections;
    std::mutex map_mutex;
};

#endif // CONNECTION_MGR_H
