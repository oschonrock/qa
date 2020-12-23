#include "console.hpp"
#include <iostream>
#include <string_view>

#ifdef _WIN32
#include <windows.h>
#endif
namespace xos::console {

namespace impl {

namespace ansi {
// ANSI Control sequences for *nix consoles
const std::string_view csi             = "\x1B["; // "Control Sequence Introducer"
const std::string_view cursor_up       = "A";
const std::string_view cursor_down     = "B";
const std::string_view cursor_forward  = "C";
const std::string_view cursor_backward = "D";

const std::string_view clear_screen = "2J";
const std::string_view home         = "H";
const std::string_view cursor_pos   = "H";

} // namespace ansi

// only try to compile the non-standard win32 code on windows
#ifdef _WIN32

void win32_move_cursor_relative(int dx, int dy) {
  HANDLE                     console = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO screen;
  GetConsoleScreenBufferInfo(console, &screen);
  COORD cursorPos = screen.dwCursorPosition;
  cursorPos.X += dx;
  cursorPos.Y += dy;
  SetConsoleCursorPosition(console, cursorPos);
}

void win32_console_clear_screen() {
  COORD                      topLeft = {0, 0};
  HANDLE                     console = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO screen;
  DWORD                      written;

  GetConsoleScreenBufferInfo(console, &screen);
  FillConsoleOutputCharacterA(console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
  FillConsoleOutputAttribute(console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
                             screen.dwSize.X * screen.dwSize.Y, topLeft, &written);
  SetConsoleCursorPosition(console, topLeft);
}

#endif

void nix_move_cursor_relative(int dx, int dy) {
  if (dx != 0) {
    std::cout << ansi::csi;
    if (dx > 0)
      std::cout << dx << ansi::cursor_forward;
    else
      std::cout << -dx << ansi::cursor_backward;
  }

  if (dy != 0) {
    std::cout << ansi::csi;
    if (dy > 0)
      std::cout << dy << ansi::cursor_down; // +ve == down for consistency with win32
    else
      std::cout << -dy << ansi::cursor_up;
  }
}

} // namespace impl

/**
 * @brief      platform indepent console cursor relative movement
 * @details    works on *nix, macOS and Windows
 * @param      int dx +ve is right
 * @param      int dy +ve is down
 */
void move_cursor_relative(int dx, int dy) {
#ifdef _WIN32
  impl::win32_move_cursor_relative(dx, dy);
#else
  impl::nix_move_cursor_relative(dx, dy);
#endif
}

/**
 * @brief      platform indepent console cursor relative movement
 * @details    works on *nix, macOS and Windows
 * @param      int x 0 is top leftmost column
 * @param      int y 0 is top row
 */
void move_cursor_absolute(int x, int y) {
#ifdef _WIN32
  HANDLE console   = GetStdHandle(STD_OUTPUT_HANDLE);
  COORD  cursorPos = {x, y};
  SetConsoleCursorPosition(console, cursorPos);
#else
  // ansi code is 1 based - stick to zero based for consistency
  std::cout << impl::ansi::csi << (y + 1) << ';' << (x + 1) << impl::ansi::cursor_pos;
#endif
}

/**
 * @brief      platform indepent console clear screen
 * @details    clear visible console (not scrollback) and moves home (1, 1)
 */
void clear_screen() {
#ifdef _WIN32
  impl::win32_console_clear_screen();
#else
  std::cout << impl::ansi::csi << impl::ansi::clear_screen;
  std::cout << impl::ansi::csi << impl::ansi::home;
#endif
}

} // namespace xos::console
