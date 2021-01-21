//Using SDL and standard IO
#pragma warning(push, 0)
#include "cxxopts.hpp"
#pragma warning(pop)
#include "Chip8.h"
#include "SDLFrontEnd.h"


int main(int argc, char* argv[])
{
	Logger::Init();

	std::string filename = "";
	bool enableDebug = false, enableChip8 = true, enableSuperChip = false, enableXOChip = false; //enableOcto = false;
	int CPUSpeed = 9;
	cxxopts::Options options("KIP-8", "Cross platform CHIP-8 emulator");
	options.add_options()
		("r,rom", "Input rom file name", cxxopts::value<std::string>(filename))
		("d,debug", "Enable debugging", cxxopts::value<bool>(enableDebug)->default_value("false"))
		("C,Chip-8", "Set system mode to Chip-8", cxxopts::value<bool>(enableChip8)->default_value("true"))
		("S,Super-Chip", "Set system mode to Super-Chip", cxxopts::value<bool>(enableSuperChip)->default_value("false"))
		//("O,Octo", "Set system mode to Octo", cxxopts::value<bool>(enableOcto)->default_value("false"))
		("X,XO-Chip", "Set system mode to XO-Chip", cxxopts::value<bool>(enableXOChip)->default_value("false"))
		("s,speed", "Set CPU cycles per frame", cxxopts::value<int>(CPUSpeed)->default_value("9"))
		("h,help", "Print usage")
		;
	auto result = options.parse(argc, argv);

	if (result.count("help"))
	{
		std::cout << options.help() << std::endl;
		exit(0);
	}

	Chip8* core = new Chip8();

	//if (enableDebug)
	//	core.ToggleDebugStepping();
	if(enableXOChip)
		core->SetSystemMode(Chip8::SYSTEM_MODE::XO_CHIP);
	/*else if(enableOcto)
		core.SetSystemMode(Chip8::SYSTEM_MODE::OCTO);*/
	else if(enableSuperChip)
		core->SetSystemMode(Chip8::SYSTEM_MODE::SUPER_CHIP);
	else
		core->SetSystemMode(Chip8::SYSTEM_MODE::CHIP_8);
	core->Reset();
	if(filename != "")
		core->Load(filename); //second argument used as file name

	SDLFrontEnd* frontend = new SDLFrontEnd(core, enableDebug);
	frontend->SetRunCycles(std::max<int>(0,CPUSpeed));

	while (frontend->Run()) {} //just run the emulator until the frontend says not to

	delete frontend;
	delete core;
	
	return 0;
}