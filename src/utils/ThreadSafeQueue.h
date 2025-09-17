#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H

#include <mutex>
#include <queue>

template<typename T>
class ThreadSafeQueue {
public:
    void push(const T &item);
    void push(T &&item);
    bool try_pop(T &out);

    void clear();

private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
};

template<typename T>
void ThreadSafeQueue<T>::push(const T &item) {
    std::lock_guard lock(m_mutex);
    m_queue.push(item);
}

template<typename T>
void ThreadSafeQueue<T>::push(T &&item) {
    std::lock_guard lock(m_mutex);
    m_queue.push(std::move(item));
}

template<typename T>
bool ThreadSafeQueue<T>::try_pop(T &out) {
    std::lock_guard lock(m_mutex);
    if (m_queue.empty()) return false;
    out = std::move(m_queue.front());
    m_queue.pop();
    return true;
}

template<typename T>
void ThreadSafeQueue<T>::clear() {
    std::lock_guard lock(m_mutex);
    while (!m_queue.empty()) {
        m_queue.pop();
    }
}

#endif //SAFEQUEUE_H
