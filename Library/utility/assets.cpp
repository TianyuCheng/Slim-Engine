#include "utility/filesystem.h"
#include "utility/assets.h"

namespace slim {

    const std::string ToAssetPath(const std::string &file) {
        filesystem::path path(std::string(SLIM_ASSETS_DIRECTORY));
        path = path / "Assets" / file;
        return path.string();
    }

}
