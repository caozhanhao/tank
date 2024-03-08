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
#include "tank/input.h"
#include "tank/globals.h"
#include "tank/game.h"
#include "tank/term.h"

#include <string>
#include <vector>
#include <algorithm>

namespace czh::g
{
  bool typing_command = false;
  std::string cmd_line{};
  size_t cmd_pos = 0;
  size_t cmd_last_cols = 0;
  std::vector<std::string> history{};
  size_t history_pos = 0;
  std::string hint{};
  bool hint_applicable = false;
}

namespace czh::input
{
  template<typename ...Args>
  void cmd_output(Args &&...args)
  {
    std::lock_guard<std::mutex> l(g::drawing_mtx);
    term::output(std::forward<Args>(args)...);
  }
  
  bool is_special_key(int c)
  {
    return (c >= 0 && c <= 6) || (c >= 8 && c <= 14) || c == 16 || c == 20 || c == 21 || c == 23 || c == 26 ||
           c == 27 || c == 127;
  }
  
  std::tuple<std::string, std::string> get_pattern()
  {
    if (g::cmd_line.empty()) return {"", ""};
    std::string before_pattern;
    std::string pattern;
    if (g::cmd_line.back() != ' ')
    {
      auto i = g::cmd_line.rfind(' ', g::cmd_pos);
      if (i != std::string::npos && i + 1 < g::cmd_line.size())
      {
        pattern = g::cmd_line.substr(i + 1);
        before_pattern = g::cmd_line.substr(0, i);
      }
      else
      {
        pattern = g::cmd_line;
      }
    }
    else
    {
      before_pattern = g::cmd_line;
      if (!before_pattern.empty()) before_pattern.pop_back();
    }
    return {before_pattern, pattern};
  }
  
  void get_hint()
  {
    std::string before_pattern, pattern;
    std::tie(before_pattern, pattern) = get_pattern();
    // command hint
    auto it_cmd = std::find_if(g::commands.cbegin(), g::commands.cend(),
                               [](auto &&f) { return utils::begin_with(f.cmd, g::cmd_line); });
    if (it_cmd != g::commands.cend())
    {
      g::hint = it_cmd->cmd.substr(g::cmd_line.size());
      g::hint_applicable = true;
      return;
    }
    // command args hint
    if (pattern.empty())
    {
      it_cmd = std::find_if(g::commands.cbegin(), g::commands.cend(),
                            [&before_pattern](auto &&f) { return utils::begin_with(f.cmd, before_pattern); });
      if (it_cmd != g::commands.cend())
      {
        g::hint = it_cmd->args;
        g::hint_applicable = false;
        return;
      }
    }
    
    // more hint for debug :)
    if (before_pattern == "connect" && pattern == "1")
    {
      g::hint = "27.0.0.1";
      g::hint_applicable = true;
      return;
    }
    if (before_pattern == "connect 127.0.0.1" && pattern.empty())
    {
      g::hint = "8080";
      g::hint_applicable = true;
      return;
    }
    
    if (before_pattern == "server" && pattern == "s")
    {
      g::hint = "tart";
      g::hint_applicable = true;
      return;
    }
    
    if (before_pattern == "server start" && pattern.empty())
    {
      g::hint = "8080";
      g::hint_applicable = true;
      return;
    }
    
    // history hint
    auto it_history = std::find_if(g::history.crbegin(), g::history.crend(),
                                   [](auto &&f) { return utils::begin_with(f, g::cmd_line); });
    if (it_history != g::history.crend())
    {
      g::hint = it_history->substr(g::cmd_line.size());
      g::hint_applicable = true;
      return;
    }
  }
  
  std::string highlight_cmd_line()
  {
    // TODO highlight
    return g::cmd_line;
  }
  
  void cmdline_refresh(bool with_hint = true)
  {
    std::lock_guard<std::mutex> l(g::drawing_mtx);
    term::move_cursor({0, g::screen_height - 1});
    term::show_cursor();
    // move to begin and clear the cmd_line
    if (g::cmd_last_cols != 0)
    {
      term::output("\x1b[", g::cmd_last_cols, "D\x1b[K");
    }
    else
    {
      term::output("\x1b[K");
    }
    // the current cmd_line
    term::output("/", highlight_cmd_line());
    // hint
    if (with_hint)
    {
      g::hint.clear();
      g::hint_applicable = false;
      get_hint();
      term::output("\x1b[2m", g::hint, "\x1b[0m");
    }
    // move cursor back
    auto cursor_col = g::cmd_pos;
    auto curr_col = g::hint.size() + g::cmd_line.size();
    if (curr_col > cursor_col)
    {
      term::output("\x1b[", curr_col - cursor_col, "D");
    }
    // save cmd_pos for next refresh
    g::cmd_last_cols = cursor_col;
  }
  
  void edit_refresh_line(bool with_hint)
  {
    cmdline_refresh(with_hint);
    term::flush();
  }
  
  void move_to_beginning()
  {
    if (g::cmd_pos == 0) return;
    cmd_output("\x1b[", g::cmd_pos, "D");
    g::cmd_pos = 0;
    g::cmd_last_cols = 0;
  }
  
  void move_to_end(bool apply_hint = true)
  {
    if (g::cmd_pos == g::cmd_line.size() && g::hint.empty()) return;
    bool refresh = false;
    
    if (apply_hint && !g::hint.empty() && g::hint_applicable)
    {
      g::cmd_line += g::hint;
      g::cmd_line += " ";
      g::hint.clear();
      g::hint_applicable = false;
      refresh = true;
    }
    
    auto origin_width = g::cmd_pos;
    g::cmd_pos = g::cmd_line.size();
    g::cmd_last_cols = g::cmd_pos;
    cmd_output("\x1b[", g::cmd_last_cols - origin_width, "C");
    if (refresh) edit_refresh_line();
  }
  
  void move_to_word_beginning()
  {
    auto origin = g::cmd_pos;
    if (g::cmd_line[g::cmd_pos - 1] == ' ')
    {
      --g::cmd_pos;
    }
    // curr is not space
    while (g::cmd_pos > 0 && g::cmd_line[g::cmd_pos] == ' ')
    {
      --g::cmd_pos;
    }
    // prev is space or begin
    while (g::cmd_pos > 0 && g::cmd_line[g::cmd_pos - 1] != ' ')
    {
      --g::cmd_pos;
    }
    g::cmd_last_cols = g::cmd_pos;
    cmd_output("\x1b[", origin - g::cmd_last_cols, "D");
  }
  
  void move_to_word_end()
  {
    auto origin = g::cmd_pos;
    // curr is not space
    while (g::cmd_pos < g::cmd_line.size() && g::cmd_line[g::cmd_pos] == ' ')
    {
      ++g::cmd_pos;
    }
    // next is space or end
    while (g::cmd_pos < g::cmd_line.size() && g::cmd_line[g::cmd_pos] != ' ')
    {
      ++g::cmd_pos;
    }
    g::cmd_last_cols = g::cmd_pos;
    cmd_output("\x1b[", g::cmd_last_cols - origin, "C");
  }
  
  void move_left()
  {
    if (g::cmd_pos > 0)
    {
      auto origin = g::cmd_pos;
      --g::cmd_pos;
      g::cmd_last_cols = g::cmd_pos;
      cmd_output("\x1b[", origin - g::cmd_last_cols, "D");
    }
  }
  
  void move_right()
  {
    if (g::cmd_pos < g::cmd_line.size())
    {
      auto origin = g::cmd_pos;
      ++g::cmd_pos;
      g::cmd_last_cols = g::cmd_pos;
      cmd_output("\x1b[", g::cmd_last_cols - origin, "C");
    }
  }
  
  void edit_delete()
  {
    if (g::cmd_pos >= g::cmd_line.size()) return;
    g::cmd_line.erase(g::cmd_pos, 1);
    edit_refresh_line();
  }
  
  void edit_backspace()
  {
    if (g::cmd_pos == 0) return;
    g::cmd_line.erase(g::cmd_pos - 1, 1);
    --g::cmd_pos;
    edit_refresh_line();
  }
  
  void edit_delete_next_word()
  {
    if (g::cmd_pos == g::cmd_line.size() - 1) return;
    auto i = g::cmd_pos;
    // skip space
    while (i < g::cmd_line.size() && g::cmd_line[i] == ' ')
    {
      ++i;
    }
    // find end
    while (i < g::cmd_line.size() && g::cmd_line[i] != ' ')
    {
      ++i;
    }
    g::cmd_line.erase(g::cmd_pos + 1, i - g::cmd_pos);
    edit_refresh_line();
  }
  
  void edit_history_helper(bool prev)
  {
    if (g::history_pos == g::history.size() - 1)
    {
      g::history.back() = g::cmd_line;
    }
    auto next_history = [prev]() -> int
    {
      auto origin = g::history_pos;
      while (g::history[origin] == g::history[g::history_pos])
        // skip same command
      {
        if (prev)
        {
          if (g::history_pos != 0)
          {
            --g::history_pos;
          }
          else
          {
            return -1;
          }
        }
        else
        {
          if (g::history_pos != g::history.size() - 1)
          {
            ++g::history_pos;
          }
          else
          {
            return -1;
          }
        }
      }
      return 0;
    };
    next_history();
    g::cmd_line = g::history[g::history_pos];
    g::cmd_pos = g::cmd_line.size();
    edit_refresh_line();
    move_to_end(false);
  }
  
  void edit_up()
  {
    edit_history_helper(true);
  }
  
  void edit_down()
  {
    edit_history_helper(false);
  }
  
  void edit_left()
  {
    move_left();
  }
  
  void edit_right()
  {
    move_right();
  }
  
  Input get_input()
  {
    if (g::typing_command)
    {
      while (true)
      {
        int buf = g::keyboard.getch();
        if (is_special_key(buf))
        {
          auto key = static_cast<SpecialKey>(buf);
          if (key == SpecialKey::TAB)
          {
            if (!g::hint.empty())
            {
              move_to_end();
            }
            continue;
          }
          switch (key)
          {
            case SpecialKey::CTRL_A:
              move_to_beginning();
              break;
            case SpecialKey::CTRL_B:
              edit_left();
              break;
            case SpecialKey::CTRL_C:
              g::history.pop_back();
              return Input::KEY_CTRL_C;
              continue;
              break;
            case SpecialKey::CTRL_Z:
              g::history.pop_back();
              return Input::KEY_CTRL_Z;
              continue;
              break;
            case SpecialKey::CTRL_D:
              edit_delete();
              break;
            case SpecialKey::CTRL_E:
              move_to_end();
              break;
            case SpecialKey::CTRL_F:
              edit_right();
              break;
            case SpecialKey::CTRL_K:
              edit_delete();
              break;
            case SpecialKey::CTRL_L:
              g::output_inited = false;
              term::clear();
              break;
            case SpecialKey::LINE_FEED:
            case SpecialKey::ENTER:
              if (g::cmd_line.empty())
              {
                g::history.pop_back();
              }
              else
              {
                g::history.back() = g::cmd_line;
              }
              edit_refresh_line(false);
              return Input::COMMAND;
              
              break;
            case SpecialKey::CTRL_N:
              edit_down();
              break;
            case SpecialKey::CTRL_P:
              edit_up();
              break;
            case SpecialKey::CTRL_T:
              if (g::cmd_pos != 0)
              {
                auto tmp = g::cmd_line[g::cmd_pos];
                g::cmd_line[g::cmd_pos] = g::cmd_line[g::cmd_pos - 1];
                g::cmd_line[g::cmd_pos - 1] = tmp;
              }
              break;
            case SpecialKey::CTRL_U:
              break;
            case SpecialKey::CTRL_W:
            {
              if (g::cmd_pos == 0) break;
              auto origin_pos = g::cmd_pos;
              while (g::cmd_pos > 0 && g::cmd_line[g::cmd_pos - 1] == ' ')
              {
                g::cmd_pos--;
              }
              while (g::cmd_pos > 0 && g::cmd_line[g::cmd_pos - 1] != ' ')
              {
                g::cmd_pos--;
              }
              g::cmd_line.erase(g::cmd_pos, origin_pos - g::cmd_pos);
              break;
            }
            case SpecialKey::ESC:// Escape Sequence
              char seq[3];
              std::cin.read(seq, 1);
              // esc ?
              if (seq[0] != '[' && seq[0] != 'O')
              {
                switch (seq[0])
                {
                  case 'd':
                    edit_delete_next_word();
                    break;
                  case 'b':
                    move_to_word_beginning();
                    break;
                  case 'f':
                    move_to_word_end();
                    break;
                }
              }
              else
              {
                std::cin.read(seq + 1, 1);
                // esc [
                if (seq[0] == '[')
                {
                  if (seq[1] >= '0' && seq[1] <= '9')
                  {
                    std::cin.read(seq + 2, 1);
                    if (seq[2] == '~' && seq[1] == '3')
                    {
                      edit_delete();
                    }
                    else if (seq[2] == ';')
                    {
                      std::cin.read(seq, 2);
                      if (seq[0] == '5' && seq[1] == 'C')
                      {
                        move_to_word_end();
                      }
                      if (seq[0] == '5' && seq[1] == 'D')
                      {
                        move_to_word_beginning();
                      }
                    }
                  }
                  else
                  {
                    switch (seq[1])
                    {
                      case 'A':
                        edit_up();
                        break;
                      case 'B':
                        edit_down();
                        break;
                      case 'C':
                        edit_right();
                        break;
                      case 'D':
                        edit_left();
                        break;
                      case 'H':
                        move_to_beginning();
                        break;
                      case 'F':
                        move_to_end();
                        break;
                      case 'd':
                        edit_delete_next_word();
                        break;
                      case '1':
                        move_to_beginning();
                        break;
                      case '4':
                        move_to_end();
                        break;
                    }
                  }
                }
                  // esc 0
                else if (seq[0] == 'O')
                {
                  switch (seq[1])
                  {
                    case 'A':
                      edit_up();
                      break;
                    case 'B':
                      edit_down();
                      break;
                    case 'C':
                      edit_right();
                      break;
                    case 'D':
                      edit_left();
                      break;
                    case 'H':
                      move_to_beginning();
                      break;
                    case 'F':
                      move_to_end();
                      break;
                  }
                }
              }
              break;
            case SpecialKey::BACKSPACE:
            case SpecialKey::CTRL_H:
              edit_backspace();
              break;
            default:
              //msg::warn(g::user_id, "Ignored unrecognized key '" + std::string(1, buf) + "'.");
              continue;
              break;
          }
          edit_refresh_line();
        }
        else if (buf == 0xe0)
        {
          buf = g::keyboard.getch();
          switch (buf)
          {
            case 72:
              edit_up();
              break;
            case 80:
              edit_down();
              break;
            case 75:
              edit_left();
              break;
            case 77:
              edit_right();
              break;
            case 71:
              move_to_beginning();
              break;
            case 79:
              move_to_end();
              break;
            case 8:
              edit_backspace();
              break;
            case 83:
              edit_delete();
              break;
          }
        }
        else
        {
          g::cmd_line.insert(g::cmd_pos++, 1, buf);
          edit_refresh_line();
        }
      }
    }
    else
    {
      while (true)
      {
        int buf = g::keyboard.getch();
        if (is_special_key(buf))
        {
          auto key = static_cast<SpecialKey>(buf);
          if (key == SpecialKey::TAB)
          {
            continue;
          }
          switch (key)
          {
            case SpecialKey::CTRL_C:
              return Input::KEY_CTRL_C;
              break;
            case SpecialKey::CTRL_Z:
              return Input::KEY_CTRL_Z;
              break;
            case SpecialKey::LINE_FEED:
            case SpecialKey::ENTER:
              return Input::KEY_ENTER;
              break;
            case SpecialKey::CTRL_N:
              edit_down();
              break;
            case SpecialKey::CTRL_P:
              edit_up();
              break;
            case SpecialKey::ESC:// Escape Sequence
              char seq[3];
              std::cin.read(seq, 1);
              // esc ?
              if (seq[0] != '[' && seq[0] != 'O')
              {
                continue;
              }
              else
              {
                std::cin.read(seq + 1, 1);
                // esc [
                if (seq[0] == '[')
                {
                  if (seq[1] >= '0' && seq[1] <= '9')
                  {
                    std::cin.read(seq + 2, 1);
                    if (seq[2] == '~' && seq[1] == '3')
                    {
                      continue;
                    }
                    else if (seq[2] == ';')
                    {
                      std::cin.read(seq, 2);
                      continue;
                    }
                  }
                  else
                  {
                    switch (seq[1])
                    {
                      case 'A':
                        return Input::UP;
                        break;
                      case 'B':
                        return Input::DOWN;
                        break;
                      case 'C':
                        return Input::RIGHT;
                        break;
                      case 'D':
                        return Input::LEFT;
                        break;
                    }
                  }
                }
                  // esc 0
                else if (seq[0] == 'O')
                {
                  switch (seq[1])
                  {
                    case 'A':
                      return Input::UP;
                      break;
                    case 'B':
                      return Input::DOWN;
                      break;
                    case 'C':
                      return Input::RIGHT;
                      break;
                    case 'D':
                      return Input::LEFT;
                      break;
                  }
                }
              }
              break;
            default:
              //msg::warn(g::user_id, "Ignored unrecognized key '" + std::string(1, buf) + "'.");
              continue;
              break;
          }
        }
        else if (buf == 0xe0)
        {
          buf = g::keyboard.getch();
          switch (buf)
          {
            case 72:
              return Input::UP;
              break;
            case 80:
              return Input::DOWN;
              break;
            case 75:
              return Input::LEFT;
              break;
            case 77:
              return Input::RIGHT;
              break;
          }
        }
        else
        {
          if (buf == 'w' || buf == 'W')
          {
            return Input::UP;
          }
          else if (buf == 'a' || buf == 'A')
          {
            return Input::LEFT;
          }
          else if (buf == 's' || buf == 'S')
          {
            return Input::DOWN;
          }
          else if (buf == 'd' || buf == 'D')
          {
            return Input::RIGHT;
          }
          else if (buf == 'o' || buf == 'O')
          {
            return Input::KEY_O;
          }
          else if (buf == 'l' || buf == 'L')
          {
            return Input::KEY_L;
          }
          else if (buf == '/')
          {
            return Input::KEY_SLASH;
          }
          else if (buf == ' ')
          {
            return Input::KEY_SPACE;
          }
        }
      }
    }
    return Input::UNEXPECTED;
  }
}