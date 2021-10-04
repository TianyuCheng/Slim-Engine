#ifndef SLIM_UTILITY_BUNDLE_H
#define SLIM_UTILITY_BUNDLE_H

#include <vector>
#include <functional>
#include <unordered_map>
#include "core/shader.h"
#include "core/sampler.h"
#include "core/pipeline.h"
#include "utility/interface.h"

namespace slim {

    class AutoReleasePool {
    public:

        AutoReleasePool(Device* device) : device(device) {
        }

        Shader* FetchOrCreate(const std::string& name,
                              const std::function<Shader*(Device*)>& createFn) {
            auto it = shaders.find(name);
            if (it == shaders.end()) {
                auto shader = createFn(device);
                shaders.insert(std::make_pair(name, shader));
                return shader;
            }
            return it->second.get();
        }

        Sampler* FetchOrCreate(const std::string& name,
                               const std::function<Sampler*(Device*)>& createFn) {
            auto it = samplers.find(name);
            if (it == samplers.end()) {
                auto sampler = createFn(device);
                samplers.insert(std::make_pair(name, sampler));
                return sampler;
            }
            return it->second.get();
        }

        Image* FetchOrCreate(const std::string& name,
                             const std::function<Image*(Device*)>& createFn) {
            auto it = images.find(name);
            if (it == images.end()) {
                auto image = createFn(device);
                images.insert(std::make_pair(name, image));
                return image;
            }
            return it->second.get();
        }

        Pipeline* FetchOrCreate(const std::string& name,
                                const std::function<Pipeline*(Device*)>& createFn) {
            auto it = pipelines.find(name);
            if (it == pipelines.end()) {
                auto pipeline = createFn(device);
                pipelines.insert(std::make_pair(name, pipeline));
                return pipeline;
            }
            return it->second.get();
        }

    private:
        SmartPtr<Device>                                    device;
        std::unordered_map<std::string, SmartPtr<Image>>    images;
        std::unordered_map<std::string, SmartPtr<Sampler>>  samplers;
        std::unordered_map<std::string, SmartPtr<Shader>>   shaders;
        std::unordered_map<std::string, SmartPtr<Pipeline>> pipelines;
    };

    class ResourceBundle {
    public:

        [[ deprecated ]]
        Shader* AutoRelease(Shader* shader) {
            shaders.emplace_back(shader);
            return shader;
        }

        [[ deprecated ]]
        Image* AutoRelease(Image* image) {
            images.emplace_back(image);
            return image;
        }

        [[ deprecated ]]
        Sampler* AutoRelease(Sampler* sampler) {
            samplers.emplace_back(sampler);
            return sampler;
        }

        [[ deprecated ]]
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
