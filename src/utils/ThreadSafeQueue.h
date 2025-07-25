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
    void push(const T& item) {
        {
            std::lock_guard lk(mutex_);
            queue_.push(item);
        }
        condVar_.notify_one();
    }

    void done() {
        {
            std::lock_guard lk(mutex_);
            done_ = true;
        }
        condVar_.notify_all();
    }

    [[nodiscard]] bool empty() {
        std::lock_guard lk(mutex_);
        return queue_.empty();
    }

    int size() {
        std::lock_guard lk(mutex_);
        return queue_.size();
    }

    T pop() {
        std::unique_lock lk(mutex_);
        if (queue_.empty()) return nullptr;
        T item = queue_.front();
        queue_.pop();
        return item;
    }

    T popBlocking() {
        std::unique_lock lk(mutex_);
        condVar_.wait(lk, [&]{ return done_ || !queue_.empty(); });
        if (done_ && queue_.empty()) return nullptr;

        T item = queue_.front();
        queue_.pop();
        return item;
    }

private:
    std::queue<T>           queue_;
    std::mutex              mutex_;
    std::condition_variable condVar_;
    bool                    done_ = false;
};


#endif //SAFEQUEUE_H
