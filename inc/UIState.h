#pragma once
#pragma warning(push, 0)
#include <SDL.h>
#include "portable-file-dialogs.h"
#pragma warning(pop)
#include "Chip8.h"


typedef struct uistate {
	bool running{ true };
	unsigned int resolution_Zoom{ 8 };
	bool zoom_Changed{ false };
	SDL_Window* window{ nullptr };
	SDL_Renderer* renderer{ nullptr };
	SDL_Texture* screen_Texture{ nullptr };
	Chip8* core{ nullptr };
	unsigned int run_Cycles{ 9 };
	SDL_Color screen_Colors[16] = { 0x00, 0x00, 0x00, 0x00 };
	bool debug_Grid_Lines{ true };
	float volume{ 5.0 };
	const unsigned int audio_Output_Size{ 4096 };
	float audio_Output[4096] = { 0.0 };
	int16_t audio_Output_Pos{ 0 };

	bool capture_KB{ false };
	bool capture_Mouse{ false };
	bool grid_Toggled{ false };

	std::vector<unsigned char> file_data;
	std::shared_ptr<pfd::open_file> open_File;
	std::string last_File{ "" };
} UIState;