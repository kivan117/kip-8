#pragma once
#include "UIState.h"
class ParentUI
{

public:
	//ParentUI(UIState* shared_state) : fe_State(shared_state) {}
	virtual void Init() = 0;
	virtual void Deinit() = 0;
	virtual void Draw() = 0;
	virtual void HandleInput(SDL_Event* event) = 0;
	virtual bool WantCaptureKB() = 0;
	virtual bool WantCaptureMouse() = 0;
};

