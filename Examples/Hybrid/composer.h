#ifndef SLIM_EXAMPLE_COMPOSER_H
#define SLIM_EXAMPLE_COMPOSER_H

#include <slim/slim.hpp>
#include "light.h"
#include "scene.h"
#include "gbuffer.h"

using namespace slim;

void AddComposerPass(RenderGraph& renderGraph,
                     ResourceBundle& bundle,
                     RenderGraph::Resource* colorBuffer,
                     MainScene* scene,
                     GBuffer* gbuffer);


#endif // SLIM_EXAMPLE_OVERLAY_H
