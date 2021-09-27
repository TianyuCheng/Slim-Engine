#include "utility/filesystem.h"
#include "utility/assets.h"

namespace slim {

    const std::string GetUserAsset(const std::string &file) {
        filesystem::path path(std::string(SLIM_USR_ASSETS_DIRECTORY));
        path = path / file.c_str();
        return path.string();
    }

    const std::string GetLibraryAsset(const std::string &file) {
        filesystem::path path(std::string(SLIM_LIB_ASSETS_DIRECTORY));
        path = path / file.c_str();
        return path.string();
    }

}
