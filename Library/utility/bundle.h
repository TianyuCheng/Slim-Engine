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

        Shader* AutoRelease(Shader* shader) {
            shaders.emplace_back(shader);
            return shader;
        }

        Image* AutoRelease(Image* image) {
            images.emplace_back(image);
            return image;
        }

        Sampler* AutoRelease(Sampler* sampler) {
            samplers.emplace_back(sampler);
            return sampler;
        }

        Pipeline* AutoRelease(Pipeline* pipeline) {
            pipelines.emplace_back(pipeline);
            return pipeline;
        }

    private:
        std::vector<SmartPtr<Image>>    images;
        std::vector<SmartPtr<Sampler>>  samplers;
        std::vector<SmartPtr<Pipeline>> pipelines;
        std::vector<SmartPtr<Shader>>   shaders;
    };

} // end of slim

#endif // SLIM_UTILITY_BUNDLE_H
