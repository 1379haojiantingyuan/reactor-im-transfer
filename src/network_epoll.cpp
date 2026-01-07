#include "../include/reactor.h"
#include "../include/business_logic.h"
#include <iostream>
#include <cstring>
#include <errno.h>

#define MAX_EVENTS 1024
#define BUFFER_SIZE 4096

// --- Basic Socket Wrappers ---

int create_server_socket(int port, const char* ip) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        LOG_ERROR("Failed to create socket");
        return -1;
    }

    // Set SO_REUSEADDR
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_ERROR("setsockopt failed");
        close(listen_fd);
        return -1;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &address.sin_addr) <= 0) {
        LOG_ERROR("Invalid IP address");
        close(listen_fd);
        return -1;
    }

    if (bind(listen_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        LOG_ERROR("bind failed");
        close(listen_fd);
        return -1;
    }

    if (listen(listen_fd, 128) < 0) {
        LOG_ERROR("listen failed");
        close(listen_fd);
        return -1;
    }

    return listen_fd;
}

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        LOG_ERROR("fcntl F_GETFL failed");
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        LOG_ERROR("fcntl F_SETFL failed");
    }
}

// --- EpollServer Implementation ---

EpollServer::EpollServer(ThreadPool* pool) 
    : epoll_fd(-1), listen_fd(-1), thread_pool(pool), running(false) {}

EpollServer::~EpollServer() {
    running = false;
    if (monitor_thread.joinable()) monitor_thread.join();
    if (epoll_fd != -1) close(epoll_fd);
    if (listen_fd != -1) close(listen_fd);
}

void EpollServer::init(int port, const char* ip) {
    running = true;
    // Start Heartbeat Monitor
    monitor_thread = std::thread(&EpollServer::heartbeat_monitor, this);
    listen_fd = create_server_socket(port, ip);
    if (listen_fd < 0) {
        throw std::runtime_error("Failed to init server socket");
    }
    
    // Important: Switch listen_fd to non-blocking
    set_nonblocking(listen_fd);

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        throw std::runtime_error("Failed to create epoll instance");
    }

    // add_fd(listen_fd, EPOLLIN | EPOLLET); // Removed to avoid double registration and ensure LT
    // Actually plan said "Design uses LT". So correcting to LT.
    // Update: wait, re-reading plan: "If fd == listen_fd: loop accept (if ET) or single accept (if LT). Design uses LT."
    // So I will use standard LT (default)
    
    // Remove EPOLLET to use Level Triggered
    // add_fd(listen_fd, EPOLLIN); 
    
    // But implementation:
    struct epoll_event event;
    event.data.fd = listen_fd;
    event.events = EPOLLIN; // Level Triggered by default
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event) == -1) {
         throw std::runtime_error("Failed to add listen_fd to epoll: " + std::string(strerror(errno)));
    }
    
    LOG_INFO("Server initialized on port " + std::to_string(port));
}

void EpollServer::add_fd(int fd, uint32_t events) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
        LOG_ERROR("Failed to add fd to epoll");
    }
    set_nonblocking(fd);
    conn_mgr.add_connection(fd);
}

void EpollServer::remove_fd(int fd) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        LOG_ERROR("Failed to remove fd from epoll");
    }
    close(fd);
    conn_mgr.remove_connection(fd);
}

void EpollServer::run() {
    struct epoll_event events[MAX_EVENTS];

    LOG_INFO("Epoll loop starting...");

    while (running) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) continue;
            LOG_ERROR("epoll_wait failed");
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;

            if (fd == listen_fd) {
                handle_new_connection();
            } else if (events[i].events & EPOLLIN) {
                handle_client_data(fd);
            } else {
                LOG_INFO("Unexpected event on fd " + std::to_string(fd));
            }
        }
    }
}

void EpollServer::handle_new_connection() {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    // Level Triggered: Accept one connection per event trigger is safe enough for simpler logic,
    // though a loop is robust. Since I am in LT, one accept is standard, but if multiple arrive at once, 
    // epoll_wait will return immediately again.
    
    int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_fd < 0) {
        LOG_ERROR("accept failed");
        return;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    
    LOG_INFO("New connection from " + std::string(client_ip) + ":" + std::to_string(ntohs(client_addr.sin_port)) + " (fd: " + std::to_string(client_fd) + ")");

    add_fd(client_fd, EPOLLIN); 
    // Note: EPOLLIN implies Level Triggered. 
    // If we wanted ET we would OR with EPOLLET.
}

void EpollServer::handle_client_data(int client_fd) {
    // Get User Context
    auto user = conn_mgr.get_user_by_fd(client_fd);
    if (!user) {
        // Should not happen if add_fd works
        remove_fd(client_fd);
        return;
    }

    // Read available data
    char temp_buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(client_fd, temp_buffer, BUFFER_SIZE);

    if (bytes_read > 0) {
        // Append to user buffer
        user->read_buffer.insert(user->read_buffer.end(), temp_buffer, temp_buffer + bytes_read);

        // Process loop (Sticky Packet Handling)
        while (true) {
            // Check if we have enough for a header
            if (user->read_buffer.size() < sizeof(PacketHeader)) {
                break; // Need more data
            }

            // Peek Header
            PacketHeader header;
            memcpy(&header, user->read_buffer.data(), sizeof(PacketHeader));

            // Sanity Check on length to prevent OOM
            if (header.total_len > 10 * 1024 * 1024 || header.total_len < (int)sizeof(PacketHeader)) {
                 LOG_ERROR("Invalid packet length from fd " + std::to_string(client_fd));
                 remove_fd(client_fd);
                 return;
            }

            // Check if we have the full packet
            if ((int)user->read_buffer.size() < header.total_len) {
                break; // Need more data
            }

            // Extract Body
            int body_len = header.total_len - sizeof(PacketHeader);
            std::vector<char> body(body_len);
            if (body_len > 0) {
                memcpy(body.data(), user->read_buffer.data() + sizeof(PacketHeader), body_len);
            }

            // Remove this packet from buffer
            user->read_buffer.erase(user->read_buffer.begin(), user->read_buffer.begin() + header.total_len);

            // Dispatch Task
            // Note: We capture 'this' to access conn_mgr, but be careful with lifetime. 
            // Server lives in main(), so it should outlive tasks.
            thread_pool->enqueue([user, header, b = std::move(body), this]() {
                BusinessLogic::process_packet(user, header, b, this->conn_mgr);
            });
        }

    } else if (bytes_read == 0) {
        LOG_INFO("Client disconnected (fd: " + std::to_string(client_fd) + ")");
        remove_fd(client_fd);
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("read error on fd " + std::to_string(client_fd));
            remove_fd(client_fd);
        }
    }
}

void EpollServer::heartbeat_monitor() {
    LOG_INFO("Heartbeat monitor thread started.");
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        // Check for timeouts (e.g. 30 seconds)
        auto dead_fds = conn_mgr.check_timeouts(30);
        
        for (int fd : dead_fds) {
            LOG_INFO("Client timed out (fd: " + std::to_string(fd) + ")");
            // Remove from epoll and map
            // Note: remove_fd triggers conn_mgr.remove_connection too
            // Thread safety: epoll_ctl is thread-safe. conn_mgr is thread-safe.
            remove_fd(fd);
        }
    }
    LOG_INFO("Heartbeat monitor thread stopped.");
}
