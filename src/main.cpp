#include "tgame.h"
#include <conio.h>
#include <thread>
#include <windows.h>
using namespace std;
using namespace czh::game;
void init()
{
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO cci;
	GetConsoleCursorInfo(handle, &cci);
	cci.bVisible = FALSE;
	SetConsoleCursorInfo(handle, &cci);
}
int main()
{
	init();
	Game game;
	game.add_tank();
	//game.add_auto_tank();
	while (true)
	{
		if (_kbhit())
		{
			switch (getch())
			{
			case 'w':
				game.react(0, Event::TANK_UP);
				break;
			case 's':
				game.react(0, Event::TANK_DOWN);
				break;
			case 'a':
				game.react(0, Event::TANK_LEFT);
				break;
			case 'd':
				game.react(0, Event::TANK_RIGHT);
				break;
			case 'f':
				game.react(0, Event::TANK_FIRE);
				break;
			case 'q':
				game.react(0, Event::QUIT);
				break;
			case 'l':
				game.add_auto_tank();
				break;
			default:
				game.react(0, Event::NOTHING);
				break;
			}
		}
		game.react(0, Event::NOTHING);
		std::this_thread::sleep_for(0.02s);
	}
	return 0;
}
