//   Copyright 2022-2023 tank - caozhanhao
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
#include "internal/term.h"
#include <iostream>

namespace czh::term
{
#if defined(CZH_TANK_ENABLE_L_TERM)
  void KeyBoard::init()
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
  
  void KeyBoard::deinit()
  {
    tcsetattr(0, TCSANOW, &initial_settings);
  }
  
  KeyBoard::KeyBoard()
    {
    init();
    }
  
  KeyBoard::~KeyBoard()
    {
      deinit();
    }
    
    int KeyBoard::kbhit()
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
    
    int KeyBoard::getch()
    {
      char ch;
      if (peek_character != -1)
      {
        ch = peek_character;
        peek_character = -1;
      }
      else read(0, &ch, 1);
      return ch;
    }
  KeyBoard keyboard;
#endif
  int getch()
  {
#if defined(CZH_TANK_ENABLE_W_TERM)
    return _getch();
#elif defined(CZH_TANK_ENABLE_L_TERM)
    return keyboard.getch();
#endif
  }
  
  bool kbhit()
  {
#if defined(CZH_TANK_ENABLE_W_TERM)
    return _kbhit();
#elif defined(CZH_TANK_ENABLE_L_TERM)
    return keyboard.kbhit();
#endif
  }
  
  void move_cursor(const TermPos &pos)
  {
#if defined(CZH_TANK_ENABLE_W_TERM)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD coord{(SHORT) pos.get_x(), (SHORT) pos.get_y()};
    SetConsoleCursorPosition(handle, coord);
#elif defined(CZH_TANK_ENABLE_L_TERM)
    printf("%c[%d;%df", 0x1b, (int)pos.get_y() + 1, (int)pos.get_x() + 1);
#endif
  }
  
  void output(const std::string &str)
  {
    std::cout << str << std::flush;
  }
  
  void mvoutput(const TermPos &pos, const std::string &str)
  {
    move_cursor(pos);
    output(str);
  }
  
  std::size_t get_height()
  {
#if defined(CZH_TANK_ENABLE_W_TERM)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(handle, &csbi);
    return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#elif defined(CZH_TANK_ENABLE_L_TERM)
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_row;
#endif
  }
  
  std::size_t get_width()
  {
#if defined(CZH_TANK_ENABLE_W_TERM)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(handle, &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
#elif defined(CZH_TANK_ENABLE_L_TERM)
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return  w.ws_col;
#endif
  }
  
  void clear()
  {
#if defined(CZH_TANK_ENABLE_W_TERM)
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO cinfo;
    DWORD recnum;
    COORD coord = {0, 0};
    GetConsoleScreenBufferInfo(handle, &cinfo);
    FillConsoleOutputCharacterW(handle, L' ', cinfo.dwSize.X * cinfo.dwSize.Y, coord, &recnum);
    FillConsoleOutputAttribute(handle, 0, cinfo.dwSize.X * cinfo.dwSize.Y, coord, &recnum);
    SetConsoleCursorPosition(handle, coord);
#elif defined(CZH_TANK_ENABLE_L_TERM)
    printf("\033[2J");
#endif
  }
}