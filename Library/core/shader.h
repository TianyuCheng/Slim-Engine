#ifndef SLIM_CORE_SHADER_H
#define SLIM_CORE_SHADER_H

#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <VmaUsage.h>

#include "core/device.h"
#include "utility/interface.h"

namespace slim {

    enum class ShaderType {
        SPIRV,
        GLSL,
        HLSL,
    };

    class Shader : public NotCopyable, public NotMovable, public ReferenceCountable, public TriviallyConvertible<VkShaderModule> {
    public:
        explicit Shader(Device *device, ShaderType type, VkShaderStageFlagBits stage, const std::string &entry, const std::string &path);
        virtual ~Shader();
        VkPipelineShaderStageCreateInfo GetInfo() const;
    private:
        Device* device = nullptr;
        std::string entry = "main";
        VkPipelineShaderStageCreateInfo info = {};
    };

    #define SHADER_VARIANT(NAME, TYPE, STAGE)                                                                                     \
    class NAME final : public Shader {                                                                                            \
    public:                                                                                                                       \
        NAME(Device *device, const std::string &entry, const std::string &path) : Shader(device, TYPE, STAGE, entry, path) { } \
        virtual ~NAME() { }                                                                                                       \
    };

    namespace spirv {
        SHADER_VARIANT(VertexShader,   ShaderType::SPIRV, VK_SHADER_STAGE_VERTEX_BIT);
        SHADER_VARIANT(FragmentShader, ShaderType::SPIRV, VK_SHADER_STAGE_FRAGMENT_BIT);
        SHADER_VARIANT(ComputeShader,  ShaderType::SPIRV, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    namespace glsl {
        SHADER_VARIANT(VertexShader,   ShaderType::GLSL, VK_SHADER_STAGE_VERTEX_BIT);
        SHADER_VARIANT(FragmentShader, ShaderType::GLSL, VK_SHADER_STAGE_FRAGMENT_BIT);
        SHADER_VARIANT(ComputeShader,  ShaderType::GLSL, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    namespace hlsl {
        SHADER_VARIANT(VertexShader,   ShaderType::HLSL, VK_SHADER_STAGE_VERTEX_BIT);
        SHADER_VARIANT(FragmentShader, ShaderType::HLSL, VK_SHADER_STAGE_FRAGMENT_BIT);
        SHADER_VARIANT(ComputeShader,  ShaderType::HLSL, VK_SHADER_STAGE_COMPUTE_BIT);
    }
    #undef SHADER_VARIANT

} // end of namespace slim

#endif // end of SLIM_CORE_SHADER_H
