#pragma once

#include <memory>
#include <vector>

namespace kat {

    template<typename T>
    T *smalloc() {
        return static_cast<T *>(malloc(sizeof(T)));
    };

    template<typename T, size_t N>
    T *smalloc() {
        return static_cast<T *>(malloc(sizeof(T) * N));
    };

    template<typename T>
    T *smalloc(const size_t &n) {
        return static_cast<T *>(malloc(sizeof(T) * n));
    };

    template<typename T>
    T *smalloc(T &&value) {
        T *ptr = smalloc<T>();
        std::memcpy(ptr, &value, sizeof(T));
        return ptr;
    };

    class stack {
        std::vector<void *> blocks;

      public:
        inline stack() : blocks(){};

        inline ~stack() {
            this->free();
        };

        inline void free() {
            for (void *b: blocks) {
                std::free(b);
            }

            blocks.clear();
        }

        template<typename T>
        T *smalloc() {
            return static_cast<T *>(this->malloc(sizeof(T)));
        };

        template<typename T, size_t N>
        T *smalloc() {
            return static_cast<T *>(this->malloc(sizeof(T) * N));
        };

        template<typename T>
        T *smalloc(const size_t &n) {
            return static_cast<T *>(this->malloc(sizeof(T) * n));
        };

        template<typename T>
        T *smalloc(T &&value) {
            T *ptr = smalloc<T>();
            std::memcpy(ptr, &value, sizeof(T));
            return ptr;
        };

        void *malloc(size_t size) {
            void *p = std::malloc(size);
            blocks.push_back(p);
            return p;
        };

        stack(stack &&) = delete;
        stack(const stack &) = delete;
        stack &operator=(stack &&) = delete;
        stack &operator=(const stack &) = delete;
    };
}