#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>

/**
 * @author https://www.geeksforgeeks.org/dsa/implement-thread-safe-queue-in-c/
*/

template<typename T>
class ThreadSafeQueue {
public:
    void push(const T &item) { {
            std::lock_guard lk(mutex_);
            queue_.push(item);
        }
    }

    void push(T &&item) { {
            std::lock_guard lk(mutex_);
            queue_.push(std::move(item));
        }
    }

    [[nodiscard]] bool empty() {
        std::lock_guard lk(mutex_);
        return queue_.empty();
    }

    T pop() {
        std::unique_lock lk(mutex_);
        if (queue_.empty()) return T{};
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    void clear() {
        std::lock_guard lk(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    bool done_ = false;
};


#endif //SAFEQUEUE_H
