#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    template<typename F, typename... Args>
    auto enqueue(F &&f, Args &&... args) -> std::future<decltype(f(args...))>;

    template<typename F>
    void enqueue_no_future(F &&f);

    [[nodiscard]] size_t getNumberOfThreads() const;

private:
    std::vector<std::jthread> m_workers;
    std::queue<std::function<void()> > m_tasks;
    std::vector<std::function<void()> > m_localTasks;

    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_stop;
    size_t m_numThreads;
};

template<typename F, typename... Args>
auto ThreadPool::enqueue(F &&f, Args &&... args) -> std::future<decltype(f(args...))> {
    using return_type = decltype(f(args...));
    auto task = std::make_shared<std::packaged_task<return_type()> >(
        std::bind_front(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<return_type> res = task->get_future();
    {
        std::lock_guard lock(m_mutex);
        m_tasks.emplace([task] { (*task)(); });
    }
    m_condition.notify_one();
    return res;
}

template<typename F>
void ThreadPool::enqueue_no_future(F &&f) {
    {
        std::lock_guard lock(m_mutex);
        m_tasks.emplace(std::forward<F>(f));
    }
    m_condition.notify_one();
}

#endif // THREADPOOL_H
