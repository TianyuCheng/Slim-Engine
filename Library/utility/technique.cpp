#include "utility/technique.h"

using namespace slim;

Technique::Technique() {

}

Technique::~Technique() {

}

void Technique::AddPass(RenderQueue queue, const GraphicsPipelineDesc &desc) {
    passes.push_back(Pass { desc, queue, nullptr });
    queue2index.insert(std::make_pair(queue, passes.size() - 1));
}

void Technique::Bind(uint32_t index, RenderFrame *renderFrame, RenderPass *renderPass, CommandBuffer *commandBuffer) {
    // build pipeline
    Pass& pass = passes[index];
    pass.desc.SetRenderPass(renderPass);
    pass.desc.SetViewport(renderFrame->GetExtent());
    pass.pipeline = renderFrame->RequestPipeline(pass.desc);

    // bind pipeline
    commandBuffer->BindPipeline(pass.pipeline);
}

uint32_t Technique::QueueIndex(RenderQueue queue) const {
    return queue2index.at(queue);
}

PipelineLayout* Technique::Layout(uint32_t index) const {
    return passes[index].desc.Layout();
}
