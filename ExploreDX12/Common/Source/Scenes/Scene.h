#pragma once

#include "Graphics/DirectX12.h"
#include "Managers/WindowManager.h"

#include <memory>

class Scene
{
public:
    virtual ~Scene() = default;

    virtual bool Initialize() = 0;

    virtual void Update() {}
    virtual void Draw() {}
    
    virtual void OnWindowResize() {}
    virtual void OnMouseDown(WPARAM btnState, int x, int y) {}
    virtual void OnMouseUp(WPARAM btnState, int x, int y) {}
    virtual void OnMouseMove(WPARAM btnState, int x, int y) {}

    using UP = std::unique_ptr<Scene>;
};