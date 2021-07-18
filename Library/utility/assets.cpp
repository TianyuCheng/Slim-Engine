#include <ghc/filesystem.hpp>
#include "utility/assets.h"

namespace slim {

    const std::string ToAssetPath(const std::string &file) {
        ghc::filesystem::path path(std::string(SLIM_ASSETS_DIRECTORY));
        path = path / "Assets" / file;
        return path.string();
    }

}
