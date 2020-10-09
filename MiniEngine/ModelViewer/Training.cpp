#include "CameraController.h"
#include "BufferManager.h"
#include "Camera.h"
#include "CommandContext.h"
#include "SystemTime.h"
#include "GameInput.h"
#include "ShadowCamera.h"
#include "DepthOnlyPass.h"

using namespace GameCore;

class Training :public GameCore::IGameApp {
public:
    Training() :m_model(), m_mainCamera(), m_shadowCamera(),
        m_cameraController(nullptr), m_depthOnlyPass(nullptr) {}

    virtual void Startup() override;

    virtual void Cleanup() override;

    virtual void Update(float deltaT) override;

    virtual void RenderScene() override;

private:
    Model m_model;
    Camera m_mainCamera;
    ShadowCamera m_shadowCamera;
    D3D12_VIEWPORT m_mainViewport;
    D3D12_RECT m_mainScissor;
    std::unique_ptr<CameraController> m_cameraController;
    std::unique_ptr<DepthOnlyPass> m_depthOnlyPass;
};

CREATE_APPLICATION(Training)

void Training::Startup() {
    TextureManager::Initialize(L"Textures/");
    ASSERT(m_model.Load("Models/sponza.h3d"), "Failed to load model");
    ASSERT(m_model.m_Header.meshCount > 0, "Model contains no meshes");

    for (uint32_t i = 0; i < m_model.m_Header.materialCount; ++i) {
        if (std::string(m_model.m_pMaterial[i].texDiffusePath).find("thorn") != std::string::npos ||
            std::string(m_model.m_pMaterial[i].texDiffusePath).find("plant") != std::string::npos ||
            std::string(m_model.m_pMaterial[i].texDiffusePath).find("chain") != std::string::npos) {
            m_model.m_pMaterialType[i] = Model::EMaterialType::Cutout;
        } else {
            m_model.m_pMaterialType[i] = Model::EMaterialType::Opaque;
        }
    }

    float modelRadius = Length(m_model.m_Header.boundingBox.max - m_model.m_Header.boundingBox.min) * .5f;
    const Vector3 eye = (m_model.m_Header.boundingBox.min + m_model.m_Header.boundingBox.max) * .5f + Vector3(modelRadius * .5f, 0.0f, 0.0f);
    m_mainCamera.SetEyeAtUp(eye, Vector3(kZero), Vector3(kYUnitVector));
    m_mainCamera.SetZRange(0.1f, 10000.0f);
    m_mainCamera.ReverseZ(true);
    m_cameraController.reset(new CameraController(m_mainCamera, Vector3(kYUnitVector)));

    m_depthOnlyPass.reset(new DepthOnlyPass());
    m_depthOnlyPass->Setup();
    m_depthOnlyPass->PushModelToDraw(&m_model); // TODO : remove
}

void Training::Cleanup() {
    m_cameraController.reset();
    m_model.Clear();
}

void Training::Update(float deltaT) {
    m_cameraController->Update(deltaT);

    m_mainViewport.TopLeftX = 0;
    m_mainViewport.TopLeftY = 0;
    m_mainViewport.Width = (float)Graphics::g_SceneColorBuffer.GetWidth();
    m_mainViewport.Height = (float)Graphics::g_SceneColorBuffer.GetHeight();
    m_mainViewport.MinDepth = 0.0f;
    m_mainViewport.MaxDepth = 1.0f;

    m_mainScissor.left = 0;
    m_mainScissor.top = 0;
    m_mainScissor.right = (LONG)Graphics::g_SceneColorBuffer.GetWidth();
    m_mainScissor.bottom = (LONG)Graphics::g_SceneColorBuffer.GetHeight();
}

void Training::RenderScene() {
    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");
    gfxContext.TransitionResource(Graphics::g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
    gfxContext.TransitionResource(Graphics::g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    gfxContext.FlushResourceBarriers();

    gfxContext.ClearColor(Graphics::g_SceneColorBuffer);
    gfxContext.ClearDepthAndStencil(Graphics::g_SceneDepthBuffer);
    gfxContext.SetViewportAndScissor(m_mainViewport, m_mainScissor);
    m_depthOnlyPass->Execute(m_mainCamera, &gfxContext);

    gfxContext.Finish();
}