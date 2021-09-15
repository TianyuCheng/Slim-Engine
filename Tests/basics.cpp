#include "common.h"

// Test slim context creation
TEST(SlimSetup, Context) {
    auto contextDesc = ContextDesc();
    auto context= SlimPtr<Context>(contextDesc);
    EXPECT_NE(context->GetInstance(), VK_NULL_HANDLE);
}

// Test slim context creation with compute
TEST(SlimSetup, ContextWithCompute) {
    auto contextDesc = ContextDesc()
        .EnableValidation()
        .EnableCompute();
    auto context= SlimPtr<Context>(contextDesc);
    auto device = SlimPtr<Device>(context);
    EXPECT_NE(device->GetComputeQueue(), VK_NULL_HANDLE);
}

// Test slim context creation with graphics
TEST(SlimSetup, ContextWithGraphics) {
    auto contextDesc = ContextDesc()
        .EnableValidation()
        .EnableGraphics();
    auto context= SlimPtr<Context>(contextDesc);
    auto device = SlimPtr<Device>(context);
    EXPECT_NE(device->GetGraphicsQueue(), VK_NULL_HANDLE);
}

// Test slim context creation with present
TEST(SlimSetup, ContextWithPresent) {
    auto contextDesc = ContextDesc()
        .EnableValidation()
        .EnableGLFW();
    auto context= SlimPtr<Context>(contextDesc);
    auto device = SlimPtr<Device>(context);
    EXPECT_NE(device->GetPresentQueue(), VK_NULL_HANDLE);
}

int main(int argc, char **argv) {
    // prepare for slim environment
    slim::Initialize();

    // run selected tests
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
