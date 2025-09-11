#pragma once

#include "Scenes/Scene.h"

class ShapesScene : public Scene
{
public:
    bool Initialize() override;

    void Update() override;
    void Draw() override;

    void OnWindowResize() override;
    void OnMouseDown(WPARAM btnState, int x, int y) override;
    void OnMouseUp(WPARAM btnState, int x, int y) override;
    void OnMouseMove(WPARAM btnState, int x, int y) override;

private:

};