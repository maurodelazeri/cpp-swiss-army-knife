#pragma once

#include "all.h"

template<typename T>
class BlockingQueue {
public:
    BlockingQueue() {

    }

    void put(const T &x) {
        std::unique_lock <std::mutex> lck(mutex);
        queue.push_back(x);
        notEmpty.notify_one(); // wait morphing saves us
    }

    void put(T &&x) {
        std::unique_lock <std::mutex> lck(mutex);
        queue.push_back(std::move(x));
        notEmpty.notify_one();
    }
    // FIXME: emplace()

    T take() {
        std::unique_lock <std::mutex> lck(mutex);
        // always use a while-loop, due to spurious wakeup
        while (queue.empty()) {
            notEmpty.wait(lck);
        }

        assert(!queue.empty());
        T front(std::move(queue.front()));
        //T front(queue.front());
        queue.pop_front();
        return front;
    }

    size_t size() const {
        std::unique_lock <std::mutex> lck(mutex);
        return queue.size();
    }

private:
    std::mutex mutex;
    std::condition_variable notEmpty;
    std::deque <T> queue;
};

