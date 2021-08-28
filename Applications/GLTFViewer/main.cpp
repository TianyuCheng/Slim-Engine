#include <slim/slim.hpp>
#include "viewer.h"

using namespace slim;

int main() {
    slim::Initialize();
    GLTFViewer().Run();
    return EXIT_SUCCESS;
}
