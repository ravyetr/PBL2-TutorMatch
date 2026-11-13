#ifndef TUTOR_MATCH_HASH_TABLE_H
#define TUTOR_MATCH_HASH_TABLE_H

#include <vector>
#include <list>
#include <utility>
#include <functional>
#include <stdexcept>
#include <string>

namespace TutorMatch {

// Custom Hash Table using Separate Chaining for O(1) average lookup
template <typename K, typename V, typename Hash = std::hash<K>>
class HashTable {
private:
    struct HashNode {
        K key;
        V value;
        HashNode(const K& k, const V& v) : key(k), value(v) {}
    };

    std::vector<std::list<HashNode>> buckets;
    size_t numElements;
    float maxLoadFactor;
    Hash hasher;

    size_t getBucketIndex(const K& key) const {
        return hasher(key) % buckets.size();
    }

    void rehash(size_t newCapacity) {
        std::vector<std::list<HashNode>> oldBuckets = std::move(buckets);
        buckets.resize(newCapacity);
        numElements = 0;

        for (auto& bucket : oldBuckets) {
            for (auto& node : bucket) {
                insert(node.key, node.value);
            }
        }
    }

public:
    explicit HashTable(size_t initialCapacity = 16, float loadFactor = 0.75f)
        : buckets(initialCapacity), numElements(0), maxLoadFactor(loadFactor) {}

    void insert(const K& key, const V& value) {
        if ((float)(numElements + 1) / buckets.size() > maxLoadFactor) {
            rehash(buckets.size() * 2);
        }

        size_t idx = getBucketIndex(key);
        for (auto& node : buckets[idx]) {
            if (node.key == key) {
                node.value = value; // Update existing
                return;
            }
        }

        buckets[idx].emplace_back(key, value);
        numElements++;
    }

    bool find(const K& key, V& outValue) const {
        size_t idx = getBucketIndex(key);
        for (const auto& node : buckets[idx]) {
            if (node.key == key) {
                outValue = node.value;
                return true;
            }
        }
        return false;
    }

    V* get(const K& key) {
        size_t idx = getBucketIndex(key);
        for (auto& node : buckets[idx]) {
            if (node.key == key) {
                return &node.value;
            }
        }
        return nullptr;
    }

    bool contains(const K& key) const {
        V val;
        return find(key, val);
    }

    bool remove(const K& key) {
        size_t idx = getBucketIndex(key);
        auto& bucket = buckets[idx];
        for (auto it = bucket.begin(); it != bucket.end(); ++it) {
            if (it->key == key) {
                bucket.erase(it);
                numElements--;
                return true;
            }
        }
        return false;
    }

    size_t size() const { return numElements; }
    bool empty() const { return numElements == 0; }

    void clear() {
        for (auto& bucket : buckets) {
            bucket.clear();
        }
        numElements = 0;
    }

    std::vector<std::pair<K, V>> getAll() const {
        std::vector<std::pair<K, V>> result;
        result.reserve(numElements);
        for (const auto& bucket : buckets) {
            for (const auto& node : bucket) {
                result.emplace_back(node.key, node.value);
            }
        }
        return result;
    }
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_HASH_TABLE_H
