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
#include "tank/command.h"
#include "tank/message.h"

namespace czh::input
{
  Input get_input()
  {
    Input ret = Input::EMPTY;
    int ch = czh::term::getch();
    if (ch == -32)
    {
      ch = czh::term::getch();
      g::keyboard_mode = 0;
    }
    if (ch == 27)
    {
      czh::term::getch();
      ch = czh::term::getch();
      g::keyboard_mode = 1;
    }
    if (g::curr_page == game::Page::COMMAND)
    {
      if (g::keyboard_mode == 1)
      {
        switch (ch)
        {
          case 'A':
            ret = Input::C_KEY_UP;
            break;
          case 'B':
            ret = Input::C_KEY_DOWN;
            break;
          case 10:
            ret = Input::C_KEY_ENTER;
            break;
          case 'D':
            ret = Input::C_KEY_LEFT;
            break;
          case 'C':
            ret = Input::C_KEY_RIGHT;
            break;
          case 127:
            ret = Input::C_KEY_BACKSPACE;
            break;
          case 126:
            ret = Input::C_KEY_DELETE;
            break;
          case 72:
            ret = Input::C_KEY_HOME;
            break;
          case 70:
            ret = Input::C_KEY_END;
            break;
          default:
            ret = Input::CHAR;
            break;
        }
      }
      else
      {
        switch (ch)
        {
          case 72:
            ret = Input::C_KEY_UP;
            break;
          case 80:
            ret = Input::C_KEY_DOWN;
            break;
          case 13:
            ret = Input::C_KEY_ENTER;
            break;
          case 75:
            ret = Input::C_KEY_LEFT;
            break;
          case 77:
            ret = Input::C_KEY_RIGHT;
            break;
          case 8:
            ret = Input::C_KEY_BACKSPACE;
            break;
          case 'S':
            ret = Input::C_KEY_DELETE;
            break;
          case 'G':
            ret = Input::C_KEY_HOME;
            break;
          case 'O':
            ret = Input::C_KEY_END;
            break;
          default:
            ret = Input::CHAR;
            break;
        }
      }
      
      switch (ret)
      {
        case Input::C_KEY_UP:
          if (!g::history.empty())
          {
            g::cmd_string = g::history[g::history_pos];
            g::cmd_string_pos = g::cmd_string.size() - 1;
            if (g::history_pos != 0) --g::history_pos;
          }
          break;
        case Input::C_KEY_DOWN:
          if (!g::history.empty())
          {
            g::cmd_string = g::history[g::history_pos];
            g::cmd_string_pos = g::cmd_string.size() - 1;
            if (g::history_pos + 1 < g::history.size()) ++g::history_pos;
          }
          break;
        case Input::C_KEY_LEFT:
          if (g::cmd_string_pos != 0)
          {
            --g::cmd_string_pos;
          }
          break;
        case Input::C_KEY_RIGHT:
          if (g::cmd_string_pos + 1 < g::cmd_string.size())
          {
            ++g::cmd_string_pos;
          }
          break;
        case Input::C_KEY_BACKSPACE:
          if (g::cmd_string_pos != 0)
          {
            g::cmd_string.erase(g::cmd_string_pos, 1);
            --g::cmd_string_pos;
          }
          break;
        case Input::C_KEY_DELETE:
          if (g::cmd_string_pos + 1 != g::cmd_string.size())
          {
            g::cmd_string.erase(g::cmd_string_pos + 1, 1);
          }
          break;
        case Input::C_KEY_HOME:
          g::cmd_string_pos = 0;
          break;
        case Input::C_KEY_END:
          g::cmd_string_pos = g::cmd_string.size() - 1;
          break;
        case Input::C_KEY_ENTER:
          cmd::run_command(g::user_id, g::cmd_string);
          
          g::history.emplace_back(g::cmd_string);
          g::cmd_string = "/";
          g::cmd_string_pos = 0;
          g::history_pos = g::history.size() - 1;
          break;
        case Input::CHAR:
          g::cmd_string_pos++;
          g::cmd_string.insert(g::cmd_string.begin() + static_cast<int>(g::cmd_string_pos), ch);
          break;
        default:
          break;
      }
      g::output_inited = false;
    }
    else
    {
      switch (ch)
      {
        case 'W':
        case 'w':
          ret = Input::G_UP;
          break;
        case 72:
          if (g::keyboard_mode == 1)
          {
            msg::warn(g::user_id, "Ignored key 72");
          }
          else
          {
            ret = Input::G_UP;
          }
          break;
        case 'S':
        case 's':
          ret = Input::G_DOWN;
          break;
        case 80:
          if (g::keyboard_mode == 1)
          {
            msg::warn(g::user_id, "Ignored key 80");
          }
          else
          {
            ret = Input::G_DOWN;
          }
          break;
        case 'A':
          if (g::keyboard_mode == 1)
          {
            ret = Input::G_UP;
          }
          else
          {
            ret = Input::G_DOWN;
          }
          break;
        case 'a':
          ret = Input::G_LEFT;
          break;
        case 75:
          if (g::keyboard_mode == 1)
          {
            msg::warn(g::user_id, "Ignored key 75");
          }
          else
          {
            ret = Input::G_LEFT;
          }
          break;
        case 'D':
          if (g::keyboard_mode == 1)
          {
            ret = Input::G_LEFT;
          }
          else
          {
            ret = Input::G_RIGHT;
          }
          break;
        case 'd':
          ret = Input::G_RIGHT;
          break;
        case 77:
          if (g::keyboard_mode == 1)
          {
            msg::warn(g::user_id, "Ignored key 77");
          }
          else
          {
            ret = Input::G_RIGHT;
          }
          break;
        case 'B':
          if (g::keyboard_mode == 0)
          {
            msg::warn(g::user_id, "Ignored key 76");
          }
          else
          {
            ret = Input::G_DOWN;
          }
          break;
        case 'C':
          if (g::keyboard_mode == 0)
          {
            msg::warn(g::user_id, "Ignored key 77");
          }
          else
          {
            ret = Input::G_RIGHT;
          }
          break;
        case ' ':
          ret = Input::G_KEY_SPACE;
          break;
        case 'O':
        case 'o':
          ret = Input::G_KEY_O;
          break;
        case 13:
          if (g::keyboard_mode == 1)
          {
            msg::warn(g::user_id, "Ignored key 13");
          }
          else
          {
            ret = Input::M_KEY_ENTER;
          }
          break;
        case 10:
          if (g::keyboard_mode == 0)
          {
            msg::warn(g::user_id, "Ignored key 10");
          }
          else
          {
            ret = Input::M_KEY_ENTER;
          }
          break;
        case 'L':
        case 'l':
          ret = Input::G_KEY_L;
          break;
        case '/':
          ret = Input::G_KEY_SLASH;
          break;
        default:
          msg::warn(g::user_id, "Ignored key " + std::to_string(static_cast<int>(ch)) + ".");
          break;
      }
    }
    return ret;
  }
}