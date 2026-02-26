#pragma once
#include "types.h"
#include <vector>
#include <mutex>
#include <condition_variable>

class CircularBuffer {
public:
    CircularBuffer(size_t size = 200) : data(size), size(size), write_idx(0), read_fifo_idx(0) {}

    // Função WRITE que estava faltando
    void write(const SensorSample& sample) {
        std::lock_guard<std::mutex> lock(mtx);
        data[write_idx] = sample;
        write_idx = (write_idx + 1) % size;
        
        // Se o buffer encher, empurra o leitor FIFO
        if (write_idx == read_fifo_idx) {
            read_fifo_idx = (read_fifo_idx + 1) % size;
        }
        cv.notify_all();
    }

    SensorSample read_fifo() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return read_fifo_idx != write_idx; });
        SensorSample s = data[read_fifo_idx];
        read_fifo_idx = (read_fifo_idx + 1) % size;
        return s;
    }

    SensorSample read_latest() {
        std::lock_guard<std::mutex> lock(mtx);
        size_t idx = (write_idx == 0) ? size - 1 : write_idx - 1;
        return data[idx];
    }

private:
    std::vector<SensorSample> data;
    size_t size, write_idx, read_fifo_idx;
    std::mutex mtx;
    std::condition_variable cv;
};
