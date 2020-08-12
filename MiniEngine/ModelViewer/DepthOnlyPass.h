#pragma once

#include "GameCore.h"
#include "GraphicsCore.h"
#include "CommandContext.h"
#include "Camera.h"
#include "Model.h"

class DepthOnlyPass {
public:
    DepthOnlyPass();

    void Setup();

    void Execute(const Math::Camera& camera, GraphicsContext* pContext);

    void Dispose();

    // TODO : Get geometry from frustum query
    void PushModelToDraw(Model* model) { m_modelList.push_back(model); }

protected:
    void DrawObjects(GraphicsContext* pContext, Model::EMaterialType filter);
    void DrawObject(GraphicsContext* pContext, const Model* pModel, Model::EMaterialType filter);

private:
    RootSignature m_depthOnlyRS;
    GraphicsPSO m_depthOnlyPSO;
    GraphicsPSO m_depthOnlyCutoutPSO;

    std::vector<Model*> m_modelList;
};