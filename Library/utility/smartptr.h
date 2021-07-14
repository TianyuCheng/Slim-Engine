#ifndef SLIM_UTILITY_SMART_PTR_H
#define SLIM_UTILITY_SMART_PTR_H

#include <atomic>

namespace slim {

    class ReferenceCount final {
        std::atomic<int> count;
    public:
        ReferenceCount() : count(0) { }
        virtual ~ReferenceCount() = default;
        int Increment() { return ++count; }
        int Decrement() { return --count; }
        bool IsZero() const { return count == 0; }
    };

    // an implementation of a simple intrusive pointer
    template <typename T>
    class SmartPtr {
    private:
        void Increment(T *p) {
            if (!p) return;
            p->RefCount().Increment();
        }

        void Decrement(T *p) {
            if (!p) return;
            if (0 == p->RefCount().Decrement()) {
                if (p == ptr) {
                    ptr = nullptr;
                }
                delete p;
            }
        }

    protected:
        T *ptr;

    public:

        inline SmartPtr() : ptr(nullptr) {
        }

        inline SmartPtr(T *p) : ptr(p) {
            Increment(ptr);
        }

        inline SmartPtr(SmartPtr<T> &&other) : ptr(other.ptr) {
            other.ptr = nullptr;
        }

        inline SmartPtr(const SmartPtr<T> &other) : ptr(other.ptr) {
            Increment(ptr);
        }

        virtual ~SmartPtr() {
            Decrement(ptr);
        }

        inline SmartPtr<T>& operator=(const SmartPtr<T> &other) {
            if (other.ptr == ptr) return *this;

            T *tmp = other.ptr;
            Increment(tmp);
            Decrement(ptr);
            ptr = tmp;

            return *this;
        }

        inline SmartPtr<T> &operator=(SmartPtr<T> &&other) {
            std::swap(ptr, other.ptr);
            return *this;
        }

        T* get() const { return ptr; }

        T* operator->() const { return ptr; }

        T& operator*() const { return *ptr; }

        operator T*() { return ptr; }
        operator T*() const { return ptr; }

        void reset() {
            Decrement(ptr);
            ptr = nullptr;
        }

        void reset(T* p) {
            Increment(p);
            Decrement(ptr);
            ptr = p;
        }
    };

    template <typename T, typename ... Args>
    SmartPtr<T> SlimPtr(Args ... args) {
        return SmartPtr(new T(args...));
    }

    template <typename T>
    SmartPtr<T> SlimPtr(T *obj) {
        return SmartPtr(obj);
    }

} // end of namespace slim

#endif // end of SLIM_UTIL_SMART_PTR_H
