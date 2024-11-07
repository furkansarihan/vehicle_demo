#include "enigine.h"
#include "ui/vehicle/vehicle_ui.h"

btBvhTriangleMeshShape *getTriangleMeshShape(Mesh &mesh)
{
    btTriangleMesh *triangleMesh = new btTriangleMesh();

    std::vector<Vertex> &vertices = mesh.vertices;
    std::vector<unsigned int> &indices = mesh.indices;

    // Add triangles to the btTriangleMesh
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        // Get vertices for the current triangle
        glm::vec3 vertex0 = vertices[indices[i]].position;
        glm::vec3 vertex1 = vertices[indices[i + 1]].position;
        glm::vec3 vertex2 = vertices[indices[i + 2]].position;

        // Add the triangle to the btTriangleMesh
        triangleMesh->addTriangle(
            btVector3(vertex0.x, vertex0.y, vertex0.z),
            btVector3(vertex1.x, vertex1.y, vertex1.z),
            btVector3(vertex2.x, vertex2.y, vertex2.z));
    }

    // Create a btBvhTriangleMeshShape using the btTriangleMesh
    btBvhTriangleMeshShape *triangleMeshShape = new btBvhTriangleMeshShape(
        triangleMesh,
        true,
        true);

    return triangleMeshShape;
}

void updateWheelInfo(Vehicle *vehicle)
{
    for (int i = 0; i < vehicle->m_vehicle->getNumWheels(); i++)
    {
        btWheelInfo &wheel = vehicle->m_vehicle->getWheelInfo(i);
        wheel.m_suspensionStiffness = vehicle->m_suspensionStiffness;
        wheel.m_wheelsDampingRelaxation = vehicle->m_suspensionDamping;
        wheel.m_wheelsDampingCompression = vehicle->m_suspensionCompression;
        wheel.m_frictionSlip = vehicle->m_wheelFriction;
        wheel.m_rollInfluence = vehicle->m_rollInfluence;
        wheel.m_suspensionRestLength1 = vehicle->m_suspensionRestLength;
    }
}

// ---

class Headlights : public Updatable
{
public:
    Headlights(
        ResourceManager *resourceManager,
        Model *carModel,
        std::string redMaterial,
        std::string blueMaterial)
        : m_resourceManager(resourceManager),
          m_carModel(carModel),
          m_redMaterial(redMaterial),
          m_blueMaterial(blueMaterial){};

    ResourceManager *m_resourceManager;
    Model *m_carModel;
    std::string m_redMaterial;
    std::string m_blueMaterial;

    void update(float deltaTime);
};

float calculateBrightness(float time, float frequency = 1.0f, float minBrightness = 0.2f, float maxBrightness = 1.0f)
{
    float amplitude = (maxBrightness - minBrightness) / 2.0f;
    float offset = minBrightness + amplitude;
    return offset + amplitude * std::sin(2.0f * glm::pi<float>() * frequency * time);
}

void updatePoliceHeadlights(float time, float &redBrightness, float &blueBrightness)
{
    // Setting the frequencies for the red and blue lights
    float redFrequency = 1.0f;
    float blueFrequency = 1.0f;

    // Calculate brightness for each color
    redBrightness = calculateBrightness(time, redFrequency);
    blueBrightness = 1.f - redBrightness;
}

void Headlights::update(float deltaTime)
{
    float time = glfwGetTime();
    float redBrightness = 0.0f, blueBrightness = 0.0f;
    updatePoliceHeadlights(time, redBrightness, blueBrightness);

    m_resourceManager->m_materials[m_redMaterial]->emissiveStrength = redBrightness;
    m_resourceManager->m_materials[m_blueMaterial]->emissiveStrength = blueBrightness;
}

// ---

class BrakeLights : public Updatable
{
public:
    BrakeLights(
        ResourceManager *resourceManager,
        Model *carModel,
        Vehicle *vehicle,
        std::string brakeLightMaterial)
        : m_resourceManager(resourceManager),
          m_carModel(carModel),
          m_vehicle(vehicle),
          m_brakeLightMaterial(brakeLightMaterial){};

    ResourceManager *m_resourceManager;
    Model *m_carModel;
    Vehicle *m_vehicle;
    std::string m_brakeLightMaterial;

    void update(float deltaTime);
};

void BrakeLights::update(float deltaTime)
{
    bool braking = m_vehicle->m_controlState.gas < 0.f;

    if (braking)
    {
        m_resourceManager->m_materials[m_brakeLightMaterial]->blendMode = MaterialBlendMode::opaque;
        m_resourceManager->m_materials[m_brakeLightMaterial]->albedo = glm::vec4(1.f);
        m_resourceManager->m_materials[m_brakeLightMaterial]->opacity = 1.0f;
        m_resourceManager->m_materials[m_brakeLightMaterial]->emissiveColor = glm::vec4(5.f, 1.f, 1.f, 1.f);
        m_resourceManager->m_materials[m_brakeLightMaterial]->emissiveStrength = 5.f;
        m_resourceManager->m_materials[m_brakeLightMaterial]->metallic = 1.f;
        m_resourceManager->m_materials[m_brakeLightMaterial]->roughness = 1.f;
    }
    else
    {
        m_resourceManager->m_materials[m_brakeLightMaterial]->blendMode = MaterialBlendMode::alphaBlend;
        m_resourceManager->m_materials[m_brakeLightMaterial]->albedo = glm::vec4(1.f, 1.f, 1.f, 1.f);
        m_resourceManager->m_materials[m_brakeLightMaterial]->metallic = 0.123f;
        m_resourceManager->m_materials[m_brakeLightMaterial]->roughness = 0.099f;
        m_resourceManager->m_materials[m_brakeLightMaterial]->opacity = 0.8f;
        m_resourceManager->m_materials[m_brakeLightMaterial]->emissiveColor = glm::vec4(0.f, 0.f, 0.f, 1.f);
        m_resourceManager->m_materials[m_brakeLightMaterial]->emissiveStrength = 0.f;
    }

    m_carModel->updateMeshTypes();
}

// ---

class KeyListener : public Updatable
{
public:
    KeyListener(GLFWwindow *window,
                RootUI *rootUI)
        : m_window(window),
          m_rootUI(rootUI){};

    GLFWwindow *m_window;
    RootUI *m_rootUI;
    Model *m_carModel;

    void update(float deltaTime);
};

void KeyListener::update(float deltaTime)
{
    if (glfwGetKey(m_window, GLFW_KEY_H) == GLFW_PRESS)
        m_rootUI->m_enabled = false;
    if (glfwGetKey(m_window, GLFW_KEY_J) == GLFW_PRESS)
        m_rootUI->m_enabled = true;
}

// ---

int main()
{
    Enigine enigine;
    enigine.init();

    GLFWwindow *window = enigine.window;
    RenderManager *renderManager = enigine.renderManager;
    ResourceManager *resourceManager = enigine.resourceManager;
    ShaderManager *shaderManager = enigine.shaderManager;
    PhysicsWorld *physicsWorld = enigine.physicsWorld;
    DebugDrawer *debugDrawer = enigine.debugDrawer;
    UpdateManager *updateManager = enigine.updateManager;
    InputManager *inputManager = enigine.inputManager;
    RootUI *rootUI = enigine.rootUI;
    SoundEngine *soundEngine = enigine.soundEngine;
    Camera *mainCamera = enigine.mainCamera;

    std::string filename = "kloofendal_43d_clear_1k.hdr";
    std::string path = resourceManager->m_executablePath + "/assets/hdris/" + filename;
    TextureParams textureParams;
    textureParams.dataType = TextureDataType::Float32;
    textureParams.generateMipmaps = true;
    Texture envTexture = resourceManager->textureFromFile(textureParams, path, path);
    renderManager->updateEnvironmentTexture(envTexture);

    //
    mainCamera->fov = glm::radians(60.f);
    renderManager->m_shadowManager->m_far = 300.f;
    renderManager->m_shadowManager->m_splitWeight = 0.8f;
    //
    renderManager->fogColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);
    renderManager->fogMinDist = 200.f;
    renderManager->fogMaxDist = 400.f;

    //
    KeyListener keyListener(window, rootUI);
    updateManager->add(&keyListener);

    // camera
    mainCamera->position = glm::vec3(5.f, 1.7f, 10.f);
    mainCamera->front = glm::vec3(-0.430f, -0.195f, -0.882f);
    mainCamera->right = glm::normalize(glm::cross(mainCamera->front, mainCamera->worldUp));
    mainCamera->up = glm::normalize(glm::cross(mainCamera->right, mainCamera->front));

    // vehicle
    Models models;

    // car-5
    models.carBody = resourceManager->getModel("assets/car-5/body.glb");
    models.wheelBase = resourceManager->getModel("assets/car-5/wheel-base-1.glb");
    models.doorFront = resourceManager->getModel("assets/car-5/door-front-left.glb");
    models.smokeParticleModel = resourceManager->getModel("assets/car/smoke.obj");
    Model *collider = resourceManager->getModel("assets/car-5/collider.obj");
    float wheelSize[4] = {0.32f, 0.32f, 0.32f, 0.32f};
    glm::vec3 wheelPos[4] = {
        glm::vec3(0.88, 0.8f, 1.5),
        glm::vec3(-0.88, 0.8f, 1.5),
        glm::vec3(0.88, 0.8f, -1.25),
        glm::vec3(-0.88, 0.8f, -1.25)};
    std::string headLightMaterial = "Riverside88_lights.002";
    std::string brakeLightMaterial = "Riverside88_lights.001";
    std::string bodyMaterial = "body";
    std::string glassMaterial = "UCB_GLASS_CLEAN";

    // car-police
    /* models.carBody = resourceManager->getModel("assets/car-6/body.glb");
    models.wheelBase = resourceManager->getModel("assets/car-6/wheel-base-1.glb");
    models.doorFront = resourceManager->getModel("assets/car-6/door-front-left.glb");
    models.smokeParticleModel = resourceManager->getModel("assets/car/smoke.obj");
    Model *collider = resourceManager->getModel("assets/car-5/collider.obj");
    float wheelSize[4] = {0.29f, 0.29f, 0.29f, 0.29f};
    glm::vec3 wheelPos[4] = {
        glm::vec3(0.78, 0.8f, 1.44),
        glm::vec3(-0.78, 0.8f, 1.44),
        glm::vec3(0.76, 0.8f, -1.23),
        glm::vec3(-0.76, 0.8f, -1.23)};
    std::string headLightMaterial = "Riverside88_lights.001";
    std::string brakeLightMaterial = "Riverside88_lights.002";
    std::string redLightMaterial = "Riverside88_lights.003";
    std::string blueLightMaterial = "Riverside88_lights.004";
    std::string glassMaterial = "UCB_GLASS_CLEAN"; */

    //
    glm::vec3 pos = glm::vec3(4.f, 4.f, 4.f);
    Vehicle vehicle(physicsWorld, VehicleType::coupe, collider, eTransform(), pos);
    CarController car(
        window,
        shaderManager,
        renderManager,
        resourceManager,
        inputManager,
        &vehicle,
        mainCamera,
        models,
        2);

    for (int i = 0; i < 4; i++)
    {
        vehicle.setWheelPosition(i, wheelPos[i]);
        vehicle.setWheelRadius(i, wheelSize[i]);
        car.m_linkTireSmokes[i]->m_offset.setPosition(wheelPos[i] - glm::vec3(0.f, 0.7f, 0.f));
    }

    //
    // door offset - when opened
    vehicle.setActiveDoorHingeOffsetFront(glm::vec3(0.8f, 1.17f, 0.8f), glm::vec3(0.6f, 0.0f, 0.5f));
    //
    // TODO: single source
    car.m_linkDoors[0]->m_offsetActive.setPosition(glm::vec3(-0.2f, -0.8f, -0.67f));
    car.m_linkDoors[0]->m_offsetActive.setRotation(glm::quat(0.5f, 0.5f, 0.5f, 0.5f));
    vehicle.m_doors[0].rigidbodyOffset.setPosition(glm::vec3(0.8f, 0.67f, 0.2f));
    vehicle.m_doors[0].rigidbodyOffset.setRotation(glm::quat(0.5f, -0.5f, -0.5f, -0.5f));
    //
    car.m_linkDoors[1]->m_offsetActive.setPosition(glm::vec3(-0.2f, 0.8f, -0.67f));
    car.m_linkDoors[1]->m_offsetActive.setRotation(glm::quat(0.5f, 0.5f, 0.5f, 0.5f));
    vehicle.m_doors[1].rigidbodyOffset.setPosition(glm::vec3(-0.8f, 0.67f, 0.2f));
    vehicle.m_doors[1].rigidbodyOffset.setRotation(glm::quat(0.5f, -0.5f, -0.5f, -0.5f));
    //
    // TODO: cull face front because of mirrored
    car.m_linkDoors[1]->m_offsetActive.setScale(glm::vec3(-1.f, 1.f, 1.f));
    car.m_linkDoors[1]->m_offsetDeactive.setScale(glm::vec3(-1.f, 1.f, 1.f));
    //
    // car.m_linkWheels[0]->m_offset.setScale(glm::vec3(1.f, 1.12f, 1.12f));
    // car.m_linkWheels[1]->m_offset.setScale(glm::vec3(1.f, 1.12f, 1.12f));
    // car.m_linkWheels[2]->m_offset.setScale(glm::vec3(1.f, 1.18f, 1.18f));
    // car.m_linkWheels[3]->m_offset.setScale(glm::vec3(1.f, 1.18f, 1.18f));
    //
    // door collider
    glm::vec3 size(0.6, 0.15, 0.4);
    vehicle.m_doors[0].body->getCollisionShape()->setLocalScaling(BulletGLM::getBulletVec3(size));
    vehicle.m_doors[1].body->getCollisionShape()->setLocalScaling(BulletGLM::getBulletVec3(size));
    //
    // exhaust
    car.m_linkExhausts[0]->m_offset.setPosition(glm::vec3(0.32f, 1.f, -1.02f));
    car.m_linkExhausts[0]->m_offset.setRotation(glm::quat(0.f, 1.f, 0.f, 0.f));
    car.m_linkExhausts[1]->m_offset.setPosition(glm::vec3(-0.32f, 1.f, -1.02f));
    car.m_linkExhausts[1]->m_offset.setRotation(glm::quat(0.f, 1.f, 0.f, 0.f));
    //
    car.m_controlVehicle = true;
    car.m_followVehicle = true;
    car.m_follow.offset = glm::vec3(0.f, 0.5f, 0.f);
    car.m_follow.distance = 6.f;
    car.m_follow.stretchSteeringFactor = 0.f;

    // car-5
    car.m_linkDoors[1]->m_offsetDeactive.setPosition(glm::vec3(0.015f, 0.0f, 0.0f));
    // car-police
    // car.m_linkDoors[1]->m_offsetDeactive.setPosition(glm::vec3(0.005f, 0.0f, 0.0f));

    vehicle.m_suspensionRestLength = 0.5f;
    vehicle.m_wheelFriction = 3.5f;
    vehicle.m_driftFriction = 5.0f;
    vehicle.steeringIncrement = 6.f;
    vehicle.maxEngineForce = 5000.f;
    vehicle.minEngineForce = -4000.f;
    vehicle.accelerationVelocity = 8000.f;
    vehicle.decreaseVelocity = 20000.f;
    updateWheelInfo(&vehicle);

    // modify materials
    {
        // headlight
        resourceManager->m_materials[headLightMaterial]->blendMode = MaterialBlendMode::opaque;
        resourceManager->m_materials[headLightMaterial]->albedo = glm::vec4(1.f);
        resourceManager->m_materials[headLightMaterial]->opacity = 1.0f;
        resourceManager->m_materials[headLightMaterial]->emissiveColor = glm::vec4(2.f, 2.f, 2.f, 1.f);
        resourceManager->m_materials[headLightMaterial]->emissiveStrength = 8.f;
        resourceManager->m_materials[headLightMaterial]->metallic = 1.f;
        resourceManager->m_materials[headLightMaterial]->roughness = 1.f;

        // glass
        resourceManager->m_materials[glassMaterial]->albedo = glm::vec4(0.184f, 0.184f, 0.184f, 0.427f);
        resourceManager->m_materials[glassMaterial]->metallic = 0.f;
        resourceManager->m_materials[glassMaterial]->roughness = 0.f;
        resourceManager->m_materials[glassMaterial]->transmission = 0.f;
        resourceManager->m_materials[glassMaterial]->opacity = 0.427f;

        // police lights
        /* resourceManager->m_materials[redLightMaterial]->blendMode = MaterialBlendMode::opaque;
        resourceManager->m_materials[redLightMaterial]->emissiveColor = glm::vec4(8.f, 0.3f, 0.3f, 1.f);
        resourceManager->m_materials[redLightMaterial]->metallic = 0.f;
        resourceManager->m_materials[redLightMaterial]->roughness = 0.f;
        resourceManager->m_materials[blueLightMaterial]->blendMode = MaterialBlendMode::opaque;
        resourceManager->m_materials[blueLightMaterial]->emissiveColor = glm::vec4(0.3f, 0.3f, 8.f, 1.f);
        resourceManager->m_materials[blueLightMaterial]->metallic = 0.f;
        resourceManager->m_materials[blueLightMaterial]->roughness = 0.f; */

        // body color

        // red-1
        resourceManager->m_materials[bodyMaterial]->albedo = glm::vec4(0.6f, 0.1f, 0.2f, 1.f);
        resourceManager->m_materials[bodyMaterial]->metallic = 0.8f;
        resourceManager->m_materials[bodyMaterial]->roughness = 0.2f;
        // yellow-1
        /* resourceManager->m_materials[bodyMaterial]->albedo = glm::vec4(0.7f, 1.0f, 0.4f, 1.f);
        resourceManager->m_materials[bodyMaterial]->metallic = 0.5f;
        resourceManager->m_materials[bodyMaterial]->roughness = 0.2f; */
        // green-1
        /* resourceManager->m_materials[bodyMaterial]->albedo = glm::vec4(0.3f, 0.4f, 0.3f, 1.f);
        resourceManager->m_materials[bodyMaterial]->metallic = 0.6f;
        resourceManager->m_materials[bodyMaterial]->roughness = 0.2f; */
    }
    models.carBody->updateMeshTypes();

    //
    updateManager->add(&car);

    //
    VehicleUI *vehicleUI = new VehicleUI(&car, &vehicle);
    enigine.rootUI->m_uiList.push_back(vehicleUI);

    // surface
    /* btTransform tr;
    tr.setIdentity();
    // tr.setOrigin(btVector3(0, -0.05f, 0));
    btCollisionShape *surfaceShape = new btStaticPlaneShape(btVector3(0, 1, 0), 0);
    btRigidBody *surfaceBody = physicsWorld->createRigidBody(surfaceShape, 0.f, tr); */

    // scene model
    std::string scenePath = "assets/track-6.glb";
    Model *sceneModel = resourceManager->getModel(scenePath);

    // scene elements per mesh
    for (int i = 0; i < sceneModel->meshes.size(); i++)
    {
        Mesh *mesh = sceneModel->meshes[i];
        Model *meshModel = new Model(sceneModel, i);

        // render source
        RenderSource *renderSource = RenderSourceBuilder()
                                         // .setFaceCullType(FaceCullType::none)
                                         .setModel(meshModel)
                                         .build();
        renderSource->transform.setModelMatrix(mesh->offset);
        renderManager->addSource(renderSource);

        // physics
        btCollisionShape *shape = getTriangleMeshShape(*mesh);
        btTransform localTransform;
        localTransform.setFromOpenGLMatrix((float *)&mesh->offset);
        btRigidBody *meshBody = physicsWorld->createRigidBody(shape, 0.f, localTransform);
    }

    //
    BrakeLights brakeLights(
        resourceManager,
        models.carBody,
        &vehicle,
        brakeLightMaterial);
    updateManager->add(&brakeLights);

    /* Headlights headlights(
        resourceManager,
        models.carBody,
        redLightMaterial,
        blueLightMaterial);
    updateManager->add(&headlights); */

    // controller support
    std::string mappings = FileManager::read("assets/gamecontrollerdb.txt");
    glfwUpdateGamepadMappings(mappings.c_str());

    enigine.start();

    // cleanup
    delete vehicleUI;
}
