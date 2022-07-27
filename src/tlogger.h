#pragma once
#include "tterm.h"
#include <stdexcept>
#include <string>
#include <iostream>
#include <chrono>
#include <ctime>
#include <cstdio>

#define CZH_NOTICE(msg) czh::logger::output(std::string("N: ") + msg);
namespace czh::logger
{
  void output(const std::string &str)
  {
    term::move_cursor(term::TermPos(0, term::get_height() - 1));
    int a = term::get_width() - str.size();
    if (a > 0)
      term::output(str + std::string(a, ' '));
    else
      term::output(str);
  }
  
  std::string get_time()
  {
    auto tt = std::chrono::system_clock::to_time_t
        (std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int) ptm->tm_year + 1900, (int) ptm->tm_mon + 1, (int) ptm->tm_mday,
            (int) ptm->tm_hour, (int) ptm->tm_min, (int) ptm->tm_sec);
    return {date};
  }
}