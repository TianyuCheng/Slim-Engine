#ifndef SLIM_UTILITY_INTERFACE_H
#define SLIM_UTILITY_INTERFACE_H

#include <atomic>
#include <optional>
#include "smartptr.h"

namespace slim {

    // This interface should disable all copy/assignment operators for subclasses
    class NotCopyable {
    public:
        NotCopyable() = default;
        virtual ~NotCopyable() = default;
        NotCopyable(const NotCopyable &) = delete;
        NotCopyable& operator=(const NotCopyable &) = delete;
    }; // end of NotCopyable interface


    // This interface should disable all move/assignment operators for subclasses
    class NotMovable {
    public:
        NotMovable() = default;
        virtual ~NotMovable() = default;
        NotMovable(NotMovable &&) = delete;
        NotMovable& operator=(NotMovable &&) = delete;
    }; // end of NotMovable interface


    // This interface should automatically support intrusive reference counting
    class ReferenceCountable {
    protected:
        ReferenceCount refCount = ReferenceCount();
    public:
        ReferenceCountable() = default;
        virtual ~ReferenceCountable() = default;
        ReferenceCount& RefCount() { return refCount; }
    };

    // This interface should automatically provide base value converting
    template <typename T>
    class TriviallyConvertible {
    protected:
        T handle;
    public:
        TriviallyConvertible() = default;
        virtual ~TriviallyConvertible() = default;
        operator T() { return handle; }
        operator T() const { return handle; }
    };

    template <typename T>
    using Optional = std::optional<T>;

} // end of namespace slim

#endif // end of SLIM_UTIL_INTERFACE_H
