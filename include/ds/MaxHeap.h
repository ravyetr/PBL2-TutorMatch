#ifndef TUTOR_MATCH_MAX_HEAP_H
#define TUTOR_MATCH_MAX_HEAP_H

#include <vector>
#include <stdexcept>
#include <algorithm>

namespace TutorMatch {

template <typename T>
class MaxHeap {
private:
    std::vector<T> heap;

    void heapifyUp(size_t index) {
        while (index > 0) {
            size_t parent = (index - 1) / 2;
            if (heap[index] > heap[parent]) {
                std::swap(heap[index], heap[parent]);
                index = parent;
            } else {
                break;
            }
        }
    }

    void heapifyDown(size_t index) {
        size_t size = heap.size();
        while (true) {
            size_t largest = index;
            size_t left = 2 * index + 1;
            size_t right = 2 * index + 2;

            if (left < size && heap[left] > heap[largest]) {
                largest = left;
            }
            if (right < size && heap[right] > heap[largest]) {
                largest = right;
            }

            if (largest != index) {
                std::swap(heap[index], heap[largest]);
                index = largest;
            } else {
                break;
            }
        }
    }

public:
    MaxHeap() = default;

    void push(const T& val) {
        heap.push_back(val);
        heapifyUp(heap.size() - 1);
    }

    T pop() {
        if (heap.empty()) {
            throw std::underflow_error("MaxHeap is empty!");
        }
        T topVal = heap.front();
        heap.front() = heap.back();
        heap.pop_back();
        if (!heap.empty()) {
            heapifyDown(0);
        }
        return topVal;
    }

    const T& top() const {
        if (heap.empty()) {
            throw std::underflow_error("MaxHeap is empty!");
        }
        return heap.front();
    }

    bool empty() const { return heap.empty(); }
    size_t size() const { return heap.size(); }
    void clear() { heap.clear(); }

    std::vector<T> extractSortedTopK(size_t k) {
        std::vector<T> result;
        size_t count = std::min(k, heap.size());
        MaxHeap<T> tempHeap = *this; // Copy heap
        for (size_t i = 0; i < count; ++i) {
            result.push_back(tempHeap.pop());
        }
        return result;
    }
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_MAX_HEAP_H
