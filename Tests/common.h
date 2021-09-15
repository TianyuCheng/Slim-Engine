#ifndef SLIM_TEST_COMMON_H
#define SLIM_TEST_COMMON_H

#include <vector>
#include <gtest/gtest.h>
#include <slim/slim.hpp>

using namespace slim;

template <typename T>
std::vector<T> GenerateSequence(size_t num) {
    std::vector<T> data(num);
    for (size_t i = 0; i < num; i++) {
        data[i] = i;
    }
    return data;
}

template <typename T>
void CompareSequence(T* expected, T* actual, size_t num) {
    for (size_t i = 0; i < num; i++) {
        EXPECT_EQ(expected[i], actual[i]);
    }
}

std::vector<uint32_t>  GenerateCheckerboard(size_t width, size_t height);
SmartPtr<GPUImage>     GenerateCheckerboard(Device* device, size_t width, size_t height);
SmartPtr<VertexBuffer> GenerateQuadVertices(Device* device);
SmartPtr<IndexBuffer>  GenerateQuadIndices(Device* device);

#endif // SLIM_TEST_COMMON_H
