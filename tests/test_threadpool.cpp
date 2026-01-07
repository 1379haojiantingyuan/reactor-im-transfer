#include <iostream>
#include <vector>
#include <atomic>
#include <cassert>
#include "../include/threadpool.h"

void test_threadpool() {
    std::cout << "[Test] ThreadPool: Starting..." << std::endl;
    
    ThreadPool pool(4);
    std::atomic<int> counter(0);
    int num_tasks = 100;
    std::vector<std::future<void>> results;

    for (int i = 0; i < num_tasks; ++i) {
        results.emplace_back(
            pool.enqueue([&counter] {
                counter++;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            })
        );
    }

    for (auto& res : results) {
        res.get();
    }

    assert(counter == num_tasks);
    std::cout << "[Test] ThreadPool: Passed. Counter = " << counter << std::endl;
}

int main() {
    test_threadpool();
    return 0;
}
