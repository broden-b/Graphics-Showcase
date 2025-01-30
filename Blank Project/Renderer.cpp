#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include "../nclgl/SceneNode.h"
#include <algorithm>
#include <random>

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
    quad = Mesh::GenerateQuad();
    heightMap = new HeightMap(TEXTUREDIR"Flat-Top Hills-2.png");
    isAlternateScene = true;
    isSplitScreen = false;
    manualCameraControl = false;

    LoadSceneTextures(false);
    LoadSceneTextures(true);

    glGenTextures(1, &sceneTexture);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glGenTextures(1, &sceneDepthTexture);
    glBindTexture(GL_TEXTURE_2D, sceneDepthTexture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sceneDepthTexture, 0);

    reflectShader = new Shader("reflectVertex.glsl", "reflectFragment.glsl");
    skyboxShader = new Shader("skyboxVertex.glsl", "skyboxFragment.glsl");
    lightShader = new Shader("PerPixelVertex.glsl", "PerPixelFragment.glsl");
    sceneShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");
    fadeShader = new Shader("fadeVertex.glsl", "fadeFragment.glsl");

    if (!reflectShader->LoadSuccess() || !skyboxShader->LoadSuccess() ||
        !lightShader->LoadSuccess() || !sceneShader->LoadSuccess() || 
        glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || 
        !fadeShader->LoadSuccess()) {
        return;
    }

    Vector3 heightmapSize = heightMap->GetHeightmapSize();
    camera = new Camera(-18.6267f, 39.5926f, heightmapSize * Vector3(0.693887f, 7.2533f, 0.753788f));
    staticCamera = new Camera(-15.0567f, 228.802f, heightmapSize* Vector3(0.250626f, 5.18471f, 0.211205f));
    cameraPath = new CameraPath();

    light = new Light(heightmapSize * Vector3(0.5f, 30, 0.5f), Vector4(0.8f, 0.8f, 0.8f, 1), heightmapSize.x);

    projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

    root = new SceneNode();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    waterRotate = 0.0f;
    waterCycle = 0.0f;
    transitionState = NONE;
    fadeAmount = 0.0f;

    cameraPath->AddKeyframe(heightmapSize * Vector3(0.693887f, 7.2533f, 0.753788f), -18.6267f, 39.5926f, 10.0f);
    cameraPath->AddKeyframe(heightmapSize * Vector3(0.547838f, 1.28546f, 0.675669f), -25.1367f, 77.5324f, 10.0f);
    cameraPath->AddKeyframe(heightmapSize * Vector3(0.560852f, 1.28546f, 0.616811f), -25.0667, 77.5324, 10.0f);
    cameraPath->AddKeyframe(heightmapSize * Vector3(0.641601f, 7.98265f, 0.646653f), -20.3768, 45.6126, 10.0f);

    cameraPath->AddKeyframe(heightmapSize * Vector3(0.537067f, 2.35752f, 0.571909f), -28.4967, 0.0426967f, 10.0f);
    cameraPath->AddKeyframe(heightmapSize * Vector3(0.560429f, 1.27249f, 0.479349f), -32.4867, 348.842, 10.0f);
    cameraPath->AddKeyframe(heightmapSize * Vector3(0.582473f, 1.27249f, 0.483697f), -32.4867f, 348.842f, 10.0f);
    cameraPath->AddKeyframe(heightmapSize * Vector3(0.64729f, 5.18471f, 0.521595f), -13.4467f, 57.2318f, 10.0f);

    cameraPath->SetKeyframeCallback(3, [this]() {
        cameraPath->Pause();
        ToggleScene();
        });

    cameraPath->SetKeyframeCallback(7, [this]() {
        isSplitScreen = true;
        });

    cameraPath->Play();

    init = true;
}

Renderer::~Renderer(void) {
    delete root;
    delete camera;
    delete heightMap;
    delete quad;
    delete reflectShader;
    delete skyboxShader;
    delete lightShader;
    delete sceneShader;
    delete light;
    delete fadeShader;
    delete staticCamera;
    delete cameraPath;
    glDeleteTextures(1, &sceneTexture);
    glDeleteTextures(1, &sceneDepthTexture);
    glDeleteFramebuffers(1, &sceneFBO);
}

void Renderer::LoadSceneTextures(bool alternate) {
    if (!alternate) {
        waterTex = SOIL_load_OGL_texture(
            TEXTUREDIR"water.TGA",
            SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS
        );
        earthTex = SOIL_load_OGL_texture(
            TEXTUREDIR"flat_grass.png",
            SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS
        );
        earthBump = SOIL_load_OGL_texture(
            TEXTUREDIR"grass_bump.jpg",
            SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS
        );
        cubeMap = SOIL_load_OGL_cubemap(
            TEXTUREDIR"meadow_west.jpg", TEXTUREDIR"meadow_east.jpg",
            TEXTUREDIR"meadow_up.jpg", TEXTUREDIR"meadow_down.jpg",
            TEXTUREDIR"meadow_south.jpg", TEXTUREDIR"meadow_north.jpg",
            SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0
        );

        SetTextureRepeating(earthTex, true);
        SetTextureRepeating(earthBump, true);
        SetTextureRepeating(waterTex, true);
    }
    else {
        altWaterTex = SOIL_load_OGL_texture(
            TEXTUREDIR"lava.png",
            SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS
        );
        altEarthTex = SOIL_load_OGL_texture(
            TEXTUREDIR"cracked_earth.JPG",
            SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS
        );
        altEarthBump = SOIL_load_OGL_texture(
            TEXTUREDIR"Barren RedsDOT3.JPG",
            SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS
        );
        altCubeMap = SOIL_load_OGL_cubemap(
            TEXTUREDIR"red_west.bmp", TEXTUREDIR"red_east.bmp",
            TEXTUREDIR"red_up.bmp", TEXTUREDIR"red_down.bmp",
            TEXTUREDIR"red_south.bmp", TEXTUREDIR"red_north.bmp",
            SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0
        );

        SetTextureRepeating(altEarthTex, true);
        SetTextureRepeating(altEarthBump, true);
        SetTextureRepeating(altWaterTex, true);
    }
}

void Renderer::ToggleScene() {
    if (transitionState == NONE) {
        transitionState = FADE_OUT;
    }
}

void Renderer::UpdateScene(float dt) {

    if (manualCameraControl) {
        camera->UpdateCamera(dt);
        cameraPath->Clear();
    }
    if (cameraPath->IsPlaying()) {
        cameraPath->Update(dt, camera);
    }
    else if (cameraPath->IsPaused() && transitionState == NONE) {
        cameraPath->Resume();
    }
    else if (cameraPath->IsComplete()) {
        camera->UpdateCamera(dt);
    }

    viewMatrix = camera->BuildViewMatrix();
    waterRotate += dt * 2.0f;
    waterCycle += dt * 0.1f;
    frameFrustrum.FromMatrix(projMatrix * viewMatrix);

    printTimer += dt;
    if (printTimer >= 5.0f) {
        std::cout << "Camera Position: (" << camera->GetPosition().x / heightMap->GetHeightmapSize().x << ", "
            << camera->GetPosition().y / heightMap->GetHeightmapSize().y << ", " << camera->GetPosition().z / heightMap->GetHeightmapSize().z
            << ") Pitch: " << camera->GetPitch() << " Yaw: " << camera->GetYaw() << std::endl;
        printTimer = 0.0f;
    }

    root->Update(dt);

    ProcessSceneTransition(dt);
}

void Renderer::BuildNodeLists(SceneNode* from) {
    if (frameFrustrum.InsideFrustrum(*from)) {
        return;
    }
    if (from->GetColour().w < 1.0f) {
        transparentNodeList.push_back(from);
    }
    else {
        nodeList.push_back(from);
    }

    for (vector<SceneNode*>::const_iterator i = from->GetChildIteratorStart();
        i != from->GetChildIteratorEnd(); ++i) {
        BuildNodeLists((*i));
    }
}

void Renderer::SortNodeLists() {
    std::sort(transparentNodeList.begin(),
        transparentNodeList.end(),
        SceneNode::CompareByCameraDistance);
    std::sort(nodeList.begin(),
        nodeList.end(),
        SceneNode::CompareByCameraDistance);
}

void Renderer::DrawNodes() {
    for (const auto& i : nodeList) {
        DrawNode(i);
    }
    for (const auto& i : transparentNodeList) {
        DrawNode(i);
    }
}

void Renderer::DrawNode(SceneNode* n) {
    if (frameFrustrum.InsideFrustrum(*n)) {
        return;
    }
    if (n->GetMesh()) {
        BindShader(sceneShader);
        UpdateShaderMatrices();

        glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, n->GetTexture());

        glUniform4fv(glGetUniformLocation(sceneShader->GetProgram(), "nodeColour"),
            1, (float*)&n->GetColour());

        modelMatrix = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
        UpdateShaderMatrices();

        n->Draw(*this);
    }
}

void Renderer::RenderScene() {
    if (isSplitScreen) {
        RenderSplitScreen();
    }
    else {
        frameFrustrum.FromMatrix(projMatrix * viewMatrix);
        BuildNodeLists(root);
        SortNodeLists();

        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        viewMatrix = camera->BuildViewMatrix();

        DrawSkybox();
        DrawHeightmap();
        DrawNodes();
        DrawWater(camera);

        ClearNodeLists();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        RenderFadeQuad();
    }
}

void Renderer::RenderFadeQuad() {
    BindShader(fadeShader);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneTexture);
    glUniform1i(glGetUniformLocation(fadeShader->GetProgram(), "sceneTex"), 0);
    glUniform1f(glGetUniformLocation(fadeShader->GetProgram(), "fadeAmount"), fadeAmount);

    quad->Draw();
}

void Renderer::ProcessSceneTransition(float dt) {
    switch (transitionState) {
    case FADE_OUT:
        fadeAmount += dt / FADE_SPEED;
        if (fadeAmount >= 1.0f) {
            fadeAmount = 1.0f;
            transitionState = SWITCHING;
        }
        break;

    case SWITCHING:
        isAlternateScene = !isAlternateScene;
        transitionState = FADE_IN;
        break;

    case FADE_IN:
        fadeAmount -= dt / FADE_SPEED;
        if (fadeAmount <= 0.0f) {
            fadeAmount = 0.0f;
            transitionState = NONE;
        }
        break;

    default:
        break;
    }
}

void Renderer::ClearNodeLists() {
    transparentNodeList.clear();
    nodeList.clear();
}

void Renderer::AddNode(SceneNode* n) {
    root->AddChild(n);
}

void Renderer::RemoveNode(SceneNode* n) {
    for (auto i = nodeList.begin(); i != nodeList.end(); ++i) {
        if ((*i) == n) {
            nodeList.erase(i);
            break;
        }
    }
    for (auto i = transparentNodeList.begin(); i != transparentNodeList.end(); ++i) {
        if ((*i) == n) {
            transparentNodeList.erase(i);
            break;
        }
    }
}

void Renderer::DrawSkybox() {
    glDepthMask(GL_FALSE);
    BindShader(skyboxShader);
    UpdateShaderMatrices();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, isAlternateScene ? altCubeMap : cubeMap);

    quad->Draw();
    glDepthMask(GL_TRUE);
}

void Renderer::DrawHeightmap() {
    BindShader(lightShader);
    SetShaderLight(*light);
    glUniform3fv(glGetUniformLocation(lightShader->GetProgram(),
        "cameraPos"), 1,
        (float*)&camera->GetPosition());

    glUniform1i(glGetUniformLocation(lightShader->GetProgram(),
        "diffuseTex"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, isAlternateScene ? altEarthTex : earthTex);

    glUniform1i(glGetUniformLocation(lightShader->GetProgram(),
        "bumpTex"), 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, isAlternateScene ? altEarthBump : earthBump);

    modelMatrix.ToIdentity();
    textureMatrix.ToIdentity();

    UpdateShaderMatrices();

    heightMap->Draw();
}

void Renderer::DrawWater(Camera* currentCamera) {
    BindShader(isAlternateScene ? lightShader : reflectShader);
    SetShaderLight(*light);

    // Always use the passed camera's position for reflections
    Vector3 cameraPos = currentCamera->GetPosition();

    if (isAlternateScene) {
        glUniform3fv(glGetUniformLocation(lightShader->GetProgram(),
            "cameraPos"), 1, (float*)&cameraPos);

        glUniform1i(glGetUniformLocation(lightShader->GetProgram(),
            "diffuseTex"), 0);
        glUniform1i(glGetUniformLocation(lightShader->GetProgram(),
            "cubeTex"), 2);
    }
    else {
        glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(),
            "cameraPos"), 1, (float*)&cameraPos);

        glUniform1i(glGetUniformLocation(reflectShader->GetProgram(),
            "diffuseTex"), 0);
        glUniform1i(glGetUniformLocation(reflectShader->GetProgram(),
            "cubeTex"), 2);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, isAlternateScene ? altWaterTex : waterTex);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_CUBE_MAP, isAlternateScene ? altCubeMap : cubeMap);

    Vector3 hSize = heightMap->GetHeightmapSize();

    modelMatrix = Matrix4::Translation(hSize * 0.5f) *
        Matrix4::Scale(hSize * 0.5f) *
        Matrix4::Rotation(90, Vector3(1, 0, 0));

    textureMatrix = Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) *
        Matrix4::Scale(Vector3(10, 10, 10)) *
        Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));

    UpdateShaderMatrices();
    quad->Draw();
}

void Renderer::RenderSplitScreen() {
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, width, height);

    viewMatrix = camera->BuildViewMatrix();
    frameFrustrum.FromMatrix(projMatrix * viewMatrix);
    BuildNodeLists(root);
    SortNodeLists();

    DrawSkybox();
    DrawHeightmap();
    DrawNodes();
    DrawWater(camera);  

    ClearNodeLists();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    
    glViewport(0, 0, width / 2, height);
    RenderFadeQuad();

    
    glViewport(width / 2, 0, width / 2, height);
    viewMatrix = staticCamera->BuildViewMatrix();
    frameFrustrum.FromMatrix(projMatrix * viewMatrix);
    BuildNodeLists(root);
    SortNodeLists();

    DrawSkybox();
    DrawHeightmap();
    DrawNodes();
    DrawWater(staticCamera); 

    ClearNodeLists();

    glViewport(0, 0, width, height);
}

void Renderer::ToggleManualCamera() {
    manualCameraControl = true;
    if (cameraPath) {
        cameraPath->Pause();
    }
}