#pragma once
#include "ParentUI.h"
#pragma warning(push, 0)
#include <SDL.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl.h"
#include "imgui_sdl.h"
#include "imgui_memory_editor.h"
#include "imgui_plot.h"
//#include "imterm\terminal.hpp"
//#include "ImTerm_Commands.h"
#include "portable-file-dialogs.h"
#pragma warning(pop)
#include "Logger.h"
#include "Chip8.h"
#include "UIState.h"
class DebugUI : public ParentUI
{
private:

	UIState* fe_State;

	bool show_regs;
	bool show_ram;
	bool show_vram;
	bool show_display;
	bool show_menu_bar;
	bool show_stack;
	bool show_log;
	bool show_audio;
	//custom_command_struct cmd_struct;
	//ImTerm::terminal<ImTerm_Commands> *terminal_log;
	MemoryEditor chip8_vram_editor, chip8_ram_editor;

	void ShowCPUEditor(bool* p_open);
	void ShowStackWindow(bool* p_open);
	void ShowDisplayWindow(bool* p_open);
	void ShowRAMWindow(bool* p_open);
	void ShowVRAMWindow(bool* p_open);
	void ShowAudioWindow(bool* p_open);
	void ShowMenuBar();
	void ShowMenuFile();
	void ShowMenuEmulation();
	void ShowMenuWindows();
	void ShowMenuOptions();
public:
	//DebugUI(UIState* shared_state) : fe_State(shared_state), show_regs(true), show_display(true), show_ram(false),
	//	show_vram(false), show_menu_bar(true), show_stack(false), show_log(false), show_audio(false), terminal_log(nullptr) {}
	DebugUI(UIState* shared_state) : fe_State(shared_state), show_regs(true), show_display(true), show_ram(false),
		show_vram(false), show_menu_bar(true), show_stack(false), show_log(false), show_audio(false) {}
	void Init() override;
	void Deinit() override;
	void Draw() override;
	void HandleInput(SDL_Event* event) override;
	bool WantCaptureKB() override;
	bool WantCaptureMouse() override;
};

