#pragma once
#include "ParentUI.h"
#pragma warning(push, 0)
#include <SDL.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl.h"
#include "imgui_sdl.h"
#include "portable-file-dialogs.h"
#pragma warning(pop)
#include "Chip8.h"
#include "UIState.h"
class BasicUI : public ParentUI
{
private:

	UIState* fe_State;

	void ShowMenuBar();
	void ShowMenuFile();
	void ShowMenuEmulation();
	void ShowMenuOptions();



public:
	BasicUI(UIState* shared_state) : fe_State(shared_state) {}
	~BasicUI() {}
	void Init() override;
	void Deinit() override;
	void Draw() override;
	void HandleInput(SDL_Event* event) override;
	bool WantCaptureKB() override;
	bool WantCaptureMouse() override;

};
