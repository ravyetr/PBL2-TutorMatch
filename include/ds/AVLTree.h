#ifndef TUTOR_MATCH_AVL_TREE_H
#define TUTOR_MATCH_AVL_TREE_H

#include <algorithm>
#include <vector>
#include <iostream>

namespace TutorMatch {

template <typename Key, typename Value>
class AVLTree {
private:
    struct AVLNode {
        Key key;
        std::vector<Value> values;
        int height;
        AVLNode* left;
        AVLNode* right;

        AVLNode(const Key& k, const Value& v)
            : key(k), values{v}, height(1), left(nullptr), right(nullptr) {}
    };

    AVLNode* root;
    size_t nodeCount;

    int getHeight(AVLNode* node) const {
        return node ? node->height : 0;
    }

    int getBalanceFactor(AVLNode* node) const {
        return node ? getHeight(node->left) - getHeight(node->right) : 0;
    }

    void updateHeight(AVLNode* node) {
        if (node) {
            node->height = 1 + std::max(getHeight(node->left), getHeight(node->right));
        }
    }

    AVLNode* rotateRight(AVLNode* y) {
        AVLNode* x = y->left;
        AVLNode* T2 = x->right;

        x->right = y;
        y->left = T2;

        updateHeight(y);
        updateHeight(x);

        return x; // New root
    }

    AVLNode* rotateLeft(AVLNode* x) {
        AVLNode* y = x->right;
        AVLNode* T2 = y->left;

        y->left = x;
        x->right = T2;

        updateHeight(x);
        updateHeight(y);

        return y; // New root
    }

    AVLNode* rebalance(AVLNode* node) {
        if (!node) return nullptr;

        int balance = getBalanceFactor(node);

        // Left Left Case
        if (balance > 1 && getBalanceFactor(node->left) >= 0) {
            return rotateRight(node);
        }
        // Left Right Case
        if (balance > 1 && getBalanceFactor(node->left) < 0) {
            node->left = rotateLeft(node->left);
            return rotateRight(node);
        }
        // Right Right Case
        if (balance < -1 && getBalanceFactor(node->right) <= 0) {
            return rotateLeft(node);
        }
        // Right Left Case
        if (balance < -1 && getBalanceFactor(node->right) > 0) {
            node->right = rotateRight(node->right);
            return rotateLeft(node);
        }

        return node;
    }

    AVLNode* findMin(AVLNode* node) const {
        while (node && node->left) {
            node = node->left;
        }
        return node;
    }

    AVLNode* insertNode(AVLNode* node, const Key& key, const Value& value) {
        if (!node) {
            nodeCount++;
            return new AVLNode(key, value);
        }

        if (key < node->key) {
            node->left = insertNode(node->left, key, value);
        } else if (key > node->key) {
            node->right = insertNode(node->right, key, value);
        } else {
            // Key exists; add value if not already present
            if (std::find(node->values.begin(), node->values.end(), value) == node->values.end()) {
                node->values.push_back(value);
            }
            return node;
        }

        updateHeight(node);
        return rebalance(node);
    }

    AVLNode* deleteKeyNode(AVLNode* node, const Key& key) {
        if (!node) return nullptr;

        if (key < node->key) {
            node->left = deleteKeyNode(node->left, key);
        } else if (key > node->key) {
            node->right = deleteKeyNode(node->right, key);
        } else {
            // Node found to delete completely
            if (!node->left || !node->right) {
                nodeCount--;
                AVLNode* temp = node->left ? node->left : node->right;
                delete node;
                return temp;
            } else {
                AVLNode* succ = findMin(node->right);
                node->key = succ->key;
                node->values = succ->values;
                node->right = deleteKeyNode(node->right, succ->key);
            }
        }

        if (!node) return nullptr;

        updateHeight(node);
        return rebalance(node);
    }

    AVLNode* deleteNode(AVLNode* node, const Key& key, const Value& value, bool& foundAndRemoved) {
        if (!node) return nullptr;

        if (key < node->key) {
            node->left = deleteNode(node->left, key, value, foundAndRemoved);
        } else if (key > node->key) {
            node->right = deleteNode(node->right, key, value, foundAndRemoved);
        } else {
            // Key matches
            auto it = std::find(node->values.begin(), node->values.end(), value);
            if (it != node->values.end()) {
                node->values.erase(it);
                foundAndRemoved = true;
            }

            // If there are still values associated with this key, retain the node
            if (!node->values.empty()) {
                return node;
            }

            // Node has no values left; remove the node from the tree
            if (!node->left || !node->right) {
                nodeCount--;
                AVLNode* temp = node->left ? node->left : node->right;
                delete node;
                return temp;
            } else {
                AVLNode* succ = findMin(node->right);
                node->key = succ->key;
                node->values = succ->values;
                node->right = deleteKeyNode(node->right, succ->key);
            }
        }

        if (!node) return nullptr;

        updateHeight(node);
        return rebalance(node);
    }

    void rangeSearchHelper(AVLNode* node, const Key& minKey, const Key& maxKey, std::vector<Value>& result) const {
        if (!node) return;

        if (minKey < node->key) {
            rangeSearchHelper(node->left, minKey, maxKey, result);
        }

        if (node->key >= minKey && node->key <= maxKey) {
            result.insert(result.end(), node->values.begin(), node->values.end());
        }

        if (maxKey > node->key) {
            rangeSearchHelper(node->right, minKey, maxKey, result);
        }
    }

    void destroyTree(AVLNode* node) {
        if (node) {
            destroyTree(node->left);
            destroyTree(node->right);
            delete node;
        }
    }

public:
    AVLTree() : root(nullptr), nodeCount(0) {}
    ~AVLTree() { destroyTree(root); }

    void insert(const Key& key, const Value& value) {
        root = insertNode(root, key, value);
    }

    bool remove(const Key& key, const Value& value) {
        bool removed = false;
        root = deleteNode(root, key, value, removed);
        return removed;
    }

    bool removeKey(const Key& key) {
        size_t oldSize = nodeCount;
        root = deleteKeyNode(root, key);
        return nodeCount < oldSize;
    }

    bool contains(const Key& key, const Value& value) const {
        AVLNode* curr = root;
        while (curr) {
            if (key < curr->key) {
                curr = curr->left;
            } else if (key > curr->key) {
                curr = curr->right;
            } else {
                return std::find(curr->values.begin(), curr->values.end(), value) != curr->values.end();
            }
        }
        return false;
    }

    std::vector<Value> rangeSearch(const Key& minKey, const Key& maxKey) const {
        std::vector<Value> result;
        rangeSearchHelper(root, minKey, maxKey, result);
        return result;
    }

    size_t size() const { return nodeCount; }
    bool empty() const { return nodeCount == 0; }
    void clear() {
        destroyTree(root);
        root = nullptr;
        nodeCount = 0;
    }
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_AVL_TREE_H
