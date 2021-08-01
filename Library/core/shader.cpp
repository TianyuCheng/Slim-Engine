#include <fstream>
#include "core/debug.h"
#include "core/shader.h"
#include "core/vkutils.h"

using namespace slim;

size_t GetSize(std::ifstream &file) {
    file.seekg(0, std::ios::end);
    int length = file.tellg();
    file.seekg(0, std::ios::beg);
    return length;
}

std::vector<uint8_t> LoadSpirvShader(const std::string &path) {
    std::ifstream file;
    file.open(path, std::ios::in | std::ios::binary);

    if (!file.good()) {
        throw std::runtime_error("Failed to load SPIRV shader!");
    }

    // read binary content
    int size = GetSize(file);
    std::vector<uint8_t> content(size);
    file.read(reinterpret_cast<char *>(content.data()), size);

    return content;
}

Shader::Shader(Device *device, ShaderType type, VkShaderStageFlagBits stage,
               const std::string &entry, const std::string &file)
    : device(device), entry(entry) {

    // load data
    std::vector<uint8_t> code;
    switch (type) {
        case ShaderType::SPIRV:
            code = LoadSpirvShader(file);
            break;
        case ShaderType::GLSL:
        case ShaderType::HLSL:
            throw std::runtime_error("Compiling HLSL/GLSL is not implemented!");
            break;
    }

    // create shader module
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    ErrorCheck(vkCreateShaderModule(*device, &createInfo, nullptr, &handle), "create shader");

    // prepare shader stage info
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.module = handle;
    info.pName = this->entry.c_str();
    info.stage = stage;
}

Shader::~Shader() {
    if (handle) {
        vkDestroyShaderModule(*device, handle, nullptr);
        handle = nullptr;
    }
}

VkPipelineShaderStageCreateInfo Shader::GetInfo() const { return info; }
