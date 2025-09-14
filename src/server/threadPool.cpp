#include "threadPool.hpp"

ThreadPool::ThreadPool(size_t numThreads)
    : m_running(true) {

    for (size_t i = 0; i < numThreads; ++i) {
        m_threads.emplace_back([this]() { workerThread(); }); // Create and start worker threads
    }
}

ThreadPool::~ThreadPool() {
    m_running = false;
    m_queueCV.notify_all(); // Wake up all threads to let them exit
    for (auto &t : m_threads) {
        if (t.joinable())
            t.join();
    }
}

void ThreadPool::enqueueTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_taskQueue.push(std::move(task));
    }
    m_queueCV.notify_one(); // Notify one waiting thread
}

void ThreadPool::workerThread() {
    while (m_running) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCV.wait(
                lock, [this]() { return !m_taskQueue.empty() || !m_running; }); // Wait for tasks or shutdown signal

            if (!m_running && m_taskQueue.empty())
                return; // Exit if shutting down and no tasks left

            task = std::move(m_taskQueue.front()); // Get the next task
            m_taskQueue.pop();
        }
        if (task)
            task(); // Execute the task outside the lock scope
    }
}