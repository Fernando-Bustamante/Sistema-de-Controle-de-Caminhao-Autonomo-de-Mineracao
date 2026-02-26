#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class ThreadSafeQueue {
public:
    // Adiciona item na fila
    void push(T val) {
        std::lock_guard<std::mutex> lock(mtx);
        q.push(val);
        cv.notify_one();
    }

    // Retira item (Bloqueia a thread se estiver vazia até chegar algo)
    T pop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !q.empty(); });
        T val = q.front();
        q.pop();
        return val;
    }

    // Retira item (NÃO bloqueia: retorna true se conseguiu, false se vazia)
    // ESSA ERA A FUNÇÃO QUE FALTAVA
    bool try_pop(T& val) {
        std::lock_guard<std::mutex> lock(mtx);
        if (q.empty()) {
            return false;
        }
        val = q.front();
        q.pop();
        return true;
    }

private:
    std::queue<T> q;
    std::mutex mtx;
    std::condition_variable cv;
};
