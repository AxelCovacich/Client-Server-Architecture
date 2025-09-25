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
    /**
     * @brief Constructs a ThreadPool with a specified number of worker threads.
     * @param numThreads The number of threads to create in the pool.
     */
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    /**
     * @brief Enqueues a new task to be executed by the thread pool.
     * @param task A callable object (e.g., lambda, function pointer) representing the task.
     */
    void enqueueTask(std::function<void()> task);

    /**
     * @brief Stops all threads and clears the task queue.
     * This method will wait for all currently running tasks to complete before returning.
     */
    void stop();

  private:
    std::vector<std::thread> m_threads;
    std::queue<std::function<void()>> m_taskQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_queueCV; // Condition variable for task availability and notification
    std::atomic<bool> m_running;

    /**
     * @brief The worker thread function that continuously processes tasks from the queue.
     */
    void workerThread();
};

#endif // THREADPOOL_HPP