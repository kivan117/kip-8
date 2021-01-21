#pragma once
#pragma warning(push, 0)
#include <SDL.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl.h"
#include "imgui_sdl.h"
#include "imgui_memory_editor.h"
#include "imterm\terminal.hpp"
#include "ImTerm_Commands.h"
#include "portable-file-dialogs.h"
#pragma warning(pop)
#include "Logger.h"
#include "Chip8.h"
class DebugUI
{
private:
	
	bool* keep_running;	
	bool captureKB;
	bool captureMouse;
	bool show_regs;
	bool show_ram;
	bool show_vram;
	bool show_display;
	bool show_menu_bar;
	bool show_stack;
	bool show_log;
	bool grid_toggled = false;
	unsigned int m_Zoom;

	std::shared_ptr<pfd::open_file> open_file;
	std::string last_file;

	custom_command_struct cmd_struct;
	ImTerm::terminal<ImTerm_Commands> terminal_log;
	MemoryEditor chip8_vram_editor, chip8_ram_editor;

	void ShowRegisterObject(const char* prefix, int uid, Chip8* core);
	void ShowCPUEditor(bool* p_open, Chip8* core);
	void ShowStackWindow(bool* p_open, Chip8* core);
	void ShowDisplayWindow(bool* p_open, SDL_Texture* tex, unsigned int zoom, unsigned int res_w, unsigned int res_h);
	void ShowRAMWindow(bool* p_open, Chip8* core, unsigned int res_w, unsigned int res_h);
	void ShowVRAMWindow(bool* p_open, Chip8* core, unsigned int res_w, unsigned int res_h);
	void ShowMenuBar(Chip8* core, unsigned int* speed, unsigned int* zoom, SDL_Color (&colors)[16], bool* debug_grid_lines, float* volume);
	void ShowMenuFile(Chip8* core);
	void ShowMenuEmulation(Chip8* core, unsigned int* speed);
	void ShowMenuWindows();
	void ShowMenuOptions(Chip8* core, unsigned int* zoom, SDL_Color (&colors)[16], bool* debug_grid_lines, float* volume);


public:
	DebugUI(bool* run, unsigned int* zoom)
		: keep_running(run), m_Zoom(*zoom), captureKB(false), captureMouse(false), show_regs(true), show_display(true),
		  show_ram(false), show_vram(false), show_menu_bar(true), show_stack(false), show_log(false),
		  open_file(nullptr), last_file(""), terminal_log(cmd_struct)
	{	
	}
	void Init(SDL_Window* window, SDL_Renderer* renderer);
	void Deinit();
	void Draw(SDL_Window* window, SDL_Renderer* renderer, SDL_Texture* tex, Chip8* core, unsigned int res_w, unsigned int res_h, unsigned int* speed, SDL_Color (&colors)[16], bool* debug_grid_lines, float* volume);
	void HandleInput(SDL_Event* event);
	bool WantCaptureKB();
	bool WantCaptureMouse();
	unsigned int GetZoom() { return m_Zoom; }
	bool GetGridToggled() { return grid_toggled; }
	void ResetGridToggled() { grid_toggled = false; }
	std::string GetLastFilename() { return last_file; }
};

