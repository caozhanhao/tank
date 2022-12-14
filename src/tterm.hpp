//   Copyright 2022 tank - caozhanhao
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
#ifndef TANK_TTERM_HPP
#define TANK_TTERM_HPP
#include <iostream>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <conio.h>
#elif defined(__linux__)
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#endif

namespace czh::term
{
	class TermPos
	{
	private:
		std::size_t x;
		std::size_t y;
	public:
		TermPos(std::size_t x_, std::size_t y_)
			:x(x_), y(y_) { }
        [[nodiscard]]std::size_t get_x() const { return x; }
        [[nodiscard]]std::size_t get_y() const { return y; }
	};
#if defined(__linux__)
	class KeyBoard
	{
	public:
		struct termios initial_settings, new_settings;
		int peek_character;
		KeyBoard()
		{
			tcgetattr(0, &initial_settings);
			new_settings = initial_settings;
			new_settings.c_lflag &= ~ICANON;
			new_settings.c_lflag &= ~ECHO;
			new_settings.c_lflag &= ~ISIG;
			new_settings.c_cc[VMIN] = 1;
			new_settings.c_cc[VTIME] = 0;
			tcsetattr(0, TCSANOW, &new_settings);
			peek_character = -1;
		}

		~KeyBoard()
		{
			tcsetattr(0, TCSANOW, &initial_settings);
		}

		int kbhit()
		{
			unsigned char ch;
			int nread;
			if (peek_character != -1) return 1;
			new_settings.c_cc[VMIN] = 0;
			tcsetattr(0, TCSANOW, &new_settings);
			nread = read(0, &ch, 1);
			new_settings.c_cc[VMIN] = 1;
			tcsetattr(0, TCSANOW, &new_settings);

			if (nread == 1) {
				peek_character = ch;
				return 1;
			}
			return 0;
		}

		int getch() {
			char ch;

			if (peek_character != -1) {
				ch = peek_character;
				peek_character = -1;
			}
			else read(0, &ch, 1);
			return ch;
		}
	};
	KeyBoard keyboard;
#endif
	int getch()
	{
#if defined(_WIN32) || defined(_WIN64)
		return _getch();
#elif defined(__linux__)
		return keyboard.getch();
#endif
	}
	bool kbhit()
	{
#if defined(_WIN32) || defined(_WIN64)
		return _kbhit();
#elif defined(__linux__)
		return keyboard.kbhit();
#endif
	}
  void move_cursor(const TermPos& pos)
  {
#if defined(_WIN32) || defined(_WIN64)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord{ (SHORT)pos.get_x(), (SHORT)pos.get_y() };
    SetConsoleCursorPosition(handle, coord);
#elif defined(__linux__)
    printf("%c[%d;%df", 0x1b, (int)pos.get_y() + 1, (int)pos.get_x() + 1);
#endif
  }
	void output(const std::string& str)
	{
		std::cout << str << std::flush;
	}
  void mvoutput(const TermPos& pos, const std::string& str)
  {
    move_cursor(pos);
    output(str);
  }
	std::size_t get_height()
	{
#if defined(_WIN32) || defined(_WIN64)
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(handle, &csbi);
		return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#elif defined(__linux__)
		struct winsize w;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		return w.ws_row;
#endif
	}
	std::size_t get_width()
	{
#if defined(_WIN32) || defined(_WIN64)
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(handle, &csbi);
		return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#elif defined(__linux__)
		struct winsize w;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		return  w.ws_col;
#endif
	}
	void clear()
	{
#if defined(_WIN32) || defined(_WIN64)
		HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO cinfo;
		DWORD recnum;
		COORD coord = { 0, 0 };
		GetConsoleScreenBufferInfo(handle, &cinfo);
		FillConsoleOutputCharacterW(handle, L' ', cinfo.dwSize.X * cinfo.dwSize.Y, coord, &recnum);
		FillConsoleOutputAttribute(handle, 0, cinfo.dwSize.X * cinfo.dwSize.Y, coord, &recnum);
		SetConsoleCursorPosition(handle, coord);
#elif defined(__linux__)
		printf("\033[2J");
#endif
	}
}
#endif