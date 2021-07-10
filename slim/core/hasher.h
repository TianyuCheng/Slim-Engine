#ifndef SLIM_CORE_HASHER_H
#define SLIM_CORE_HASHER_H

#include <memory>

namespace slim {

    template <class Arg>
    size_t HashCombine(size_t seed, const Arg &v) {
        std::hash<Arg> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        return seed;
    }

    template <class Arg0, class ...Arg1>
    size_t HashCombine(size_t seed, const Arg0 &v, const Arg1...rest) {
        seed = HashCombine(seed, v);
        seed = HashCombine(seed, rest...);
        return seed;
    }

    struct PairHash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2> &p) const {
            return slim::HashCombine(p.first, p.second);
        }
    };

} // end of namespace slim::core

#endif // SLIM_CORE_HASHER_H
