#include "Engine/Game.h"

#if _WIN32
#include <windows.h>
int main(int argc, char* argv[]);
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	return main(__argc, __argv);  // Calls your main()
}
#endif

int main(int argc, char* argv[])
{
	Struktur::Game();
	return 0;
}
