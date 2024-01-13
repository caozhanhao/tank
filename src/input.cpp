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
#include "tank/game.h"
#include "tank/logger.h"
#include "tank/renderer.h"
#include "tank/term.h"
#include "tank/command.h"

extern int czh::game::keyboard_mode;
extern czh::game::Page czh::game::curr_page;

namespace czh::input
{
  Input get_input()
  {
    Input ret = Input::EMPTY;
    char ch = czh::term::getch();
    if (ch == -32)
    {
      ch = czh::term::getch();
      game::keyboard_mode = 0;
    }
    if (ch == 27)
    {
      czh::term::getch();
      ch = czh::term::getch();
      game::keyboard_mode = 1;
    }
    if (game::curr_page == game::Page::COMMAND)
    {
      if (game::keyboard_mode == 1)
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
        if (!game::history.empty())
        {
          game::cmd_string = game::history[game::history_pos];
          game::cmd_string_pos = game::cmd_string.size() - 1;
          if (game::history_pos != 0) --game::history_pos;
          }
          break;
        case Input::C_KEY_DOWN:
        if (!game::history.empty())
        {
          game::cmd_string = game::history[game::history_pos];
          game::cmd_string_pos = game::cmd_string.size() - 1;
          if (game::history_pos + 1 < game::history.size()) ++game::history_pos;
        }
          break;
        case Input::C_KEY_LEFT:
          if (game::cmd_string_pos != 0)
            --game::cmd_string_pos;
          break;
        case Input::C_KEY_RIGHT:
          if (game::cmd_string_pos + 1 < game::cmd_string.size())
            ++game::cmd_string_pos;
          break;
        case Input::C_KEY_BACKSPACE:
          if (game::cmd_string_pos != 0)
          {
            game::cmd_string.erase(game::cmd_string_pos, 1);
            --game::cmd_string_pos;
          }
          break;
        case Input::C_KEY_DELETE:
          if (game::cmd_string_pos + 1 != game::cmd_string.size())
            game::cmd_string.erase(game::cmd_string_pos + 1, 1);
          break;
        case Input::C_KEY_HOME:
          game::cmd_string_pos = 0;
          break;
        case Input::C_KEY_END:
          game::cmd_string_pos = game::cmd_string.size() - 1;
          break;
        case Input::C_KEY_ENTER:
          cmd::run_command(game::cmd_string);
          game::history.emplace_back(game::cmd_string);
          game::cmd_string = "/";
          game::cmd_string_pos = 0;
          game::history_pos = game::history.size() - 1;
          break;
        case Input::CHAR:
          game::cmd_string_pos++;
          game::cmd_string.insert(game::cmd_string.begin() + game::cmd_string_pos, ch);
          break;
      }
      game::output_inited = false;
      renderer::render();
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
          if (game::keyboard_mode == 1)
            logger::warn("Ignored key 72");
          else
            ret = Input::G_UP;
          break;
        case 'S':
        case 's':
          ret = Input::G_DOWN;
          break;
        case 80:
          if (game::keyboard_mode == 1)
            czh::logger::warn("Ignored key 80");
          else
            ret = Input::G_DOWN;
          break;
        case 'A':
          if (game::keyboard_mode == 1)
            ret = Input::G_UP;
          else
            ret = Input::G_DOWN;
          break;
        case 'a':
          ret = Input::G_LEFT;
          break;
        case 75:
          if (game::keyboard_mode == 1)
            czh::logger::warn("Ignored key 75");
          else
            ret = Input::G_LEFT;
          break;
        case 'D':
          if (game::keyboard_mode == 1)
            ret = Input::G_LEFT;
          else
            ret = Input::G_RIGHT;
          break;
        case 'd':
          ret = Input::G_RIGHT;
          break;
        case 77:
          if (game::keyboard_mode == 1)
            czh::logger::warn("Ignored key 77");
          else
            ret = Input::G_RIGHT;
          break;
        case 'B':
          if (game::keyboard_mode == 0)
            czh::logger::warn("Ignored key 76");
          else
            ret = Input::G_DOWN;
          break;
        case 'C':
          if (game::keyboard_mode == 0)
            czh::logger::warn("Ignored key 77");
          else
            ret = Input::G_RIGHT;
          break;
        case ' ':
          ret = Input::G_KEY_SPACE;
          break;
        case 'O':
        case 'o':
          ret = Input::G_KEY_O;
          break;
        case 13:
          if (game::keyboard_mode == 1)
            czh::logger::warn("Ignored key 13");
          else
           ret = Input::M_KEY_ENTER;
          break;
        case 10:
          if (game::keyboard_mode == 0)
            czh::logger::warn("Ignored key 10");
          else
            ret = Input::M_KEY_ENTER;
          break;
        case 'L':
        case 'l':
          ret = Input::G_KEY_L;
          break;
        case '/':
          ret = Input::G_KEY_SLASH;
          break;
        default:
          czh::logger::warn("Ignored key ", static_cast<int>(ch), ".");
          break;
      }
    }
    return ret;
  }
}