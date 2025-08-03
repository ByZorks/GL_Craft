#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    template<typename F, typename... Args>
    auto enqueue(F &&f, Args &&... args) -> std::future<decltype(f(args...))>;

    [[nodiscard]] size_t m_num_threads() const;

private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()> > m_tasks;

    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_stop;
    size_t m_numThreads;
};

template<typename F, typename... Args>
auto ThreadPool::enqueue(F &&f, Args &&... args) -> std::future<decltype(f(args...))> {
    using return_type = decltype(f(args...));
    auto task = std::make_shared<std::packaged_task<return_type()> >(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<return_type> res = task->get_future(); {
        std::lock_guard lock(m_mutex);
        m_tasks.emplace([task] { (*task)(); });
    }
    m_condition.notify_one();
    return res;
}
#endif // THREADPOOL_H
