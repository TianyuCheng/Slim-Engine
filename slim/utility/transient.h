#ifndef SLIM_UTIL_TRANSIENT_H
#define SLIM_UTIL_TRANSIENT_H

#include <cstdlib>

namespace slim {

    template <typename ObjectType, typename PoolType = typename ObjectType::PoolType>
    class Transient {
    public:
        explicit Transient()
            : pool(nullptr), object(nullptr), hash(0) {
        }

        explicit Transient(ObjectType *obj)
            : pool(nullptr), object(obj), hash(0) {
        }

        // needs a pool and objects
        // pool will automatically recycle when destroyed
        explicit Transient(PoolType *pool, ObjectType *object, size_t hash = 0)
            : pool(pool), object(object), hash(hash) {
        }

        explicit Transient(const Transient &obj) = delete;

        explicit Transient(Transient &&that) {
            pool = that.pool;
            object = that.object;
            hash = that.hash;

            // clear that object
            that.pool = nullptr;
            that.object = nullptr;
        }

        Transient& operator=(Transient &&that) {
            pool = that.pool;
            object = that.object;
            hash = that.hash;

            // clear that object
            that.pool = nullptr;
            that.object = nullptr;
            return *this;
        }

        void Reset() {
            if (pool) {
                pool->Recycle(object, hash);
                pool = nullptr;
                object = nullptr;
            }
        }

        virtual ~Transient() {
            Reset();
        }

        operator ObjectType*()       { return object; }
        operator ObjectType*() const { return object; }

        ObjectType* operator->()       { return object; }
        ObjectType* operator->() const { return object; }

        ObjectType* get()       { return object; }
        ObjectType* get() const { return object; }

    private:
        PoolType *pool = nullptr;
        ObjectType *object = nullptr;
        size_t hash = 0;
    }; // end of Transient class

} // end of slim namespace

#endif // SLIM_UTIL_TRANSIENT_H
