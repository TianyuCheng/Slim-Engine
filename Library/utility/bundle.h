#ifndef SLIM_UTILITY_BUNDLE_H
#define SLIM_UTILITY_BUNDLE_H

#include <vector>
#include "core/shader.h"
#include "core/sampler.h"
#include "core/pipeline.h"
#include "utility/interface.h"

namespace slim {

    struct ResourceBundle {
    public:

        void AutoRelease(Shader* shader) {
            shaders.emplace_back(shader);
        }

        void AutoRelease(Image* image) {
            images.emplace_back(image);
        }

        void AutoRelease(Sampler* sampler) {
            samplers.emplace_back(sampler);
        }

        void AutoRelease(Pipeline* pipeline) {
            pipelines.emplace_back(pipeline);
        }

    private:
        std::vector<SmartPtr<Image>>    images;
        std::vector<SmartPtr<Sampler>>  samplers;
        std::vector<SmartPtr<Pipeline>> pipelines;
        std::vector<SmartPtr<Shader>>   shaders;
    };

} // end of slim

#endif // SLIM_UTILITY_BUNDLE_H
