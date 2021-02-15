//Using SDL and standard IO
#include "CLI11.hpp"
#include "Chip8.h"
#include "SDLFrontEnd.h"
#include <iostream>


int main(int argc, char* argv[])
{
	Logger::Init();

	std::string filename = "";
	bool enableGUI = false, enableChip8 = true, enableSuperChip = false, enableXOChip = false; //enableOcto = false;
	int CPUSpeed = 9;
	
	CLI::App app{"Cross platform CHIP-8 interpreter"};

	app.add_option("-r,--rom", filename, "Input rom file name");
	app.add_flag("-g,--GUI", enableGUI, "Enable Full GUI");
	app.add_flag("-C,--Chip-8", enableChip8, "Set system mode to Chip-8");
	app.add_flag("-S,--Super-Chip", enableSuperChip, "Set system mode to Super-Chip");
	app.add_flag("-X,--XO-Chip", enableXOChip, "Set system mode to XO-Chip");
	app.add_option("-s,--speed", CPUSpeed, "Set CPU cycles per frame");
	CLI11_PARSE(app, argc, argv);

	Chip8* core = new Chip8();

	if(enableXOChip)
		core->SetSystemMode(Chip8::SYSTEM_MODE::XO_CHIP);
	else if(enableSuperChip)
		core->SetSystemMode(Chip8::SYSTEM_MODE::SUPER_CHIP);
	else
		core->SetSystemMode(Chip8::SYSTEM_MODE::CHIP_8);
	core->Reset();

	SDLFrontEnd* frontend = new SDLFrontEnd(core, enableGUI);
	
	frontend->SetRunCycles(std::max<int>(0,CPUSpeed));

	if (filename != "")
		frontend->Load(filename);

	while (frontend->Run()) {} //just run the emulator until the frontend says not to

	delete frontend;
	delete core;
	
	return 0;
}