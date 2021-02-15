#include "Chip8.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <random>

Chip8::Chip8()
{
	Reset("Initializing");
}
Chip8::~Chip8()
{
}

void Chip8::Reset(std::string message)
{
	LOG_DEBUG("CPU reset: {}", message);
	srand((unsigned int)std::chrono::system_clock::now().time_since_epoch().count());
	
	ResetMemory(mode == SYSTEM_MODE::SUPER_CHIP);
	
	memcpy(&Memory[0x00], &Font[0], 80); //normal font. 5 bytes per character, 16 characters
	memcpy(&Memory[0x50], &LargeFont[0], 160); //large font. 10 bytes per character, 16 characters
	memcpy(&Memory[0x200], &LogoRom[0], 97); //load our default kip-8 logo rom on system reset
	
	std::fill_n(FrameBuffer, 128*64, 0);
	std::fill_n(PreviousFramebuffer, 128 * 64, 0);
	
	std::fill_n(Keys, 16, 0);
	std::fill_n(PrevKeys, 16, 0);

	sp = -1;
	std::fill_n(Stack, 64, 0);

	std::fill_n(regs.v, 16, 0);
	regs.i = 0;
	pc = 0x200;

	sound_timer = 0;
	delay_timer = 0;

	for (int it = 0; it < 16; it++) //TODO: make this configurable to match audio pattern buffer length, not hard coded to 16
	{
		audio_pattern[it] = 0xF0; //it % 2 ? 0xFF : 0x00;
	}

	res.hires = false;
	active_plane = 1;
	screen_dirty = true;
	SetScreenDirty();
	SetWipeScreen();
	UnHalt();

	return;
}
void Chip8::Run(uint16_t cycles) {
	if (!GetHalted() && !GetDebugStepping())
	{
		m_Run_Cycles = cycles;
	}
	uint8_t dt = GetDelayTimer();
	if (dt)
	{
		SetDelayTimer(dt - 1);
	}

	uint8_t st = GetSoundTimer();
	if (st)
	{
		//PLAY SOUND
		SetSoundTimer(st - 1);
	}
	if (m_Run_Cycles && !GetDebugStepping())
		LOG_TRACE("Running {} cycles.", m_Run_Cycles);
	while(m_Run_Cycles > 0)
	{
		m_Run_Cycles--;
		uint16_t op = Fetch(pc);
		pc += 2;
		Decode_Execute(op);
	}

	return;
}

void Chip8::ResetMemory(bool randomize)
{
	if (randomize)
		for (int i = 0x200; i <= RamLimit; i++)
			Memory[i] = rand() & 0xFF;
	else
		std::fill_n(&Memory[0x200], RamLimit - 0x200 + 1, 0);
}

uint16_t Chip8::Fetch(uint16_t location) {
	uint8_t upper_byte = Memory[location];
	uint8_t lower_byte = Memory[location + 1];
	
	uint16_t op = (uint16_t)(upper_byte << 8 | lower_byte);
	return op;
}
void Chip8::Load(const std::vector<unsigned char> &buffer)
{
	for (auto it = 0; it < buffer.size(); it++)
	{
		Memory[0x200 + it] = (uint8_t)buffer[it];
	}
}

uint8_t* Chip8::GetVRAM()
{
	return FrameBuffer;
}

uint8_t* Chip8::GetPrevVRAM()
{
	return PreviousFramebuffer;
}

void Chip8::SaveCurrentVRAM()
{
	memcpy(PreviousFramebuffer, FrameBuffer, 128 * 64);
}

void Chip8::SetScreenDirty()
{
	screen_dirty = true;
}

bool Chip8::ToggleDebugStepping(std::string message)
{
	debug_stepping = !debug_stepping;
	if (debug_stepping)
	{
		Logger::GetLogger()->set_level(spdlog::level::trace);
		LOG_DEBUG("Breakpoint: {}", message);
		Halt();
	}
	else
	{
		LOG_DEBUG("Continue: {}", message);
		Logger::GetLogger()->set_level(spdlog::level::info);
		UnHalt();
	}
	return debug_stepping;
}

void Chip8::SetSystemMode(SYSTEM_MODE newmode)
{
	LOG_INFO("Changing system mode: {}", newmode);
	mode = newmode;

	quirks.draw_wrap = false;
	quirks.draw_vblank = false;
	quirks.vip_jump = false;
	quirks.vip_regs_read_write = false;
	quirks.vip_shifts = false;
	quirks.logic_flag_reset = false;

	StackSize = 16;
	EntryPoint = 0x200;
	RamLimit = 0x0FFF;

	switch (mode)
	{
		case(SYSTEM_MODE::CHIP_8):
		{
			LOG_INFO("Set system mode CHIP-8");
			res.base_height = 32;
			res.base_width = 64;
			res.hires = false;
			quirks.vip_jump = true;
			quirks.vip_regs_read_write = true;
			quirks.vip_shifts = true;
			quirks.logic_flag_reset = true;
			quirks.draw_vblank = true;
			
			StackSize = 12;
			RamLimit = 0x0E8F; //upper limit for ram area. On VIP, E90 and up are reserved for stack, variables, etc
			
			break;
		}
		case(SYSTEM_MODE::SUPER_CHIP):
		{
			LOG_INFO("Set system mode SUPER-CHIP");
			res.base_height = 64;
			res.base_width = 128;
			res.hires = false;
			StackSize = 16;
			RamLimit = 0x0FFF;
			break;
		}
		case(SYSTEM_MODE::XO_CHIP):
		{
			LOG_INFO("Set system mode XO-Chip");
			res.base_height = 64;
			res.base_width = 128;
			res.hires = false;

			quirks.vip_jump = true;
			quirks.vip_regs_read_write = true;
			quirks.vip_shifts = true;
			quirks.draw_wrap = true;

			StackSize = 16;
			RamLimit = 0xFFFF;

			break;
		}
		default:
			break;
	}
	Reset("Changed System Mode.");
}

Chip8::SYSTEM_MODE Chip8::GetSystemMode()
{
	return mode;
}

void Chip8::Decode_Execute(uint16_t opcode) {
	uint8_t op_nibs[4] = { 0 };
	op_nibs[0] = (uint8_t)((opcode & 0xf000) >> 12);
	op_nibs[1] = (uint8_t)((opcode & 0x0f00) >> 8);
	op_nibs[2] = (uint8_t)((opcode & 0x00f0) >> 4);
	op_nibs[3] = (uint8_t)(opcode & 0x000f);
	switch (op_nibs[0])
	{
	case(0x0):
	{
		if ((opcode & 0x0FF0) == 0x0C0) // 0x00CN, scroll display down N pixels (SUPER-CHIP)
		{
			LOG_TRACE("[{:04X}] {:04X}\t00CN\tSCHIP  \tScroll down N", pc - 2, opcode);
			if (mode == SYSTEM_MODE::CHIP_8)
			{
				LOG_ERROR("Opcode not valid in CHIP-8 Mode: {:04X}", opcode);
			}
			else
			{

				if (op_nibs[3] == 0) //edge case elimination. do not scroll the screen 0 lines
					break;
				
				SetScreenDirty();

				uint8_t yoffset = op_nibs[3];
				uint8_t bytes_per_line = res.base_width;

				for (uint8_t i = res.base_height - 1; i >= yoffset; i--) //i is the y iterator, j is the x iterator
				{
					for (uint8_t j = 0; j < bytes_per_line; j++)
					{
						FrameBuffer[i * (bytes_per_line)+j] &= ~active_plane; //erase the bits corresponding to the in use draw plane, which will be shifted
						FrameBuffer[i * (bytes_per_line)+j] |= (FrameBuffer[(i - yoffset) * (bytes_per_line) + j]) & active_plane; // shift the affected bits by ORing them from source to destination
					}
				}
				for (int8_t i = yoffset - 1; i >= 0; i--) //i is the y iterator, j is the x iterator
				{
					for (uint8_t j = 0; j < bytes_per_line; j++)
					{
						FrameBuffer[i * (bytes_per_line)+j] &= ~active_plane; //erase the bits corresponding to the in use draw plane, which will be shifted
					}
				}
			}
			break;
		}
		if ((opcode & 0x0FF0) == 0x0D0) // 0x00DN, scroll display up N pixels (XO-Chip)
		{
			LOG_TRACE("[{:04X}] {:04X}\t00DN\tXO-CHIP\tScroll up N", pc - 2, opcode);
			if (mode == SYSTEM_MODE::CHIP_8)
			{
				LOG_ERROR("Opcode not valid in CHIP-8 Mode: {:04X}", opcode);
			}
			else if(mode == SYSTEM_MODE::SUPER_CHIP)
			{
				LOG_ERROR("Opcode not valid in SUPER-CHIP Mode: {:04X}", opcode);
			}
			else
			{
				if (op_nibs[3] == 0)
					break;

				SetScreenDirty();

				uint8_t yoffset = op_nibs[3];
				if (!res.hires)
					yoffset *= 2;
				uint8_t bytes_per_line = res.base_width;

				for (int i = 0; i < res.base_height - yoffset; i++) //i is the y iterator, j is the x iterator
				{
					for (uint8_t j = 0; j < bytes_per_line; j++)
					{
						FrameBuffer[i * (bytes_per_line)+j] &= ~active_plane; //erase the bits corresponding to the in use draw plane, which will be shifted
						FrameBuffer[i * (bytes_per_line)+j] |= (FrameBuffer[(i + yoffset) * (bytes_per_line) + j]) & active_plane; // shift the affected bits by ORing them from source to destination
					}
				}
				for (int i = res.base_height - (yoffset); i < res.base_height; i++) //i is the y iterator, j is the x iterator
				{
					for (uint8_t j = 0; j < bytes_per_line; j++)
					{
						FrameBuffer[i * (bytes_per_line) + j] &= ~active_plane; //erase the bits corresponding to the in use draw plane, which will be shifted
					}
				}
			}
			break;
		}
		switch ((opcode & 0x0FFF))
		{
			case(0x0E0): // 0x00E0, clear screen
			{
				LOG_TRACE("[{:04X}] {:04X}\t00E0\tCHIP-8 \tClear screen", pc - 2, opcode);	
				
				for (int it = 0; it < res.base_height * res.base_width; it++)
				{
					FrameBuffer[it] &= ~active_plane;
					PreviousFramebuffer[it] &= ~active_plane;
				}

				SetScreenDirty();
				SetWipeScreen();
				break;
			}
			case(0x0EE): //0x00EE, return
			{
				LOG_TRACE("[{:04X}] {:04X}\t00EE\tCHIP-8 \tReturn", pc - 2, opcode);
				if (sp >= 0)
				{
					//set program counter to top value of stack, decrement stack pointer
					pc = Stack[sp];
					sp--;
					break;
				}
				else
				{
					LOG_ERROR("Invalid stack operation. Stack underflow!");
					LOG_INFO("PC: {:04X}", pc - 2);
					Halt();
					break;
				}
			}
			case(0x0FB): //0x00FB, scroll right. 4 pixels in hi-res, 2 pixels in low-res (SUPER-CHIP)
			{
				LOG_TRACE("[{:04X}] {:04X}\t00FB\tSCHIP  \tScroll right", pc - 2, opcode);
				if (mode == SYSTEM_MODE::CHIP_8)
					LOG_ERROR("Opcode not valid in CHIP-8 Mode: {:04X}", opcode);
				else
				{
					SetScreenDirty();
					for (uint8_t y = 0; y < res.base_height; y++)
					{
						for (uint8_t x = res.base_width - 1; x >= 4; x--)
						{
							FrameBuffer[y * (res.base_width) + x] &= ~active_plane; //erase the bits corresponding to the in use draw plane, which will be shifted
							FrameBuffer[y * (res.base_width) + x] |= ((FrameBuffer[y * (res.base_width) + x - 4]) & active_plane); // shift the affected bits by ORing them from source to destination
						}
						for (int8_t x = 3; x >= 0; x--)
						{
							FrameBuffer[y * (res.base_width) + x] &= ~active_plane; //erase the bits corresponding to the in use draw plane, which will be shifted
						}
					}

				}
				break;
			}
			case(0x0FC): //0x00FC, scroll left. 4 pixels in hi-res, 2 pixels in low-res (SUPER-CHIP)
			{
				LOG_TRACE("[{:04X}] {:04X}\t00FC\tSCHIP  \tScroll left", pc - 2, opcode);
				if (mode == SYSTEM_MODE::CHIP_8)
					LOG_ERROR("Opcode not valid in CHIP-8 Mode: {:04X}", opcode);
				else
				{
					SetScreenDirty();
					for (uint8_t y = 0; y < res.base_height; y++)
					{
						for (uint8_t x = 0; x < res.base_width - 4; x++)
						{
							FrameBuffer[y * (res.base_width) + x] &= ~active_plane; //erase the bits corresponding to the in use draw plane, which will be shifted
							FrameBuffer[y * (res.base_width) + x] |= ((FrameBuffer[y * (res.base_width) + x + 4]) & active_plane); // shift the affected bits by ORing them from source to destination
						}
						for (uint8_t x = res.base_width - 4; x < res.base_width; x++)
						{
							FrameBuffer[y * (res.base_width) + x] &= ~active_plane; //erase the bits corresponding to the in use draw plane, which will be shifted
						}
					}
				}
				break;
			}
			case(0x0FD): //0x00FD, Exit Interpreter (SUPER-CHIP)
			{
				LOG_TRACE("[{:04X}] {:04X}\t00FD\tSCHIP  \tExit interpreter", pc - 2, opcode);
				if (mode == SYSTEM_MODE::CHIP_8)
					LOG_ERROR("Opcode not valid in CHIP-8 Mode: {:04X}", opcode);
				else
				{
					LOG_WARN("Exit Interpreter called by program.");
					Halt();
				}
				break;
			}
			case(0x0FE): //0x00FE, Disable Hi-Res (SUPER-CHIP)
			{
				LOG_TRACE("[{:04X}] {:04X}\t00FE\tSCHIP  \tDisable Hi-Res", pc - 2, opcode);
				if (mode == SYSTEM_MODE::CHIP_8)
					LOG_ERROR("Opcode not valid in CHIP-8 Mode: {:04X}", opcode);
				else
				{
					if (mode == SYSTEM_MODE::XO_CHIP)
					{
						std::fill_n(FrameBuffer, 128 * 64, 0); //wipes all draw planes clean
						std::fill_n(PreviousFramebuffer, 128 * 64, 0);
						SetScreenDirty();
						SetWipeScreen();
					}
					SetLowRes();
				}
				break;
			}
			case(0x0FF): //0x00FF, Enable Hi-Res (SUPER-CHIP)
			{
				LOG_TRACE("[{:04X}] {:04X}\t00FF\tSCHIP  \tEnable Hi-Res", pc - 2, opcode);
				if (mode == SYSTEM_MODE::CHIP_8)
				{
					LOG_ERROR("Opcode not valid in CHIP-8 Mode: {:04X}", opcode);
				}
				else
				{
					if (mode == SYSTEM_MODE::XO_CHIP)
					{
						std::fill_n(FrameBuffer, 128 * 64, 0); //wipes all draw planes clean
						std::fill_n(PreviousFramebuffer, 128 * 64, 0);
						SetScreenDirty();
						SetWipeScreen();
					}
					SetHiRes();
				}
				break;
			}
			default:
				LOG_ERROR("[{:04X}] {:04X}\tUnknown opcode", pc - 2, opcode);
				Halt();
				break;
		}
		break;
	}
	case(0x1): //1NNN, jump
	{
		LOG_TRACE("[{:04X}] {:04X}\t1NNN\tCHIP-8 \tJump", pc - 2, opcode);
		pc = (uint16_t)(opcode & 0x0FFF);
		break;
	}
	case(0x2): //2NNN, call
	{
		LOG_TRACE("[{:04X}] {:04X}\t2NNN\tCHIP-8 \tCall", pc - 2, opcode);
		//push current pc to top of stack
		if (sp + 1 >= StackSize)
		{
			LOG_ERROR("Invalid stack operation. Stack overflow!\n\tPC: {:04X}", pc - 2);
			Halt();
			break;
		}
		else
		{
			sp++;
			Stack[sp] = pc;

			//set pc to NNN
			pc = (uint16_t)(opcode & 0x0FFF);
		}
		break;
	}
	case(0x3): //3XNN, skip over the next opcode if VX == NN
	{
		LOG_TRACE("[{:04X}] {:04X}\t3XNN\tCHIP-8 \tSkip if VX == NN", pc - 2, opcode);
		if (regs.v[op_nibs[1]] == (opcode & 0x00FF))
		{
			if (mode == SYSTEM_MODE::XO_CHIP && (((Memory[pc] << 8) | Memory[pc+1]) == 0xF000))
				pc += 2;
			pc += 2;
		}
		break;
	}
	case(0x4): //4XNN, skip over the next opcode if VX != NN
	{
		LOG_TRACE("[{:04X}] {:04X}\t4XNN\tCHIP-8 \tSkip if VX != NN", pc - 2, opcode);
		if (regs.v[op_nibs[1]] != (opcode & 0x00FF))
		{
			if (mode == SYSTEM_MODE::XO_CHIP && (((Memory[pc] << 8) | Memory[pc+1]) == 0xF000))
				pc += 2;
			pc += 2;
		}
		break;
	}
	case(0x5): 
	{
		switch (op_nibs[3])
		{
			case(0): //5XY0, skip over the next opcode if VX == VY
			{
				LOG_TRACE("[{:04X}] {:04X}\t5XY0\tCHIP-8 \tSkip if VX == VY", pc - 2, opcode);
				if (regs.v[op_nibs[1]] == regs.v[op_nibs[2]])
				{
					if (mode == SYSTEM_MODE::XO_CHIP && (((Memory[pc] << 8) | Memory[pc + 1]) == 0xF000))
						pc += 2;
					pc += 2;
				}
				break;
			}
			case(2): //5XY2, save vx - vy
			{
				if (mode == SYSTEM_MODE::CHIP_8)
				{
					LOG_ERROR("Opcode not valid in CHIP-8 Mode: {:04X}", opcode);
					break;
				}
				else if (mode == SYSTEM_MODE::SUPER_CHIP)
				{
					LOG_ERROR("Opcode not valid in SUPER-CHIP Mode: {:04X}", opcode);
					break;
				}
				LOG_TRACE("[{:04X}] {:04X}\t5XY2\tXO-CHIP\tSave VX to VY at I", pc - 2, opcode);
				bool ascending_order = op_nibs[1] < op_nibs[2] ? true : false;
				uint8_t num_of_regs;
				if (ascending_order)
				{
					num_of_regs = op_nibs[2] - op_nibs[1] + 1;
				}
				else
				{
					num_of_regs = op_nibs[1] - op_nibs[2] + 1;
				}
				if (regs.i < 0 || regs.i >(RamLimit + 1 - (num_of_regs)))
				{
					LOG_ERROR("Attempted memory access violation.\nAttempt to store register contents outside bounds of Memory: {:04X}\n\tPC: {:04X}", regs.i, pc - 2);
					Halt();
					break;
				}
				if (ascending_order)
				{
					for (uint8_t it = 0; it < num_of_regs; it++)
					{
						Memory[regs.i + it] = regs.v[op_nibs[1] + it];
					}
				}
				else
				{
					for (uint8_t it = 0; it < num_of_regs; it++)
					{
						Memory[regs.i + it] = regs.v[op_nibs[1] - it];
					}
				}

				break;
			}
			case(3): //5XY3, load vx - vy
			{
				if (mode == SYSTEM_MODE::CHIP_8)
				{
					LOG_ERROR("Opcode not valid in CHIP-8 Mode: {:04X}", opcode);
					break;
				}
				else if (mode == SYSTEM_MODE::SUPER_CHIP)
				{
					LOG_ERROR("Opcode not valid in SUPER-CHIP Mode: {:04X}", opcode);
					break;
				}
				LOG_TRACE("[{:04X}] {:04X}\t5XY3\tXO-CHIP\tLoad VX to VY from I", pc - 2, opcode);
				bool ascending_order = op_nibs[1] < op_nibs[2] ? true : false;
				uint8_t num_of_regs;
				if (ascending_order)
				{
					num_of_regs = op_nibs[2] - op_nibs[1] + 1;
				}
				else
				{
					num_of_regs = op_nibs[1] - op_nibs[2] + 1;
				}
				if (regs.i < 0 || regs.i >(RamLimit + 1 - (num_of_regs)))
				{
					LOG_ERROR("Attempted memory access violation.\nAttempt to load registers from outside bounds of Memory: {:04X}\n\tPC: {:04X}", regs.i, pc - 2);
					Halt();
					break;
				}
				if (ascending_order)
				{
					for (uint8_t it = 0; it < num_of_regs; it++)
					{
						regs.v[op_nibs[1] + it] = Memory[regs.i + it];
					}
				}
				else
				{
					for (uint8_t it = 0; it < num_of_regs; it++)
					{
						regs.v[op_nibs[1] - it] = Memory[regs.i + it];
					}
				}

				break;
			}
			default:
			{
				LOG_ERROR("[{:04X}] {:04X}\tUnknown opcode", pc - 2, opcode);
				Halt();
				break;
			}
		}

		break;
	}
	case(0x6): //6XNN, set X register to NN
	{
		LOG_TRACE("[{:04X}] {:04X}\t6XNN\tCHIP-8 \tSet VX = NN", pc - 2, opcode);
		uint8_t value = opcode & 0x00FF;
		regs.v[op_nibs[1]] = value;
		break;
	}
	case(0x7)://7XNN, add NN to X register
	{
		LOG_TRACE("[{:04X}] {:04X}\t7XNN\tCHIP-8 \tSet VX = VX + NN", pc - 2, opcode);
		unsigned int value = opcode & 0x00FF;
		unsigned int old_vx = regs.v[op_nibs[1]];
		regs.v[op_nibs[1]] = (uint8_t)((old_vx + value) % 0x100);
		break;
	}
	case(0x8):
	{
		switch (op_nibs[3])
		{
		case(0x0): //8XY0 	Store the value of register VY in register VX
				   //       VY is not affected
			LOG_TRACE("[{:04X}] {:04X}\t8XY0\tCHIP-8 \tSet VX = VY", pc - 2, opcode);
			regs.v[op_nibs[1]] = regs.v[op_nibs[2]];			
			break;
		case(0x1): //8XY1 	Set VX to VX OR VY
				   //       VY is not affected
			LOG_TRACE("[{:04X}] {:04X}\t8XY1\tCHIP-8 \tSet VX = VX OR VY", pc - 2, opcode);
			regs.v[op_nibs[1]] = regs.v[op_nibs[1]] | regs.v[op_nibs[2]];
			if (quirks.logic_flag_reset) { regs.v[0xF] = 0; }			
			break;
		case(0x2): //8XY2 	Set VX to VX AND VY
				   //       VY is not affected
			LOG_TRACE("[{:04X}] {:04X}\t8XY2\tCHIP-8 \tSet VX = VX AND VY", pc - 2, opcode);
			regs.v[op_nibs[1]] = regs.v[op_nibs[1]] & regs.v[op_nibs[2]];
			if (quirks.logic_flag_reset) { regs.v[0xF] = 0; }
			break;
		case(0x3): //8XY3 	Set VX to VX XOR VY
				   //       VY is not affected
			LOG_TRACE("[{:04X}] {:04X}\t8XY3\tCHIP-8 \tSet VX = VX XOR VY", pc - 2, opcode);
			regs.v[op_nibs[1]] = regs.v[op_nibs[1]] ^ regs.v[op_nibs[2]];
			if (quirks.logic_flag_reset) { regs.v[0xF] = 0; }
			break;
		case(0x4): //8XY4   VX = VX + VY
		{		   //       VY is not affected
				   //       Set VF to 01 if a carry occurs (VX + VY > 255)
				   //       Set VF to 00 if a carry does not occur (VX + VY <= 255)
			LOG_TRACE("[{:04X}] {:04X}\t8XY4\tCHIP-8 \tSet VX = VX + VY", pc - 2, opcode);
			uint8_t carry = 0;
			if (regs.v[op_nibs[1]] + regs.v[op_nibs[2]] > 0xFF)
				carry = 1;
			regs.v[op_nibs[1]] = regs.v[op_nibs[1]] + regs.v[op_nibs[2]];
			regs.v[0xF] = carry;
			break;
		}
		case(0x5): //8XY5   VX = VX - VY
		{		   //       VY is not affected
				   //       Set VF to 00 if a borrow occurs ( VX < VY)
				   //       Set VF to 01 if a borrow does not occur (VX >= VY)
			LOG_TRACE("[{:04X}] {:04X}\t8XY5\tCHIP-8 \tSet VX = VX - VY", pc - 2, opcode);
			uint8_t borrow = 1;
			if (regs.v[op_nibs[1]] < regs.v[op_nibs[2]])
				borrow = 0;
			regs.v[op_nibs[1]] = regs.v[op_nibs[1]] - regs.v[op_nibs[2]];
			regs.v[0xF] = borrow;
			break;
		}
		case(0x6): //8XY6   Shift Right
		{		   //       QUIRK vip_shifts: set VX = VY before shift
				   //       Shift VX right 1, store the shifted bit in VF
			if (quirks.vip_shifts)
			{
				LOG_TRACE("[{:04X}] {:04X}\t8XY6\tCHIP-8 \tSet VX = VY >> 1", pc - 2, opcode);
				regs.v[op_nibs[1]] = regs.v[op_nibs[2]];
			}
			else
			{
				LOG_TRACE("[{:04X}] {:04X}\t8XY6\tSCHIP  \tSet VX = VX >> 1", pc - 2, opcode);
			}
			regs.v[0xF] = regs.v[op_nibs[1]] & 0x01;
			regs.v[op_nibs[1]] = regs.v[op_nibs[1]] >> 1;
			break;
		}
		case(0x7): //8XY7   VX = VY - VX
		{		   //       VY is not affected
				   //       Set VF to 00 if a borrow occurs ( VY < VX)
				   //       Set VF to 01 if a borrow does not occur (VY >= VX)
			LOG_TRACE("[{:04X}] {:04X}\t8XY7\tCHIP-8 \tSet VX = VY - VX", pc - 2, opcode);
			uint8_t borrow = 1;
			if (regs.v[op_nibs[2]] < regs.v[op_nibs[1]])
				borrow = 0;
			regs.v[op_nibs[1]] = regs.v[op_nibs[2]] - regs.v[op_nibs[1]];
			regs.v[0xF] = borrow;
			break;
		}
		case(0xE): //8XYE   Shift Left
				   //       QUIRK vip_shifts: set VX = VY before shift
				   //       Shift VX left 1, store the shifted bit in VF
			if (quirks.vip_shifts)
			{
				LOG_TRACE("[{:04X}] {:04X}\t8XYE\tCHIP-8 \tSet VX = VY << 1", pc - 2, opcode);
				regs.v[op_nibs[1]] = regs.v[op_nibs[2]];
			}
			else
			{
				LOG_TRACE("[{:04X}] {:04X}\t8XYE\tSCHIP  \tSet VX = VX << 1", pc - 2, opcode);
			}
			regs.v[0xF] = (regs.v[op_nibs[1]]) >> 7;
			regs.v[op_nibs[1]] = regs.v[op_nibs[1]] << 1;
			break;
		default:
			LOG_ERROR("[{:04X}] {:04X}\tUnknown opcode", pc - 2, opcode);
			Halt();
			break;
		}
		break;
	}
	case(0x9): //9XY0, skip over the next opcode if VX != VY
	{
		LOG_TRACE("[{:04X}] {:04X}\t9XY0\tCHIP-8 \tSkip if VX != VY", pc - 2, opcode);
		if (regs.v[op_nibs[1]] != regs.v[op_nibs[2]])
		{
			if (mode == SYSTEM_MODE::XO_CHIP && (((Memory[pc] << 8) | Memory[pc+1]) == 0xF000))
				pc += 2;
			pc += 2;
		}
		break;
	}
	case(0xA): //ANNN, set I register to NNN
	{
		LOG_TRACE("[{:04X}] {:04X}\tANNN\tCHIP-8 \tSet I = NNN", pc - 2, opcode);
		regs.i = (uint16_t)(opcode & 0x0FFF);
 		break;
	}
	case(0xB): //BNNN, jump to XNN + vx 
	{          //QUIRK vip_jump: jump to NNN + v0 
		if (quirks.vip_jump)
		{
			LOG_TRACE("[{:04X}] {:04X}\tBNNN\tCHIP-8 \tJump V0 + NNN", pc - 2, opcode);
			pc = (uint16_t)((opcode & 0x0FFF) + regs.v[0x0]);
		}
		else
		{
			LOG_TRACE("[{:04X}] {:04X}\tBNNN\tSCHIP  \tJump VX + XNN", pc - 2, opcode);
			pc = (uint16_t)((opcode & 0x0FFF) + regs.v[op_nibs[1]]);
		}
		break;
	}
	case(0xC): //CXNN, Set VX to a random number with a mask of NN
	{
		LOG_TRACE("[{:04X}] {:04X}\tCXNN\tCHIP-8 \tSet VX = Random() & NN", pc - 2, opcode);
		regs.v[op_nibs[1]] = rand() & (opcode & 0xFF);
		break;
	}
	case(0xD): //DXYN Draw Sprite
	{
		//draw a sprite
		//sprite data is located at the i register
		//sprite is N pixels tall
		//X is the register holding the x position
		//Y is the register holding the y position
		//xy position is upper left corner of sprite, inclusive
		//the upper left corner is always screen wrapped
		//the rest of the sprite is either wrapped or clipped depending on quirk settings
		//new pixels are xor'ed onto the current data
		//chip-8 and super-chip only have 1 drawing surface
		//xo-chip has 2, layered over each other, with 4 colors representing the 2-bits per pixel combinations


		
		if (!active_plane) //if no drawing planes are active, then just reset VF
			break;
		SetScreenDirty(); //if any plane IS selected, mark the screen as needing to be redrawn


		uint8_t sprite_width = 8;
		uint8_t sprite_height = op_nibs[3];
		if (mode != SYSTEM_MODE::CHIP_8)
		{
			LOG_TRACE("[{:04X}] {:04X}\tDXYN\tSCHIP  \tDraw Sprite", pc - 2, opcode);
			if (sprite_height == 0)
			{
				sprite_height = 16;

				if(mode == SYSTEM_MODE::XO_CHIP || res.hires)
					sprite_width = 16;
			}
		}
		else
			LOG_TRACE("[{:04X}] {:04X}\tDXYN\tCHIP-8 \tDraw Sprite", pc - 2, opcode);


		uint8_t pixel_size = 1;
		if ((mode != SYSTEM_MODE::CHIP_8) && !res.hires)
			pixel_size = 2;									//draw 2x2 pixels for low resolution mode for super-chip/xo-chip
		
		uint8_t bytes_per_row = sprite_width / 8;

		uint16_t new_pixel=0;

		uint8_t start_y, start_x, dest_y, dest_x, schip_line_collisions;

		start_x = regs.v[op_nibs[1]]; //need these temp variables in case some crazy people feed VF in as X or Y
		start_y = regs.v[op_nibs[2]];
		schip_line_collisions = 0;

		uint16_t mask = 1 << 15;
		mask = mask >> (16 - sprite_width); //we'll read in either 1 or 2 bytes of sprite data per row. we then use this bitmask to check each bit to see if we draw that pixel to the screen.

		uint16_t sprite_data_i = regs.i;

		regs.v[0xF] = 0;

		for (int plane_it = 1; plane_it < 3; plane_it++) //TODO: don't hard code 2 planes here, config for variable number of max draw planes (for future 16-color chip extension)
		{
			if (!(plane_it & active_plane))
				continue;

			for (int y = 0; y < sprite_height; y++)
			{
				bool coll_this_line = false; //track number of lines with collisions / clipping for SCHIP 1.1 quirk
				if(mode == SYSTEM_MODE::XO_CHIP) //TODO: make this an octo-wrap-quirk toggle
					dest_y = ((start_y % (res.base_height / pixel_size)) + y);
				else
					dest_y = (start_y + y);
				if (dest_y >= (res.base_height / pixel_size))
				{
					if (quirks.draw_wrap)
						dest_y = dest_y % (res.base_height / pixel_size);
					else
					{
						schip_line_collisions++; //sprite will clip this line at border of screen. We need to count the number of clipped lines for the super-chip VF collision quirk
						continue;
					}
				}

				dest_y *= pixel_size;

				if (sprite_data_i + y >= RamLimit)
				{
					LOG_ERROR("Sprite data index out of bounds!\n\tPC: {:04X}", pc - 2);
					Halt();
				}

				//sprite rows are 1 byte in low resolution mode, 2 bytes in high resolution mode
				//this always reads in 2 bytes of data for a row, then uses bit shifts to keep 1 or both bytes depending on if it's low/high resolution mode
				uint16_t upper_pixel, lower_pixel;
				upper_pixel = Memory[sprite_data_i + (bytes_per_row * y)];
				lower_pixel = Memory[sprite_data_i + ((bytes_per_row * y) + 1)];
				new_pixel = (upper_pixel << 8) | lower_pixel;
				new_pixel = new_pixel >> (16 - sprite_width);

				for (int x = 0; x < sprite_width; x++)
				{
					if(mode == SYSTEM_MODE::XO_CHIP) //TODO make this an octo-wrap-quirk toggle
						dest_x = ((start_x % (res.base_width / pixel_size)) + x);
					else
						dest_x = (start_x + x);

					if (dest_x >= (res.base_width / pixel_size))
					{
						if (quirks.draw_wrap)
							dest_x = dest_x % (res.base_width / pixel_size);
						else
							break;
					}

					dest_x *= pixel_size;

					if (new_pixel & (mask >> x))
					{
						if (FrameBuffer[((dest_y)*res.base_width) + (dest_x)] & plane_it)
						{
							FrameBuffer[((dest_y)*res.base_width) + (dest_x)] &= ~plane_it;
							if (pixel_size == 2)
							{
								FrameBuffer[((dest_y)*res.base_width) + (dest_x + 1)] &= ~plane_it;
								FrameBuffer[((dest_y + 1) * res.base_width) + (dest_x)] &= ~plane_it;
								FrameBuffer[((dest_y + 1) * res.base_width) + (dest_x + 1)] &= ~plane_it;
							}
							regs.v[0xF] = 1;
							coll_this_line = true;
						}
						else
						{
							FrameBuffer[((dest_y)*res.base_width) + (dest_x)] |= plane_it;
							if (pixel_size == 2)
							{
								FrameBuffer[((dest_y)*res.base_width) + (dest_x + 1)] |= plane_it;
								FrameBuffer[((dest_y + 1) * res.base_width) + (dest_x)] |= plane_it;
								FrameBuffer[((dest_y + 1) * res.base_width) + (dest_x + 1)] |= plane_it;
							}
						}
					}

				}
				if (coll_this_line)				//TODO: potential bug/UB here if we use schip_line_collisions with > 1 plane active. schip is 1 plane only so "should" never occur.
					schip_line_collisions++;
			}

			sprite_data_i += bytes_per_row * sprite_height;

		}
		if ((mode == SYSTEM_MODE::SUPER_CHIP) && res.hires) //specific to SUPER-CHIP, sets VF to the number of lines that had a collision or were clipped off screen
			regs.v[0xF] = schip_line_collisions;
		//Quirk: COSMAC VIP would wait for VBlank to perform the draw, effectively stalling the program
		//       I'm simulating this by drawing to VRAM immediately, but then throwing out any cycles left until VBlank
		if (quirks.draw_vblank)
		{
			m_Run_Cycles = 0;
		}
		break;
	}
	case(0xE):
	{
		if ((opcode & 0x00FF) == 0x009E) //EX9E, Skip the following instruction if the key corresponding to the hex value currently stored in register VX is pressed
		{
			LOG_TRACE("[{:04X}] {:04X}\tEX9E\tCHIP-8 \tSkip if Key VX Pressed", pc - 2, opcode);
			if (regs.v[op_nibs[1]] > 0x000F)
			{
				LOG_WARN("Checking for invalid key code: {:02X}\n\tPC: {:04X}\n\tOP: {:04X}", regs.v[op_nibs[1]], pc - 2, opcode);
			}
			if (Keys[regs.v[op_nibs[1]] & 0x000F] != 0)
			{
				if (mode == SYSTEM_MODE::XO_CHIP && (((Memory[pc] << 8) | Memory[pc+1]) == 0xF000))
					pc += 2;
				pc += 2;
			}
			break;
		}
		else if ((opcode & 0x00FF) == 0x00A1) //EXA1, Skip the following instruction if the key corresponding to the hex value currently stored in register VX is NOT pressed
		{
			LOG_TRACE("[{:04X}] {:04X}\tEXA1\tCHIP-8 \tSkip if Key VX Not Pressed", pc - 2, opcode);
			if (regs.v[op_nibs[1]] > 0x000F)
			{
				LOG_WARN("Checking for invalid key code: {:02X}\n\tPC: {:04X}\n\tOP: {:04X}", regs.v[op_nibs[1]], pc - 2, opcode);
			}
			if (Keys[regs.v[op_nibs[1]] & 0x000F] == 0)
			{
				if (mode == SYSTEM_MODE::XO_CHIP && (((Memory[pc] << 8) | Memory[pc+1]) == 0xF000))
					pc += 2;
				pc += 2;
			}
			break;
		}
		LOG_ERROR("[{:04X}] {:04X}\tUnknown opcode", pc - 2, opcode);
		Halt();
		break;
	}
	case(0xF):
	{
		if (opcode == 0xF000)
		{
			if (mode == SYSTEM_MODE::CHIP_8)
			{
				LOG_ERROR("Opcode not valid in CHIP-8 Mode: {:04X}", opcode);
				break;
			}
			else if (mode == SYSTEM_MODE::SUPER_CHIP)
			{
				LOG_ERROR("Opcode not valid in SUPER-CHIP Mode: {:04X}", opcode);
				break;
			}
			
			LOG_TRACE("[{:04X}] {:04X}\tF000\tXO-CHIP\tSet I = NNNN", pc - 2, opcode);
			if (pc + 1 > RamLimit)
			{
				LOG_ERROR("Attempted memory access violation.\nAttempt read outside bounds of Memory: {:04X}", pc);
				Halt();
			}
			else
			{
				uint16_t addr = ((uint16_t)Memory[pc] << 8) | ((uint16_t)Memory[pc + 1]);
				regs.i = addr;
				pc += 2;
			}
			break;
		}

		switch (opcode & 0x00FF)
		{
		case(0x01): //FN01: select zero or more drawing planes by bitmask (0 <= n <= 3). XO-CHIP
		{
			if (mode == SYSTEM_MODE::CHIP_8)
			{
				LOG_ERROR("Opcode not valid in CHIP-8 Mode: {:04X}", opcode);
				break;
			}
			else if (mode == SYSTEM_MODE::SUPER_CHIP)
			{
				LOG_ERROR("Opcode not valid in SUPER-CHIP Mode: {:04X}", opcode);
				break;
			}
			LOG_TRACE("[{:04X}] {:04X}\tFN01\tXO-CHIP\tSet Draw Plane(s)", pc - 2, opcode);
			if (op_nibs[1] > 3) //TODO: allow more than 2 planes
				LOG_WARN("Attempt to set invalid draw plane(s). Valid numbers are 0 - 3.");
			else
				active_plane = op_nibs[1];
			break;
		}
		case(0x02):  //F002: read 16 bytes from i into the audio pattern buffer. XO-CHIP
		{
			if (mode == SYSTEM_MODE::CHIP_8)
			{
				LOG_ERROR("Opcode not valid in CHIP-8 Mode: {:04X}", opcode);
				break;
			}
			else if (mode == SYSTEM_MODE::SUPER_CHIP)
			{
				LOG_ERROR("Opcode not valid in SUPER-CHIP Mode: {:04X}", opcode);
				break;
			}
			if (opcode == 0xF002) 
			{
				LOG_TRACE("[{:04X}] {:04X}\tF002\tXO-CHIP\tLoad audio pattern buffer from I", pc - 2, opcode);
				if (regs.i + 15 > RamLimit)
				{
					LOG_ERROR("Attempted memory access violation.\nAttempt read outside bounds of Memory: {:04X}", regs.i);
					Halt();
					break;
				}
				//TODO: Implement XO-CHIP audio
				for (int it = 0; it < 16; it++) //TODO: make this resize with configurable buffer length, not hard coded 16
				{
					audio_pattern[it] = Memory[regs.i + it];
				}
				
			}
			break;
		}
		case(0x07): //FX07, Store the current value of the delay timer in register VX
			LOG_TRACE("[{:04X}] {:04X}\tFX07\tCHIP-8 \tSet VX = Delay Timer", pc - 2, opcode);
			regs.v[op_nibs[1]] = GetDelayTimer();
			break;
		case(0x0A): //FX0A, Wait for a keypress and store the result in register VX
			LOG_TRACE("[{:04X}] {:04X}\tFX0A\tCHIP-8 \tSet VX = Key [WAIT FOR KEY]", pc - 2, opcode);
			pc -= 2;
			for (int i = 0; i < 16; i++)
			{
				if (Keys[i] && !PrevKeys[i]) //only register keys which have been newly pressed by the player
				{
					PrevKeys[i] = Keys[i]; //hacky way of ignoring this key until it's released and pressed again
					regs.v[op_nibs[1]] = i;
					pc += 2;
					break;
				}
			}
			break;
		case(0x15): //FX15, Set the delay timer to the value of register VX
			LOG_TRACE("[{:04X}] {:04X}\tFX15\tCHIP-8 \tSet Delay Timer = VX", pc - 2, opcode);
			SetDelayTimer(regs.v[op_nibs[1]]);
			break;
		case(0x18): //FX18, Set the sound timer to the value of register VX
			LOG_TRACE("[{:04X}] {:04X}\tFX18\tCHIP-8 \tSet Sound Timer = VX", pc - 2, opcode);
			SetSoundTimer(regs.v[op_nibs[1]]);
			break;
		case(0x1E): //FX1E, Add the value stored in register VX to register I
			//Quirk probably: I + VX overflow (SCHIP)
			if (mode == SYSTEM_MODE::SUPER_CHIP)
			{
				if (regs.i + regs.v[op_nibs[1]] > 0xFFF)
					regs.v[0xF] = 1;
				else
					regs.v[0xF] = 0;
			}
			regs.i += regs.v[op_nibs[1]];
			LOG_TRACE("[{:04X}] {:04X}\tFX1E\tCHIP-8 \tSet I = I + VX", pc - 2, opcode);
			break;
		case(0x29): //FX29, Font character. Set I to the address for the font character stored in VX
		{	        //The system font is loaded into memory starting at 0x50 on system start / reset
					//Each font character is 5 bytes long

			//Quirk: SUPER-CHIP 1.0 large fonts.
			//    if the high nibble in VX is 1 (ie. for values between 10 and 19 in hex)
			//    point I to a 10-byte font sprite for the digit in the lower nibble of VX (only digits 0-9)
			if (quirks.schip_10_fonts && (regs.v[op_nibs[1]] > 0x0F) && (regs.v[op_nibs[1]] < 0x1A))
			{
				LOG_TRACE("[{:04X}] {:04X}\tFX29\tSCHIP  \tSet I = Large Font Char VX", pc - 2, opcode);
				regs.i = 0x50 + ((regs.v[op_nibs[1]] & 0xF) * 10);
			}
			else
			{
				LOG_TRACE("[{:04X}] {:04X}\tFX29t\tCHIP-8 \tSet I = Font Char VX", pc - 2, opcode);
				regs.i = ((regs.v[op_nibs[1]] & 0xF) * 5);
			}
			break;
		}
		case(0x30): //FX30, Large Font character. Set I to the address for the large font character stored in VX
		{			//This is a SUPER-CHIP only instruction
					//Large font characters are 0-9 only for SUPER-CHIP and 0-F for Octo & XO-Chip
					//The characters are stored as 10 bytes each starting at 0x

			LOG_TRACE("[{:04X}] {:04X}\tFX30\tSCHIP  \tSet I = Large Font Char VX", pc - 2, opcode);
			if (mode == SYSTEM_MODE::CHIP_8)
				LOG_ERROR("Opcode not valid in CHIP-8 Mode: {:04X}", opcode);
			else
			{
				if ((regs.v[op_nibs[1]] > 0x9) && (mode == SYSTEM_MODE::SUPER_CHIP))
					LOG_WARN("Improper argument. SUPER-CHIP only supports large font digits 0-9.");
				regs.i = 0x50 + ((regs.v[op_nibs[1]] & 0xF) * 10);
			}
			break;
		}
		case(0x33): //FX33 Convert the value in VX to BCD, store the 3 byte value in memory at the address in I
		{
			LOG_TRACE("[{:04X}] {:04X}\tFX33\tCHIP-8 \tVX BCD, Store at I", pc - 2, opcode);

			if (regs.i < 0 || regs.i >(RamLimit - 2))
			{
				LOG_ERROR("Attempted memory access violation.\nAttempt to store BCD outside bounds of Memory: {:04X}\n\tPC: {:04X}", regs.i, pc - 2);
				Halt();
				break;
			}

			uint8_t hundreds, tens, ones;
			uint8_t VX = regs.v[op_nibs[1]];

			hundreds = (VX / 100) % 10;

			tens = (VX / 10) % 10;

			ones = VX % 10;

			Memory[regs.i] = hundreds;
			Memory[regs.i + 1] = tens;
			Memory[regs.i + 2] = ones;

			break;
		}
		case(0x55): //FX55 Save registers in memory. Save register V0 through VX in memory starting at the address in I
		{          
			LOG_TRACE("[{:04X}] {:04X}\tFX55\tCHIP-8 \tSave V0 to VX at I", pc - 2, opcode);
			uint8_t num_of_regs = op_nibs[1] + 1;
			if (regs.i < 0 || regs.i >(RamLimit + 1 - (num_of_regs)))
			{
				LOG_ERROR("Attempted memory access violation.\nAttempt to store register contents outside bounds of Memory: {:04X}\n\tPC: {:04X}", regs.i, pc - 2);
				Halt();
				break;
			}
			//QUIRK vip_regs_read_write: increment I register and read only from I register instead of using separate index variable
			if (quirks.vip_regs_read_write)
			{
				for (uint8_t it = 0; it < num_of_regs; it++)
				{
					Memory[regs.i] = regs.v[it];
					regs.i++;
				}
			}
			else
			{
				for (uint8_t it = 0; it < num_of_regs; it++)
				{
					Memory[regs.i + it] = regs.v[it];
				}
				if (quirks.schip_10_regs_read_write)
					regs.i += num_of_regs - 1;
			}
			break;
		}
		case(0x65): //FX65 Load memory into registers. Load the values in memory starting at the address in I into registers V0 to VX
		{	  
			LOG_TRACE("[{:04X}] {:04X}\tFX65\tCHIP-8 \tLoad V0 to VX from I", pc - 2, opcode);

			uint8_t num_of_regs = op_nibs[1] + 1;

			if (regs.i < 0 || regs.i >(RamLimit + 1 - num_of_regs))
			{
				LOG_ERROR("Attempted memory access violation.\nAttempt to load register contents from outside bounds of Memory: {:04X}\n\tPC: {:04X}", regs.i, pc - 2);
				Halt();
				break;
			}
			//QUIRK vip_regs_read_write: increment I register and read only from I register instead of using separate index variable
			if (quirks.vip_regs_read_write)
			{
				for (uint8_t it = 0; it < num_of_regs; it++)
				{
					regs.v[it] = Memory[regs.i];
					regs.i++;
				}
			}
			else
			{
				for (uint8_t it = 0; it < num_of_regs; it++)
				{
					regs.v[it] = Memory[regs.i + it];
				}
			}
			break;
		}
		case(0x75): //FX75 Save registers V0 to VX into RPL Memory (SUPER-CHIP)
		{
			LOG_TRACE("[{:04X}] {:04X}\tFX75\tSCHIP  \tSave V0 to VX in RPL Memory", pc - 2, opcode);
			uint8_t highest_reg = op_nibs[1];
			if (highest_reg > 7)
			{
				LOG_WARN("Invalid argument. Attempting to save too many registers to RPL. Saving V0 to V7");
				highest_reg = 7;
			}

			for (uint8_t i = 0; i <= highest_reg; i++)
			{
				RPLMemory[i] = regs.v[i];
			}
			write_rpl = true;

			break;
		}
		case(0x85): //FX85 Load registers V0 to VX from RPL Memory (SUPER-CHIP)
		{
			LOG_TRACE("[{:04X}] {:04X}\tFX85\tSCHIP  \tLoad V0 to VX from RPL Memory", pc - 2, opcode);
			uint8_t highest_reg = op_nibs[1];
			if (highest_reg > 7)
			{
				LOG_WARN("Invalid argument. Attempting to load too many registers from RPL. Loading V0 to V7");
				highest_reg = 7;
			}

			for (uint8_t i = 0; i <= highest_reg; i++)
			{
				regs.v[i] = RPLMemory[i];
			}
			break;
		}
		default:
			LOG_ERROR("[{:04X}] {:04X}\tUnknown opcode", pc - 2, opcode);
			Halt();
			break;
		}
		break;
	}
	default:
		LOG_ERROR("[{:04X}] {:04X}\tUnknown opcode", pc - 2, opcode);
		Halt();
		break;
	}
	return;
}
