#include "utility/material.h"

using namespace slim;

Material::Material(const std::string &name, Queue queue) : name(name), queue(queue) {
}

void Material::Bind(const RenderInfo &renderInfo) {
    // update pipeline viewport
    desc.SetViewport(renderInfo.renderFrame->GetExtent());
    desc.SetRenderPass(renderInfo.renderPass);

    // request and bind pipeline
    pipeline = renderInfo.renderFrame->RequestPipeline(desc);
    renderInfo.commandBuffer->BindPipeline(pipeline);
}
