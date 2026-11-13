#ifndef TUTOR_MATCH_LRU_CACHE_H
#define TUTOR_MATCH_LRU_CACHE_H

#include "business/algorithms/DoublyLinkedList.h"
#include <unordered_map>
#include <utility>
#include <stdexcept>
#include <mutex>

namespace TutorMatch {

template <typename Key, typename Value>
class LRUCache {
private:
    typedef std::pair<Key, Value> KeyValuePair;
    size_t capacity;
    DoublyLinkedList<KeyValuePair> itemList;
    std::unordered_map<Key, typename DoublyLinkedList<KeyValuePair>::Node*> cacheMap;
    mutable std::mutex cacheMutex;

public:
    explicit LRUCache(size_t capacity = 50) : capacity(capacity) {}

    bool get(const Key& key, Value& outValue) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        auto it = cacheMap.find(key);
        if (it == cacheMap.end()) {
            return false; // Cache Miss
        }

        // Cache Hit: Move accessed item to front of doubly-linked list (Most Recently Used)
        itemList.moveToFront(it->second);
        outValue = it->second->data.second;
        return true;
    }

    void put(const Key& key, const Value& value) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        auto it = cacheMap.find(key);

        if (it != cacheMap.end()) {
            // Update existing entry and move to front
            it->second->data.second = value;
            itemList.moveToFront(it->second);
            return;
        }

        // Evict Least Recently Used (LRU) item if capacity reached
        if (cacheMap.size() >= capacity && capacity > 0) {
            auto tailNode = itemList.getTail();
            if (tailNode) {
                cacheMap.erase(tailNode->data.first);
                itemList.popBack();
            }
        }

        // Insert new entry at front
        auto newNode = itemList.pushFront(KeyValuePair(key, value));
        cacheMap[key] = newNode;
    }

    bool remove(const Key& key) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        auto it = cacheMap.find(key);
        if (it != cacheMap.end()) {
            itemList.removeNode(it->second);
            cacheMap.erase(it);
            return true;
        }
        return false;
    }

    void clear() {
        std::lock_guard<std::mutex> lock(cacheMutex);
        itemList.clear();
        cacheMap.clear();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(cacheMutex);
        return cacheMap.size();
    }

    size_t getCapacity() const { return capacity; }
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_LRU_CACHE_H
