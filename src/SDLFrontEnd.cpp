#include "SDLFrontEnd.h"
#include <cmath>
#include <iostream>
#include <fstream>



void SDLFrontEnd::init()
{
    m_Running = true;
    m_Resolution_Zoom = 8;
	if (debug_interface)
		debugUI = new DebugUI(&m_Running, &m_Resolution_Zoom);
	m_Res_Width = kip8->res.base_width;
	m_Res_Height = kip8->res.base_height;
    m_Pixel.w = m_Resolution_Zoom;
    m_Pixel.h = m_Resolution_Zoom;
    m_Debug_Cycles = 1;
    m_Run_Cycles = 9;
    m_Window = NULL;
    m_Screen_Surface = NULL;
	m_Volume = 5.0; // 0 - 10
    initVideo();
    initAudio();
}

int SDLFrontEnd::initVideo()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        m_Running = false;
        return -1;
    }

	if(debugUI)
		m_Window = SDL_CreateWindow("KIP-8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
	else
		m_Window = SDL_CreateWindow("KIP-8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, (kip8->res.base_width * m_Resolution_Zoom) + kip8->res.base_width + 1, (kip8->res.base_height * m_Resolution_Zoom) + kip8->res.base_height + 1, SDL_WINDOW_SHOWN);
	if (m_Window == NULL)
    {
        std::cout << "Window could not be created! SDL_Error: " <<  SDL_GetError() << std::endl;
        m_Running = false;
        return -1;
    }
    else
    {
		ScreenColors[0].r = 0x88;//default background color
		ScreenColors[0].g = 0x55;
		ScreenColors[0].b = 0x00;
		ScreenColors[0].a = 0xFF;

		ScreenColors[1].r = 0xEE;//default foreground color
		ScreenColors[1].g = 0xBB;
		ScreenColors[1].b = 0x00;
		ScreenColors[1].a = 0xFF;

		ScreenColors[2].r = 0xEE;//default plane 2 color
		ScreenColors[2].g = 0x55;
		ScreenColors[2].b = 0x00;
		ScreenColors[2].a = 0xFF;

		ScreenColors[3].r = 0x55;//default plane1 + plane 2 overlap color
		ScreenColors[3].g = 0x11;
		ScreenColors[3].b = 0x00;
		ScreenColors[3].a = 0xFF;


		//software blit a blank screen one time
        m_Screen_Surface = SDL_GetWindowSurface(m_Window);
        SDL_FillRect(m_Screen_Surface, NULL, SDL_MapRGB(m_Screen_Surface->format, ScreenColors[0].r, ScreenColors[0].g, ScreenColors[0].b));
        SDL_UpdateWindowSurface(m_Window);

		//make a hardware accel renderer for future screen drawing
		m_Renderer = SDL_CreateRenderer(m_Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

		windowRect.x = 0;
		windowRect.y = 0;
		SDL_GetWindowSize(m_Window, &windowRect.w, &windowRect.h);
		if(debugUI)
			debugUI->Init(m_Window, m_Renderer);

		ResetDisplayTexture();
    }
    return 0;
}

void audio_callback(void* user, Uint8* stream, int len) {

	SDLFrontEnd* frontend = (SDLFrontEnd * )user;

	for (int it = 0; it < len; it++)
	{
		if (frontend->kip8->GetSoundTimer() && !frontend->m_Paused) // ST > 0, play sound 
		{
			if (frontend->kip8->GetSystemMode() == Chip8::SYSTEM_MODE::XO_CHIP) //Play XO-Chip audio. Square wave read from pattern buffer
			{
				if (frontend->kip8->audio_pattern[(it / 8) % 16] >> (7 - (it % 8)) & 0x01) //read through the audio pattern buffer, check each bit as a separate sample
				{
					stream[it] = (Uint8)(10.0f * frontend->m_Volume); //each sample is just a 1 or 0, super basic square wave, so we just multiply it by the user configurable volume (output at volume or output silence)
				}
				else
					stream[it] = frontend->audio_spec.silence;
			}
			else //Not XO-Chip mode, just play a 512hz sine wave tone
			{
				stream[it] = (Sint8)(sin((double)frontend->m_Sample_Pos / frontend->SAMPLES_PER_SINE * M_PI * 2.0) * (10.0 * (double)frontend->m_Volume));
				frontend->m_Sample_Pos++;
			}
		}
		else
			stream[it] = frontend->audio_spec.silence;
	}
}

int SDLFrontEnd::initAudio()
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
		std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        m_Running = false;
        return -1;
    }

	SDL_zero(audio_spec);
	audio_spec.freq = SAMPLE_FREQ;
	audio_spec.format = AUDIO_S8;
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
	delete debugUI;
    SDL_Quit();
}

void SDLFrontEnd::deinitAudio()
{
    SDL_CloseAudioDevice(m_Audio_Device);
    SDL_CloseAudio();
}

void SDLFrontEnd::deinitVideo()
{
	if (debugUI)
		debugUI->Deinit();

	SDL_DestroyRenderer(m_Renderer);	
	SDL_FreeSurface(m_Screen_Surface);
    SDL_DestroyWindow(m_Window);
    
	
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
		//PlaySound();
		
	}
	HandleInput();
	m_Timer.start();

	DrawScreen();
	if (debugUI)
		debugUI->Draw(m_Window, m_Renderer, m_Screen_Texture, kip8, kip8->res.base_width, kip8->res.base_height, &m_Run_Cycles, ScreenColors, &debug_grid_lines, &m_Volume);
	SDL_RenderPresent(m_Renderer);	

	if (kip8->RequestsRPLSave())
	{
		PersistRPL();
		kip8->ResetRPLRequest();
	}

    return m_Running;
}

void SDLFrontEnd::AdvanceCore()
{
    if (!kip8->GetDebugStepping())
        kip8->Run((uint16_t)m_Run_Cycles);
    else
    {
        kip8->Run(0);
    }
}

//void SDLFrontEnd::PlaySound()
//{
//    if (kip8->GetBeeper())
//    {
//        kip8->ResetBeeper();
//        SDL_ClearQueuedAudio(m_Audio_Device);
//        //SDL_PauseAudioDevice(audio_device, 0);
//
//        // queue the requested frames' worth of beeping audio
//        Sint8 sample = 0;
//        const Uint32 sample_size = sizeof(Sint8) * 1;
//        Uint32 frame_audio_length = (Uint32)std::ceil((double)SAMPLE_FREQ / 60.0);
//
//        for (unsigned int i = 0; i < frame_audio_length * kip8->GetSoundTimer(); i++) {
//            // SDL_QueueAudio expects a signed 16-bit value
//            sample = (Sint8)(sin((double)m_Sample_Pos / SAMPLES_PER_SINE * M_PI * 2.0) * (10.0 * m_Volume));
//            ++m_Sample_Pos;
//
//            SDL_QueueAudio(m_Audio_Device, &sample, sample_size);
//        }
//
//        //sample = 0;
//        //SDL_QueueAudio(m_Audio_Device, &sample, sample_size);
//
//        //samplePos = 0;
//    }
//}

void SDLFrontEnd::DrawScreen()
{
	if (debugUI)
	{
		if (debugUI->GetZoom() != m_Resolution_Zoom)
		{
			//resize and redraw display
			m_Resolution_Zoom = debugUI->GetZoom();
			m_Pixel.w = m_Resolution_Zoom;
			m_Pixel.h = m_Resolution_Zoom;
			ResetDisplayTexture();
			std::fill_n(kip8->GetPrevVRAM(), 128 * 64, 0);
			kip8->SetScreenDirty();
			kip8->SetWipeScreen();
		}
		if (debugUI->GetGridToggled())
		{
			debugUI->ResetGridToggled();
			ResetResolution();
		}
	}

	if (kip8->res.base_width != m_Res_Width || kip8->res.base_height != m_Res_Height)
	{
		ResetResolution();
	}


    if (kip8->GetScreenDirty())
    {
		if(debugUI)
			SDL_SetRenderTarget(m_Renderer, m_Screen_Texture);
		else
			SDL_SetRenderTarget(m_Renderer, nullptr);

		if (kip8->GetWipeScreen())
		{
			SDL_SetRenderDrawColor(m_Renderer, ScreenColors[0].r, ScreenColors[0].g, ScreenColors[0].b, ScreenColors[0].a);
			SDL_RenderFillRect(m_Renderer, NULL);
			kip8->ResetWipeScreen();
		}

		uint8_t* CurrentFB = kip8->GetVRAM();
		uint8_t* PreviousFramebuffer = kip8->GetPrevVRAM();
		for (int i = 0; i < kip8->res.base_width * kip8->res.base_height; i++)
		{
			if ((CurrentFB[i]) ^ PreviousFramebuffer[i])
			{
				int x = i % m_Res_Width;
				int y = i / m_Res_Width;
				m_Pixel.x = (x * m_Resolution_Zoom);				
				m_Pixel.y = (y * m_Resolution_Zoom);
				if (debug_grid_lines)
				{
					m_Pixel.x += x + 1;
					m_Pixel.y += y + 1;
				}
				SDL_SetRenderDrawColor(m_Renderer, ScreenColors[CurrentFB[i]].r, ScreenColors[CurrentFB[i]].g, ScreenColors[CurrentFB[i]].b, 0xFF);
				SDL_RenderFillRect(m_Renderer, &m_Pixel);
			}
		}

		kip8->SaveCurrentVRAM();
		kip8->ResetScreenDirty();

		SDL_SetRenderTarget(m_Renderer, nullptr);

    }

	SDL_RenderPresent(m_Renderer);    
}

void SDLFrontEnd::ResetResolution()
{
	m_Res_Width = kip8->res.base_width;
	m_Res_Height = kip8->res.base_height;

	ResetDisplayTexture();

	return;
}

void SDLFrontEnd::ResetDisplayTexture()
{
	if (m_Screen_Texture)
		SDL_DestroyTexture(m_Screen_Texture);

	if(debug_grid_lines)
		m_Screen_Texture = SDL_CreateTexture(m_Renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, (kip8->res.base_width * m_Resolution_Zoom) + kip8->res.base_width + 1,
		(kip8->res.base_height * m_Resolution_Zoom) + kip8->res.base_height + 1);
	else
		m_Screen_Texture = SDL_CreateTexture(m_Renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, (kip8->res.base_width * m_Resolution_Zoom), (kip8->res.base_height * m_Resolution_Zoom));

	SDL_SetRenderTarget(m_Renderer, m_Screen_Texture);
	SDL_SetRenderDrawColor(m_Renderer, ScreenColors[0].r, ScreenColors[0].g, ScreenColors[0].b, ScreenColors[0].a);
	SDL_RenderClear(m_Renderer);
	SDL_SetRenderTarget(m_Renderer, nullptr);
	std::fill_n(kip8->GetPrevVRAM(), 128 * 64, 0);
	kip8->SetScreenDirty();
	kip8->SetWipeScreen();
	return;
}

void SDLFrontEnd::PersistRPL()
{
	LOG_INFO("Attempting to persist Super-Chip RPL Data.");
	memcpy(SuperChipRPLData, kip8->GetRPLMem(), 8);
	std::string file{ "" };
	if(debugUI)
	 file = debugUI->GetLastFilename();
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

void SDLFrontEnd::HandleInput()
{
	SDL_Event event;
	

	while (SDL_PollEvent(&event))
	{
		if(debugUI)
			debugUI->HandleInput(&event);
		if (event.type == SDL_QUIT)
			m_Running = false;
		if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(m_Window))
			m_Running = false;

		switch (event.type)
		{
		case(SDL_KEYUP):
		{
			if (debugUI)
			{
				if (debugUI->WantCaptureKB())
					break;
			}
			if (m_Paused)
				break;
			switch (event.key.keysym.scancode)
			{
			case(SDL_SCANCODE_1):
			{
				kip8->SetKey(0x1, 0);
				break;
			}
			case(SDL_SCANCODE_2):
			{
				kip8->SetKey(0x2, 0);
				break;
			}
			case(SDL_SCANCODE_3):
			{
				kip8->SetKey(0x3, 0);
				break;
			}
			case(SDL_SCANCODE_4):
			{
				kip8->SetKey(0xC, 0);
				break;
			}
			case(SDL_SCANCODE_Q):
			{
				kip8->SetKey(0x4, 0);
				break;
			}
			case(SDL_SCANCODE_W):
			{
				kip8->SetKey(0x5, 0);
				break;
			}
			case(SDL_SCANCODE_E):
			{
				kip8->SetKey(0x6, 0);
				break;
			}
			case(SDL_SCANCODE_R):
			{
				kip8->SetKey(0xD, 0);
				break;
			}
			case(SDL_SCANCODE_A):
			{
				kip8->SetKey(0x7, 0);
				break;
			}
			case(SDL_SCANCODE_S):
			{
				kip8->SetKey(0x8, 0);
				break;
			}
			case(SDL_SCANCODE_D):
			{
				kip8->SetKey(0x9, 0);
				break;
			}
			case(SDL_SCANCODE_F):
			{
				kip8->SetKey(0xE, 0);
				break;
			}
			case(SDL_SCANCODE_Z):
			{
				kip8->SetKey(0xA, 0);
				break;
			}
			case(SDL_SCANCODE_X):
			{
				kip8->SetKey(0x0, 0);
				break;
			}
			case(SDL_SCANCODE_C):
			{
				kip8->SetKey(0xB, 0);
				break;
			}
			case(SDL_SCANCODE_V):
			{
				kip8->SetKey(0xF, 0);
				break;
			}
			case(SDL_SCANCODE_GRAVE):
			{
				kip8->ToggleDebugStepping();
				break;
			}
			case(SDL_SCANCODE_F8):
			{
				kip8->Reset();
				break;
			}
			default:
				break;
			}
			break;
		}
		case(SDL_KEYDOWN):
		{
			if (debugUI)
			{
				if (debugUI->WantCaptureKB())
					break;
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
			{
				m_Running = false;
				break;
			}
			if(event.key.keysym.scancode == SDL_SCANCODE_SPACE)
			{
				if(kip8->GetDebugStepping())
					kip8->Step();
				break;
			}
			if (event.key.keysym.scancode == SDL_SCANCODE_LALT)
			{
				m_Paused = !m_Paused;
				if(m_Paused)
					SDL_SetWindowTitle(m_Window, "KIP-8 (Paused)");
				else
					SDL_SetWindowTitle(m_Window, "KIP-8");
				break;
			}
			if (m_Paused)
				break;
			switch (event.key.keysym.scancode)
			{
			case(SDL_SCANCODE_1):
			{
				kip8->SetKey(0x1, 1);
				break;
			}
			case(SDL_SCANCODE_2):
			{
				kip8->SetKey(0x2, 1);
				break;
			}
			case(SDL_SCANCODE_3):
			{
				kip8->SetKey(0x3, 1);
				break;
			}
			case(SDL_SCANCODE_4):
			{
				kip8->SetKey(0xC, 1);
				break;
			}
			case(SDL_SCANCODE_Q):
			{
				kip8->SetKey(0x4, 1);
				break;
			}
			case(SDL_SCANCODE_W):
			{
				kip8->SetKey(0x5, 1);
				break;
			}
			case(SDL_SCANCODE_E):
			{
				kip8->SetKey(0x6, 1);
				break;
			}
			case(SDL_SCANCODE_R):
			{
				kip8->SetKey(0xD, 1);
				break;
			}
			case(SDL_SCANCODE_A):
			{
				kip8->SetKey(0x7, 1);
				break;
			}
			case(SDL_SCANCODE_S):
			{
				kip8->SetKey(0x8, 1);
				break;
			}
			case(SDL_SCANCODE_D):
			{
				kip8->SetKey(0x9, 1);
				break;
			}
			case(SDL_SCANCODE_F):
			{
				kip8->SetKey(0xE, 1);
				break;
			}
			case(SDL_SCANCODE_Z):
			{
				kip8->SetKey(0xA, 1);
				break;
			}
			case(SDL_SCANCODE_X):
			{
				kip8->SetKey(0x0, 1);
				break;
			}
			case(SDL_SCANCODE_C):
			{
				kip8->SetKey(0xB, 1);
				break;
			}
			case(SDL_SCANCODE_V):
			{
				kip8->SetKey(0xF, 1);
				break;
			}
			default:
				break;
			}
			break;
		}
		default:
			break;
		}
	}
}
