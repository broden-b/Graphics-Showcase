#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Frustrum.h"
#include "../nclgl/CameraPath.h"

class Camera;
class Shader;
class HeightMap;
class SceneNode;

class Renderer : public OGLRenderer {
public:

    enum TransitionState {
        NONE,
        FADE_OUT,
        SWITCHING,
        FADE_IN
    };

    Renderer(Window& parent);
    ~Renderer(void);
    void RenderScene() override;
    void UpdateScene(float dt) override;
    void ToggleScene();
    void ToggleSplitScreen() { isSplitScreen = !isSplitScreen; }
    void ToggleManualCamera();

protected:
    void AddNode(SceneNode* n);
    void RemoveNode(SceneNode* n);
    void BuildNodeLists(SceneNode* from);
    void SortNodeLists();
    void ClearNodeLists();
    void DrawNodes();
    void DrawNode(SceneNode* n);
    void DrawHeightmap();
    void DrawWater(Camera* currentCamera);    
    void DrawSkybox();
    void LoadSceneTextures(bool isAlternateScene);
    void RenderFadeQuad();
    void ProcessSceneTransition(float dt);
    void RenderSplitScreen();

    SceneNode* root;
    vector<SceneNode*> transparentNodeList;
    vector<SceneNode*> nodeList;

    Shader* fadeShader;
    Shader* lightShader;
    Shader* reflectShader;
    Shader* skyboxShader;
    Shader* sceneShader;

    HeightMap* heightMap;
    Mesh* quad;

    Light* light;
    Camera* camera;
    Camera* staticCamera;
    CameraPath* cameraPath;

    Frustrum frameFrustrum;

    GLuint cubeMap;
    GLuint waterTex;
    GLuint earthTex;
    GLuint earthBump;

    GLuint sceneTexture;
    GLuint sceneDepthTexture;
    GLuint sceneFBO;

    GLuint altCubeMap;
    GLuint altWaterTex;
    GLuint altEarthTex;
    GLuint altEarthBump;

    TransitionState transitionState;

    bool manualCameraControl;
    bool isRenderingStaticCamera;
    bool isAlternateScene;
    bool isSplitScreen;
    bool isInitialCameraPath;
    float printTimer = 0.0f;
    float waterRotate;
    float waterCycle;
    float fadeAmount;
    static constexpr float FADE_SPEED = 1.5f;
};