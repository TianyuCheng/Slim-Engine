#include "config.h"
#include "scene.h"

Scene::Scene(Device* device)
    : device(device) {
    InitScene();
    InitFrame();
    InitCamera();
    InitLights();
    InitSurfels();
    InitGeometry();

    lightDebugControl = {};
    surfelDebugControl = {};
}

void Scene::InitScene() {
    // model loading
    builder = SlimPtr<scene::Builder>(device);
    builder->EnableRayTracing();
    builder->GetAccelBuilder()->EnableCompaction();

    // load model
    #ifdef ENABLE_SPONZA_SCENE
    model.Load(builder, GetUserAsset("Scenes/Sponza/glTF/Sponza.gltf"));
    model.GetScene(0)->ApplyTransform();
    #endif

    #ifdef ENABLE_COLOR_BLEEDING_SCENE
    model.Load(builder, GetUserAsset("Scenes/ColorBleeding/colorbleeding.gltf"));
    model.GetScene(0)->Scale(0.4, 0.4, 0.4);
    model.GetScene(0)->ApplyTransform();
    #endif

    // build scene
    builder->Build();

    // prepare images and samplers
    for (auto& image   : model.images  ) images.push_back(image);
    for (auto& sampler : model.samplers) samplers.push_back(sampler);

    // material ID mappings
    std::unordered_map<scene::Material*, uint32_t> material2id = {};

    // materials
    std::vector<MaterialInfo> materials;
    builder->ForEachMaterial<MaterialInfo>(materials,
        [&](MaterialInfo& m, scene::Material* material) {
            const auto& data = material->GetData<gltf::MaterialData>();
            m.baseColor = data.baseColor;
            m.emissiveColor = data.emissive;
            m.baseColorTexture = data.baseColorTexture;
            m.baseColorSampler = data.baseColorSampler;
            m.emissiveTexture = data.emissiveTexture;
            m.emissiveSampler = data.emissiveSampler;
            material2id[material] = material2id.size();
        });
    materialBuffer = SlimPtr<Buffer>(device, sizeof(MaterialInfo) * materials.size(),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    materialBuffer->SetName("Materials");

    // instances
    std::vector<InstanceInfo> instances;
    builder->ForEachInstance<InstanceInfo>(instances,
        [&](InstanceInfo& i, scene::Node* node, scene::Mesh* mesh, scene::Material* material, uint32_t instanceID) {
            i.M = node->GetTransform().LocalToWorld();
            i.N = glm::transpose(glm::inverse(i.M));
            i.instanceID = instanceID;
            i.materialID = material2id[material];
            i.indexAddress = mesh->GetIndexAddress(device);
            i.vertexAddress = mesh->GetVertexAddress(device, 0);
            i.materialAddress = device->GetDeviceAddress(materialBuffer);
        });
    instanceBuffer = SlimPtr<Buffer>(device, sizeof(InstanceInfo) * instances.size(),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    instanceBuffer->SetName("Instances");

    // copy data
    device->Execute([&](CommandBuffer* commandBuffer) {
        commandBuffer->CopyDataToBuffer(materials, materialBuffer);
        commandBuffer->CopyDataToBuffer(instances, instanceBuffer);
    });
}

void Scene::InitFrame() {
    frameInfoBuffer = SlimPtr<Buffer>(device,
        sizeof(FrameInfo),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    frameInfoBuffer->SetName("FrameInfo");
}

void Scene::InitCamera() {
    // camera
    camera = SlimPtr<Flycam>("camera");
    #ifdef ENABLE_MINUSCALE_SCENE

        #ifdef ENABLE_SPONZA_SCENE
        walkSpeed = 1.0f;
        camera->LookAt(glm::vec3(0.03, 1.35, 0.0), glm::vec3(0.0, 1.35, 0.0), glm::vec3(0.0, 1.0, 0.0));
        #endif

        #ifdef ENABLE_COLOR_BLEEDING_SCENE
        walkSpeed = 5.0f;
        camera->LookAt(glm::vec3(-5.0, 1.35, 1.0), glm::vec3(0.0, 1.35, 1.0), glm::vec3(0.0, 1.0, 0.0));
        #endif

    #else
    walkSpeed = 10.0f;
    camera->LookAt(glm::vec3(3.0, .135, 0.0), glm::vec3(0.0, 0.135, 0.0), glm::vec3(0.0, 1.0, 0.0));
    #endif
    camera->SetWalkSpeed(walkSpeed);

    // create camera buffer
    cameraBuffer = SlimPtr<Buffer>(device,
        sizeof(CameraInfo),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    cameraBuffer->SetName("CameraInfo");
}

void Scene::InitLights() {
    sky = SkyInfo {
        vec3(1.0, 1.0, 1.0)
    };

    // init sky buffer
    skyBuffer = SlimPtr<Buffer>(device,
        sizeof(SkyInfo),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    skyBuffer->SetName("SkyInfo");

    // init light buffer
    lightBuffer = SlimPtr<Buffer>(device,
        sizeof(LightInfo) * 20,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    lightBuffer->SetName("LightInfo");

    // init light transform buffer
    lightXformBuffer = SlimPtr<Buffer>(device,
        sizeof(glm::mat4) * 20,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    lightXformBuffer->SetName("LightXform");
}

void Scene::InitSurfels() {
    // init surfel buffer
    surfelBuffer = SlimPtr<Buffer>(device,
        sizeof(Surfel) * SURFEL_CAPACITY,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    surfelBuffer->SetName("Surfel");

    // init surfel live buffer
    surfelLiveBuffer = SlimPtr<Buffer>(device,
        sizeof(uint32_t) * SURFEL_CAPACITY,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    surfelLiveBuffer->SetName("SurfelAlive");

    // init surfel data buffer
    surfelDataBuffer = SlimPtr<Buffer>(device,
        sizeof(SurfelData) * SURFEL_CAPACITY,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    surfelDataBuffer->SetName("SurfelData");

    // init surfel grid buffer
    surfelGridBuffer = SlimPtr<Buffer>(device,
        sizeof(SurfelGridCell) * SURFEL_GRID_COUNT,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    surfelGridBuffer->SetName("SurfelGrid");

    // init surfel cell buffer
    surfelCellBuffer = SlimPtr<Buffer>(device,
        sizeof(uint32_t) * SURFEL_GRID_COUNT * 64, // SURFEL_CELL_CAPACITY,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    surfelCellBuffer->SetName("SurfelCell");

    // init surfel stat buffer
    surfelStatBuffer = SlimPtr<Buffer>(device,
        sizeof(SurfelStat),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY);
    surfelStatBuffer->SetName("SurfelStat");

    // // init surfel ray direction visualization buffer
    // surfelRayDirBuffer = SlimPtr<Buffer>(device,
    //     sizeof(glm::vec3) * 2 * SURFEL_CAPACITY,
    //     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    //     VMA_MEMORY_USAGE_GPU_ONLY);
    // surfelRayDirBuffer->SetName("SurfelRayDir");

    // init surfel depth atlas storage texture
    VkExtent2D radialDepthExtent = {};
    radialDepthExtent.width = SURFEL_DEPTH_ATLAS_TEXELS;
    radialDepthExtent.height = SURFEL_DEPTH_ATLAS_TEXELS;
    surfelDepthImage = SlimPtr<GPUImage>(
        device, VK_FORMAT_R32G32_SFLOAT, radialDepthExtent,
        1, 1, VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    // init surfel ray guide atlas storage texture
    VkExtent2D rayGuideExtent = {};
    rayGuideExtent.width = SURFEL_RAYGUIDE_ATLAS_TEXELS;
    rayGuideExtent.height = SURFEL_RAYGUIDE_ATLAS_TEXELS;
    surfelRayGuideImage = SlimPtr<GPUImage>(
        device, VK_FORMAT_R32G32_SFLOAT, rayGuideExtent,
        1, 1, VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    ResetSurfels();
}

void Scene::InitGeometry() {
    geometryBuilder = SlimPtr<scene::Builder>(device);

    GeometryData cone = Cone { }.Create();
    GeometryData sphere = Sphere { }.Create();
    GeometryData cylinder = Cylinder { }.Create();

    // cone mesh
    coneMesh = geometryBuilder->CreateMesh();
    coneMesh->SetIndexBuffer(cone.indices);
    coneMesh->SetVertexBuffer(cone.vertices);

    // sphere mesh
    sphereMesh = geometryBuilder->CreateMesh();
    sphereMesh->SetIndexBuffer(sphere.indices);
    sphereMesh->SetVertexBuffer(sphere.vertices);

    // cylinder mesh
    cylinderMesh = geometryBuilder->CreateMesh();
    cylinderMesh->SetIndexBuffer(cylinder.indices);
    cylinderMesh->SetVertexBuffer(cylinder.vertices);

    geometryBuilder->Build();
}

float Scene::Near() const {
    #ifdef ENABLE_MINUSCALE_SCENE
    return 0.001;
    #else
    return 0.1;
    #endif
}

float Scene::Far() const {
    #ifdef ENABLE_MINUSCALE_SCENE
    return 40.0;
    #else
    return 4000.0;
    #endif
}

void Scene::ResetSurfels() {
    // copy data over
    device->Execute([&](CommandBuffer* commandBuffer) {
        SurfelStat stat = {};
        stat.count = 0;
        stat.alloc = 0;
        stat.pause = 0;
        stat.x = 1;
        stat.y = 1;
        stat.z = 1;
        commandBuffer->CopyDataToBuffer(stat, surfelStatBuffer);

        std::vector<uint32_t> freeSurfels(SURFEL_CAPACITY);
        for (uint32_t i = 0; i < SURFEL_CAPACITY; i++) {
            freeSurfels[i] = i;
        }
        commandBuffer->CopyDataToBuffer(freeSurfels, surfelLiveBuffer);

        std::vector<SurfelData> surfelData(SURFEL_CAPACITY);
        for (uint32_t i = 0; i < SURFEL_CAPACITY; i++) {
            surfelData[i].surfelID = i;
        }
        commandBuffer->CopyDataToBuffer(surfelData, surfelDataBuffer);

        // clear attachments
        float diameter = SURFEL_MAX_RADIUS * 2.0;
        VkClearColorValue clear = {};
        clear.float32[0] = diameter;
        clear.float32[1] = diameter * diameter;
        clear.float32[2] = 1.0;
        clear.float32[3] = 1.0;
        commandBuffer->ClearColor(surfelDepthImage, clear);
        commandBuffer->ClearColor(surfelRayGuideImage, clear);
    });
}

void Scene::PauseSurfels() {
    device->Execute([&](CommandBuffer* commandBuffer) {
        uint32_t pause = 1;
        commandBuffer->CopyDataToBuffer(pause, surfelStatBuffer, offsetof(SurfelStat, pause));
    });
}

void Scene::ResumeSurfels() {
    device->Execute([&](CommandBuffer* commandBuffer) {
        uint32_t pause = 0;
        commandBuffer->CopyDataToBuffer(pause, surfelStatBuffer, offsetof(SurfelStat, pause));
    });
}
