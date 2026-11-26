#include "Engine/Core/CommandLine.h"
#include "Engine/Game.h"

#if WINDOWS_EXECUTABLE
	#include <windows.h>
int main(int argc, char* argv[]);
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	return main(__argc, __argv);  // Calls your main()
}
#endif

int main(int argc, char* argv[])
{
	// Before Game(): GameContext's construction already reads --demo.
	Struktur::Core::CommandLine::Set(argc, argv);
	Struktur::Game();
	return 0;
}
