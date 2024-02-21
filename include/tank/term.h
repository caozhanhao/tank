//   Copyright 2022-2024 tank - caozhanhao
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
#ifndef TANK_TERM_H
#define TANK_TERM_H
#pragma once

#include <iostream>

#if __has_include(<conio.h>) && __has_include(<windows.h>)
#define CZH_TANK_KEYBOARD_MODE_0

#include <Windows.h>
#include <conio.h>

#elif __has_include(<sys/ioctl.h>) && __has_include(<unistd.h>) && __has_include(<sys/select.h>) && __has_include(<termios.h>)
#define CZH_TANK_KEYBOARD_MODE_1

#include <cstdio>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

#else
#error "Unknown target."
#endif

namespace czh::term
{
  class KeyBoard
  {
  public:
    int keyboard_mode;
#if defined(CZH_TANK_KEYBOARD_MODE_0)
    DWORD initial_settings, new_settings;
#elif defined(CZH_TANK_KEYBOARD_MODE_1)
    struct termios initial_settings, new_settings;
    int peek_character;
#endif
    
    KeyBoard();
    
    ~KeyBoard();
    
    void init();
    
    void deinit();
    
    int kbhit();
    
    int getch();
  };
  
  class TermPos
  {
  private:
    std::size_t x;
    std::size_t y;
  public:
    TermPos(std::size_t x_, std::size_t y_)
        : x(x_), y(y_) {}
    
    [[nodiscard]]std::size_t get_x() const { return x; }
    
    [[nodiscard]]std::size_t get_y() const { return y; }
  };
  
  int getch();
  
  bool kbhit();
  
  void move_cursor(const TermPos &pos);
  
  void flush();
  
  template<typename ...Args>
  void output(Args &&...args)
  {
    (std::cout << ... << args);
  }
  
  template<typename ...Args>
  void mvoutput(const TermPos &pos, Args &&...args)
  {
    move_cursor(pos);
    output(std::forward<Args>(args)...);
  }
  
  std::size_t get_height();
  
  std::size_t get_width();
  
  void clear();
  
  void hide_cursor();
  
  void show_cursor();
}
#endif