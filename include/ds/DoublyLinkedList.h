#ifndef TUTOR_MATCH_DOUBLY_LINKED_LIST_H
#define TUTOR_MATCH_DOUBLY_LINKED_LIST_H

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace TutorMatch {

template <typename T>
class DoublyLinkedList {
public:
    struct Node {
        T data;
        Node* prev;
        Node* next;
        Node(const T& val) : data(val), prev(nullptr), next(nullptr) {}
    };

private:
    Node* head;
    Node* tail;
    size_t listSize;

public:
    DoublyLinkedList() : head(nullptr), tail(nullptr), listSize(0) {}
    ~DoublyLinkedList() { clear(); }

    DoublyLinkedList(const DoublyLinkedList&) = delete;
    DoublyLinkedList& operator=(const DoublyLinkedList&) = delete;

    Node* getHead() const { return head; }
    Node* getTail() const { return tail; }

    Node* pushFront(const T& val) {
        Node* newNode = new Node(val);
        newNode->next = head;
        if (head) {
            head->prev = newNode;
        } else {
            tail = newNode;
        }
        head = newNode;
        listSize++;
        return newNode;
    }

    Node* pushBack(const T& val) {
        Node* newNode = new Node(val);
        newNode->prev = tail;
        if (tail) {
            tail->next = newNode;
        } else {
            head = newNode;
        }
        tail = newNode;
        listSize++;
        return newNode;
    }

    T popBack() {
        if (!tail) {
            throw std::underflow_error("List is empty!");
        }
        Node* temp = tail;
        T val = temp->data;
        tail = tail->prev;
        if (tail) {
            tail->next = nullptr;
        } else {
            head = nullptr;
        }
        delete temp;
        listSize--;
        return val;
    }

    void removeNode(Node* node) {
        if (!node) return;
        if (node->prev) {
            node->prev->next = node->next;
        } else {
            head = node->next;
        }

        if (node->next) {
            node->next->prev = node->prev;
        } else {
            tail = node->prev;
        }

        delete node;
        listSize--;
    }

    void moveToFront(Node* node) {
        if (!node || node == head) return;

        if (node->prev) node->prev->next = node->next;
        if (node->next) node->next->prev = node->prev;
        if (node == tail) tail = node->prev;

        node->prev = nullptr;
        node->next = head;
        if (head) head->prev = node;
        head = node;
    }

    size_t size() const { return listSize; }
    bool empty() const { return listSize == 0; }

    void clear() {
        Node* curr = head;
        while (curr) {
            Node* next = curr->next;
            delete curr;
            curr = next;
        }
        head = nullptr;
        tail = nullptr;
        listSize = 0;
    }
};

} // namespace TutorMatch

#endif // TUTOR_MATCH_DOUBLY_LINKED_LIST_H
