#include "SDLFrontEnd.h"
#include <cmath>
#include <iostream>
#include <fstream>

SDLFrontEnd::SDLFrontEnd(Chip8* core)
{
	m_State.core = core;
	debug_interface = false;
	init();
}
SDLFrontEnd::SDLFrontEnd(Chip8* core, bool debug)
{
	m_State.core = core;
	debug_interface = debug;
	init();
}

void SDLFrontEnd::init()
{
    m_State.running = true;
    m_State.resolution_Zoom = 8;
	if (debug_interface)
	{
		imgui_UI = new DebugUI(&m_State);
	}
	else
	{
		imgui_UI = new BasicUI(&m_State);
	}
	m_Res_Width = m_State.core->res.base_width;
	m_Res_Height = m_State.core->res.base_height;
    m_Pixel.w = m_State.resolution_Zoom;
    m_Pixel.h = m_State.resolution_Zoom;
    m_State.run_Cycles = 9;
    m_State.window = NULL;
	m_State.volume = 5.0; // 0 - 10
	//setup default keymap. should make this configurable soon
	keymap.insert_or_assign(SDL_SCANCODE_X, 0x0);
	keymap.insert_or_assign(SDL_SCANCODE_1, 0x1);
	keymap.insert_or_assign(SDL_SCANCODE_2, 0x2);
	keymap.insert_or_assign(SDL_SCANCODE_3, 0x3);
	keymap.insert_or_assign(SDL_SCANCODE_Q, 0x4);
	keymap.insert_or_assign(SDL_SCANCODE_W, 0x5);
	keymap.insert_or_assign(SDL_SCANCODE_E, 0x6);
	keymap.insert_or_assign(SDL_SCANCODE_A, 0x7);
	keymap.insert_or_assign(SDL_SCANCODE_S, 0x8);
	keymap.insert_or_assign(SDL_SCANCODE_D, 0x9);
	keymap.insert_or_assign(SDL_SCANCODE_Z, 0xA);
	keymap.insert_or_assign(SDL_SCANCODE_C, 0xB);
	keymap.insert_or_assign(SDL_SCANCODE_4, 0xC);
	keymap.insert_or_assign(SDL_SCANCODE_R, 0xD);
	keymap.insert_or_assign(SDL_SCANCODE_F, 0xE);
	keymap.insert_or_assign(SDL_SCANCODE_V, 0xF);

    initVideo();
    initAudio();
}

int SDLFrontEnd::initVideo()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        m_State.running = false;
        return -1;
    }

	if(debug_interface)
		m_State.window = SDL_CreateWindow("KIP-8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
	else
		m_State.window = SDL_CreateWindow("KIP-8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, (m_State.core->res.base_width * m_State.resolution_Zoom) + m_State.core->res.base_width + 1, (m_State.core->res.base_height * m_State.resolution_Zoom) + m_State.core->res.base_height + 1, SDL_WINDOW_SHOWN);
	if (m_State.window == NULL)
    {
        std::cout << "Window could not be created! SDL_Error: " <<  SDL_GetError() << std::endl;
        m_State.running = false;
        return -1;
    }
    else
    {
		m_State.screen_Colors[0].r = 0x88;//default background color
		m_State.screen_Colors[0].g = 0x55;
		m_State.screen_Colors[0].b = 0x00;
		m_State.screen_Colors[0].a = 0xFF;

		m_State.screen_Colors[1].r = 0xEE;//default foreground color
		m_State.screen_Colors[1].g = 0xBB;
		m_State.screen_Colors[1].b = 0x00;
		m_State.screen_Colors[1].a = 0xFF;

		m_State.screen_Colors[2].r = 0xEE;//default plane 2 color
		m_State.screen_Colors[2].g = 0x55;
		m_State.screen_Colors[2].b = 0x00;
		m_State.screen_Colors[2].a = 0xFF;

		m_State.screen_Colors[3].r = 0x55;//default plane1 + plane 2 overlap color
		m_State.screen_Colors[3].g = 0x11;
		m_State.screen_Colors[3].b = 0x00;
		m_State.screen_Colors[3].a = 0xFF;

		//make a hardware accel renderer for future screen drawing
		m_State.renderer = SDL_CreateRenderer(m_State.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE);

		if (imgui_UI)
			imgui_UI->Init();	

		ResetDisplayTexture();
    }
    return 0;
}

void audio_callback(void* user, Uint8* stream, int len) {

	SDLFrontEnd* frontend = (SDLFrontEnd * )user;
	int16_t* audio_stream = (int16_t*)stream;
	int audio_len = len / 2;
	for (int it = 0; it < audio_len; it++)
	{
		if (frontend->GetState()->core->GetSoundTimer() && !frontend->m_Paused) // ST > 0, play sound 
		{
			if (frontend->GetState()->core->GetSystemMode() == Chip8::SYSTEM_MODE::XO_CHIP) //Play XO-Chip audio. Square wave read from pattern buffer
			{
				if (frontend->GetState()->core->audio_pattern[(it / 8) % 16] >> (7 - (it % 8)) & 0x01) //read through the audio pattern buffer, check each bit as a separate sample
				{
					audio_stream[it] = (Sint16)(3276.7 * (double)frontend->GetState()->volume); //each sample is just a 1 or 0, super basic square wave, so we just multiply it by the user configurable volume (output at volume or output silence)
				}
				else
					audio_stream[it] = (Sint16)(-3276.7 * (double)frontend->GetState()->volume);
			}
			else //Not XO-Chip mode, just play a 512hz sine wave tone
			{
				double temp = (SDL_sin(((double)(frontend->m_Sample_Pos++) / frontend->SAMPLES_PER_SINE) * (double)M_PI * 2.0) * (3276.7 * (double)frontend->GetState()->volume));
				audio_stream[it] = (int16_t) std::round(temp);
				
			}
		}
		else
			audio_stream[it] = frontend->audio_spec.silence;

		if (!frontend->m_Paused)
		{
			frontend->GetState()->audio_Output[frontend->GetState()->audio_Output_Pos % 4096] = (float)((double)audio_stream[it] / 32767.0);
			++frontend->GetState()->audio_Output_Pos %= 4096;
		}
	}
}

int SDLFrontEnd::initAudio()
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
		std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        m_State.running = false;
        return -1;
    }

	SDL_zero(audio_spec);
	audio_spec.freq = SAMPLE_FREQ;
	audio_spec.format = AUDIO_S16;
	audio_spec.channels = 1;
	audio_spec.samples = 64;
	audio_spec.userdata = this;
	audio_spec.callback = audio_callback;
	m_Audio_Device = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, 0);
	SDL_PauseAudioDevice(m_Audio_Device, 0);

    return 0;
}

void SDLFrontEnd::deinit()
{
	if (imgui_UI)
	{
		delete imgui_UI;
		imgui_UI = nullptr;
	}


    SDL_Quit();
}

void SDLFrontEnd::deinitAudio()
{
    SDL_CloseAudioDevice(m_Audio_Device);
    SDL_CloseAudio();
}

void SDLFrontEnd::deinitVideo()
{
	if (imgui_UI)
		imgui_UI->Deinit();

	SDL_DestroyRenderer(m_State.renderer);
    SDL_DestroyWindow(m_State.window);
    
	
}

bool SDLFrontEnd::Run()
{
	if (m_Paused)
		m_Timer.start();	
	time_accumulator += m_Timer.elapsed<stopwatch::mus>();
	while (time_accumulator >= m_FrameMicroSeconds) //if it's been less than 1/60th of a second since we started the previous frame, do nothing
	{
		time_accumulator -= m_FrameMicroSeconds;
		AdvanceCore();
		
	}
	HandleInput();
	m_Timer.start();

	DrawScreen();
	if (imgui_UI)
	{
		imgui_UI->Draw();
	}

	SDL_RenderPresent(m_State.renderer);	

	if (m_State.core->RequestsRPLSave())
	{
		PersistRPL();
		m_State.core->ResetRPLRequest();
	}

	if (m_State.open_File && m_State.open_File->ready())
	{
		auto result = m_State.open_File->result();
		if (result.size())
		{
			m_State.last_File = result[0];
			m_State.core->Reset();
			Load(result[0]);
		}
		m_State.open_File = nullptr;
	}

    return m_State.running;
}

void SDLFrontEnd::AdvanceCore()
{
    if (!m_State.core->GetDebugStepping())
        m_State.core->Run((uint16_t)m_State.run_Cycles);
    else
    {
        m_State.core->Run(0);
    }
}

void SDLFrontEnd::DrawScreen()
{

	if (m_State.zoom_Changed)
	{
		m_State.zoom_Changed = false;
		//resize and redraw display
		m_Pixel.w = m_State.resolution_Zoom;
		m_Pixel.h = m_State.resolution_Zoom;
		ResetDisplayTexture();
		std::fill_n(m_State.core->GetPrevVRAM(), 128 * 64, 0);
		m_State.core->SetScreenDirty();
		m_State.core->SetWipeScreen();
	}
	if (m_State.grid_Toggled)
	{
		m_State.grid_Toggled = false;
		ResetResolution();
	}

	if (m_State.core->res.base_width != m_Res_Width || m_State.core->res.base_height != m_Res_Height)
	{
		ResetResolution();
	}


    if (m_State.core->GetScreenDirty())
    {

		SDL_SetRenderTarget(m_State.renderer, m_State.screen_Texture);

		if (m_State.core->GetWipeScreen())
		{
			SDL_SetRenderDrawColor(m_State.renderer, m_State.screen_Colors[0].r, m_State.screen_Colors[0].g, m_State.screen_Colors[0].b, m_State.screen_Colors[0].a);
			SDL_RenderFillRect(m_State.renderer, NULL);
			m_State.core->ResetWipeScreen();
		}

		uint8_t* CurrentFB = m_State.core->GetVRAM();
		uint8_t* PreviousFramebuffer = m_State.core->GetPrevVRAM();
		for (int i = 0; i < m_State.core->res.base_width * m_State.core->res.base_height; i++)
		{
			if ((CurrentFB[i]) ^ PreviousFramebuffer[i])
			{
				int x = i % m_Res_Width;
				int y = i / m_Res_Width;
				m_Pixel.x = (x * m_State.resolution_Zoom);				
				m_Pixel.y = (y * m_State.resolution_Zoom);
				if (m_State.debug_Grid_Lines)
				{
					m_Pixel.x += x + 1;
					m_Pixel.y += y + 1;
				}
				SDL_SetRenderDrawColor(m_State.renderer, m_State.screen_Colors[CurrentFB[i]].r, m_State.screen_Colors[CurrentFB[i]].g, m_State.screen_Colors[CurrentFB[i]].b, 0xFF);
				SDL_RenderFillRect(m_State.renderer, &m_Pixel);
			}
		}

		m_State.core->SaveCurrentVRAM(); //sets "previous frame buffer" based on the data we just drew
		m_State.core->ResetScreenDirty();

		SDL_SetRenderTarget(m_State.renderer, nullptr);
    }
	SDL_SetRenderTarget(m_State.renderer, nullptr);
	if(!debug_interface)
		SDL_RenderCopy(m_State.renderer, m_State.screen_Texture, NULL, NULL);  
}

void SDLFrontEnd::ResetResolution()
{
	m_Res_Width = m_State.core->res.base_width;
	m_Res_Height = m_State.core->res.base_height;

	ResetDisplayTexture();

	return;
}

void SDLFrontEnd::ResetDisplayTexture()
{
	if (m_State.screen_Texture)
		SDL_DestroyTexture(m_State.screen_Texture);

	int new_w, new_h;

	if (m_State.debug_Grid_Lines)
	{
		new_w = (m_State.core->res.base_width * m_State.resolution_Zoom) + m_State.core->res.base_width + 1;
		new_h = (m_State.core->res.base_height * m_State.resolution_Zoom) + m_State.core->res.base_height + 1;
	}
	else
	{
		new_w = (m_State.core->res.base_width * m_State.resolution_Zoom);
		new_h = (m_State.core->res.base_height * m_State.resolution_Zoom);
	}

	m_State.screen_Texture = SDL_CreateTexture(m_State.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, new_w, new_h);
	SDL_SetRenderTarget(m_State.renderer, m_State.screen_Texture);
	SDL_SetRenderDrawColor(m_State.renderer, m_State.screen_Colors[0].r, m_State.screen_Colors[0].g, m_State.screen_Colors[0].b, m_State.screen_Colors[0].a);
	SDL_RenderClear(m_State.renderer);
	SDL_SetRenderTarget(m_State.renderer, nullptr);
	std::fill_n(m_State.core->GetPrevVRAM(), 128 * 64, 0);
	m_State.core->SetScreenDirty();
	m_State.core->SetWipeScreen();

	if (!debug_interface)
		SDL_SetWindowSize(m_State.window, new_w, new_h);

	return;
}

void SDLFrontEnd::PersistRPL()
{
	LOG_INFO("Attempting to persist Super-Chip RPL Data.");
	memcpy(SuperChipRPLData, m_State.core->GetRPLMem(), 8);
	std::string file{ "" };
	file = m_State.last_File;
	if (file != "")
	{
		file += ".rpl";
		std::ofstream ofd(file, std::ios::binary | std::ios::out);
		if (!ofd.good())
		{
			LOG_ERROR("Could not open file to save RPL data: {}", file.c_str());
			return;
		}
		ofd.write((char*)SuperChipRPLData, 8);
		ofd.close();
	}
}

void SDLFrontEnd::SetTitle()
{
	std::string temp_str("KIP-8");

	if (!m_State.last_File.empty())
	{
		temp_str += "    ";
		size_t offset = m_State.last_File.find_last_of("/\\");
		temp_str += m_State.last_File.substr(offset + 1);
	}

	if (m_Paused)
		temp_str += "    (Paused)";

	SDL_SetWindowTitle(m_State.window, temp_str.c_str());
}

void SDLFrontEnd::Load(std::string filename)
{
	

	LOG_INFO("Loading new file: {}", filename);
	std::ifstream ifd(filename, std::ios::binary | std::ios::ate);
	if (!ifd.good())
	{
		LOG_ERROR("File did not open correctly!: {}", filename.c_str());
		return;
	}
	size_t size = ifd.tellg();
	ifd.seekg(0, std::ios::beg);

	if (0x1FF + size >= m_State.core->GetRAMLimit()) //game is too big or possibly not even a chip-8 game
	{
		LOG_ERROR("File too large! {}", filename.c_str());
		return;
	}
	if (m_State.last_File != filename)
		m_State.last_File = filename;

	m_State.core->ResetMemory(m_State.core->GetSystemMode() == Chip8::SYSTEM_MODE::SUPER_CHIP); //if in super-chip mode, randomize, otherwise zero out memory

	//char* rom_dest = (char*)m_State.core->GetRAM() + 0x200; //TODO: read this into the file_data local buffer on the frontend and pass that by reference to a load function in the core instead
	m_State.file_data.resize(size);
	ifd.read((char*)m_State.file_data.data(), size);
	ifd.close();
	m_State.core->Load(m_State.file_data);

	SetTitle();
	
	//automatically try to load rpl data if available
	if (m_State.core->GetSystemMode() == Chip8::SYSTEM_MODE::SUPER_CHIP)
	{
		filename += ".rpl";
		ifd.open(filename, std::ios::binary | std::ios::ate);
		if (!ifd.good())
		{
			LOG_WARN("Unable to open RPL file for reading: {}", filename.c_str());
			return;
		}
		size = ifd.tellg();
		ifd.seekg(0, std::ios::beg);
		ifd.read((char*)m_State.core->GetRPLMem(), min(8, (int)size));
		ifd.close();
	}
}

void SDLFrontEnd::HandleInput()
{
	SDL_Event event;	

	while (SDL_PollEvent(&event))
	{
		if (imgui_UI)
			imgui_UI->HandleInput(&event);

		if (event.type == SDL_QUIT)
			m_State.running = false;
		if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(m_State.window))
			m_State.running = false;

		switch (event.type)
		{
			case(SDL_KEYUP):
			{
				//ignore key input if the emulator is paused OR if the debug ui is captureing keyboard input
				if (m_Paused || (imgui_UI && imgui_UI->WantCaptureKB()))
					break;
			
				//if the key pressed is in the emulator keymap, set the key state in the emulator
				if (keymap.find(event.key.keysym.scancode) != keymap.end())
				{
					m_State.core->SetKey(keymap.at(event.key.keysym.scancode), 0);
					break;
				}

				//handle other front-end specific hotkeys
				switch (event.key.keysym.scancode)
				{
					case(SDL_SCANCODE_GRAVE):
					{
						m_State.core->ToggleDebugStepping();
						break;
					}
					case(SDL_SCANCODE_F8):
					{
						m_State.core->Reset();
						break;
					}
					default:
						break;
				}
				break;
			}
			case(SDL_KEYDOWN):
			{
				//if debug ui is capturing key input, ignore this key input
				if (imgui_UI && imgui_UI->WantCaptureKB())
				{
						break;
				}
				//handle front-end specific hotkeys
				switch (event.key.keysym.scancode)
				{
					case(SDL_SCANCODE_ESCAPE):
					{
						m_State.running = false;
						break;
					}
					case(SDL_SCANCODE_SPACE):
					{
						if (m_State.core->GetDebugStepping())
							m_State.core->Step();
						break;
					}
					case(SDL_SCANCODE_LALT):
					{
						m_Paused = !m_Paused;
						
						SetTitle();

						break;
					}
					default:
						break;
				}

				//ignore game key input if the emulator is paused
				if (m_Paused)
					break;

				//if the key is part of the keymap, toggle game key input state
				if (keymap.find(event.key.keysym.scancode) != keymap.end())
				{
					m_State.core->SetKey(keymap.at(event.key.keysym.scancode), 1);
					break;
				}

				break;
			}
			default:
				break;
			}
	}
}
