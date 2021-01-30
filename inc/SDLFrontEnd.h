#pragma once
#include <map>
#include <vector>
#pragma warning(push, 0)
#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_sdl.h"
#include "imgui_memory_editor.h"
#include "Stopwatch.h"
#pragma warning(pop)
#include "ParentUI.h"
#include "BasicUI.h"
#include "DebugUI.h"
#include "Chip8.h"
#include "Logger.h"
#include "UIState.h"

class SDLFrontEnd
{
public:
	bool m_Paused = false; //if true, ignore time lapsed since last frame. do not run emu core. do not play emu sound. do not accept emu input etc
	SDL_AudioSpec audio_spec;
	uint32_t m_Sample_Pos = 0;
	const int SINE_FREQ = 512;
	const int SAMPLE_FREQ = 4096;
	const double SAMPLES_PER_SINE = (double)SAMPLE_FREQ / (double)SINE_FREQ;

	SDLFrontEnd(Chip8* core);
	SDLFrontEnd(Chip8* core, bool debug);
	~SDLFrontEnd() { deinit(); }
	bool Run();
	void SetRunCycles(int cycles) { m_State.run_Cycles = cycles; return; }
	void Load(std::string filename);
	UIState* GetState() { return &m_State; }

private:
	bool debug_interface;
	SDL_Rect m_Pixel;
	unsigned int m_Res_Width;
	unsigned int m_Res_Height;
	uint8_t SuperChipRPLData[8] = { 0 };
	ParentUI* imgui_UI;
	SDL_AudioDeviceID m_Audio_Device;
	stopwatch::Stopwatch m_Timer;
	const double m_FrameMicroSeconds = 1000000.0 / 60.0; //microseconds per frame length
	double time_accumulator = 0.0;
	
	//breaking out input into 2 maps lets us change the user's input keys or the emulated key layout without affecting both
	std::map<uint8_t, uint8_t> keymap_internal; //maps from internal key matrix to current key layout
	std::map<SDL_Scancode, uint8_t> keymap; //maps scancodes for player's kayboard to current key layout

	//the important shared data between this SDL front end and the ImGui menus/windows
	UIState m_State;

private:
	void init();
	int  initVideo();
	int  initAudio();
	void deinit();
	void deinitAudio();
	void deinitVideo();	
	void AdvanceCore();
	void DrawScreen();
	void HandleInput();
	void ResetResolution();
	void ResetDisplayTexture();
	void PersistRPL();
	void SetTitle();
	void SetInternalKeys(UIState::KeyLayout layout);
	void SetMappedKey(SDL_Scancode scancode, uint8_t index);

};

