#ifndef SLIM_EXAMPLE_COMPOSER_H
#define SLIM_EXAMPLE_COMPOSER_H

#include <slim/slim.hpp>
#include "light.h"
#include "gbuffer.h"

using namespace slim;

void AddComposerPass(RenderGraph& renderGraph,
                     AutoReleasePool& pool,
                     RenderGraph::Resource* colorBuffer,
                     GBuffer* gbuffer,
                     DirectionalLight* light);


#endif // SLIM_EXAMPLE_OVERLAY_H
