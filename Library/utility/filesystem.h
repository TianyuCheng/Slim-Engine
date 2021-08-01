#ifndef SLIM_UTILITY_FILESYSTEM_H
#define SLIM_UTILITY_FILESYSTEM_H

#include <ghc/filesystem.hpp>

namespace slim {
    namespace filesystem = ghc::filesystem;
    using path = filesystem::path;
}

#endif // end of SLIM_UTILITY_FILESYSTEM_H
