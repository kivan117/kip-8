#pragma once
#pragma warning(push, 0)
#include <SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_sdl.h"
#include "imgui_memory_editor.h"
#include "Stopwatch.h"
#pragma warning(pop)
#include "DebugUI.h"
#include "Chip8.h"
#include "Logger.h"
class SDLFrontEnd
{
private:
	bool debug_interface;
	bool m_Running; //whether to keep running the program or exit completely

	bool debug_grid_lines = true;
	unsigned int m_Resolution_Zoom; //int resolution multiplier for the screen output
	unsigned int m_Debug_Cycles; //how many cycles to advance CPU per frame when in debug mode. Default 1.
	unsigned int m_Run_Cycles; //how many cycles to advance CPU per frame in normal run mode. Default 9.
	SDL_Window* m_Window;
	SDL_Surface* m_Screen_Surface;
	SDL_Renderer* m_Renderer;
	SDL_Rect windowRect;
	SDL_Rect m_Pixel;
	SDL_Texture* m_Screen_Texture;
	SDL_Color ScreenColors[16] = { 0x00, 0x00, 0x00, 0x00 };
	//SDL_Color fg_color{ 0x00, 0x80, 0x80, 0xFF };
	//SDL_Color bg_color{ 0x00, 0x10, 0x10, 0xFF };
	unsigned int m_Res_Width;
	unsigned int m_Res_Height;
	
	uint8_t SuperChipRPLData[8] = { 0 };
	float deflicker = 0.1f;

	DebugUI* debugUI;

	SDL_AudioDeviceID m_Audio_Device;
	


	stopwatch::Stopwatch m_Timer;

	const double m_FrameMicroSeconds = 1000000.0 / 60.0; //microseconds per frame length
	double time_accumulator = 0.0;
public:
	SDLFrontEnd(Chip8* core) : kip8(core), debug_interface(false), debugUI(NULL) { init(); }
	SDLFrontEnd(Chip8* core, bool debug) : kip8(core), debug_interface(debug), debugUI(NULL) { init(); }
	~SDLFrontEnd() { deinit(); }
	bool Run();
	void SetRunCycles(int cycles) { m_Run_Cycles = cycles; return; }
	Chip8* kip8;
	SDL_AudioSpec audio_spec;
	uint32_t m_Sample_Pos = 0;
	float m_Volume = 5.0; // 0 - 10
	const int SINE_FREQ = 512;
	const int SAMPLE_FREQ = 4096;
	const double SAMPLES_PER_SINE = (double)SAMPLE_FREQ / (double)SINE_FREQ;
	bool m_Paused = false; //if true, ignore time lapsed since last frame. do not run emu core
private:
	void init();
	int  initVideo();
	//void audio_callback(void* user, Uint8* stream, int len);
	int  initAudio();
	void deinit();
	void deinitAudio();
	void deinitVideo();	
	void AdvanceCore();
	//void PlaySound();
	void DrawScreen();
	void HandleInput();
	void ResetResolution();
	void ResetDisplayTexture();
	void PersistRPL();
};

