#include <iostream>
#include <unistd.h>
#include "../include/reactor.h"
#include "../include/threadpool.h"
#include "../include/logger.h"

int main() {
    try {
        LOG_INFO("Starting ChatSystem Server...");

        // 1. Initialize ThreadPool
        ThreadPool pool(4);
        LOG_INFO("ThreadPool initialized with 4 workers.");

        // 2. Initialize EpollServer
        EpollServer server(&pool);
        server.init(8080);
        
        // 3. Start Event Loop
        server.run();
        
    } catch (const std::exception& e) {
        LOG_ERROR("Server crashed: " + std::string(e.what()));
        return 1;
    }

    return 0;
}
