#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
  public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    void enqueueTask(std::function<void()> task);
    void stop();

  private:
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_taskQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV; // Condition variable for task availability and notification
    std::atomic<bool> m_running;
    void workerThread();
};

#endif // THREADPOOL_HPP