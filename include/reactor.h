#ifndef REACTOR_H
#define REACTOR_H

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <cstring>
#include <vector>
#include <functional>
#include "logger.h"
#include "threadpool.h"
#include "connection_mgr.h"

// Basic socket wrapper functions
int create_server_socket(int port, const char* ip = "0.0.0.0");
void set_nonblocking(int fd);

class EpollServer {
public:
    EpollServer(ThreadPool* pool);
    ~EpollServer();

    void init(int port, const char* ip = "0.0.0.0");
    void run();

private:
    int epoll_fd;
    int listen_fd;
    ThreadPool* thread_pool;
    bool running;
    ConnectionMgr conn_mgr;
    
    // Helper to add file descriptor to epoll
    void add_fd(int fd, uint32_t events);
    // Helper to remove file descriptor from epoll
    void remove_fd(int fd);
    
    // Handlers
    void handle_new_connection();
    void handle_client_data(int client_fd);
    
    // Heartbeat Monitor
    void heartbeat_monitor();
    std::thread monitor_thread;
};

#endif // REACTOR_H
