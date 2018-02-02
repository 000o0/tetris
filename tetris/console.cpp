#include "stdafx.h"
#include "console.hpp"

void console_controller::initialise()
{
	// SET WINDOW SIZE
	auto window_handle = GetConsoleWindow();
	RECT window_rectangle;
	GetWindowRect(window_handle, &window_rectangle);
	MoveWindow(window_handle, window_rectangle.left, window_rectangle.top, 400, 400, true);

	// HIDE NATIVE CURSOR
	CONSOLE_CURSOR_INFO cursor_info{ 1, false };
	SetConsoleCursorInfo(this->console_handle, &cursor_info);
}

void console_controller::set_title(const std::wstring& title)
{
	SetConsoleTitleW(title.c_str());
}

void console_controller::clear()
{
	CONSOLE_SCREEN_BUFFER_INFO buffer;
	if (!GetConsoleScreenBufferInfo(this->console_handle, &buffer))
		return;

	auto null_coords = COORD{ 0,0 };
	auto count = buffer.dwSize.X * buffer.dwSize.Y;

	uint32_t written_count;
	if (!FillConsoleOutputCharacterW(this->console_handle, static_cast<TCHAR>(' '), count, null_coords, reinterpret_cast<LPDWORD>(&written_count)))
		return;
}
void console_controller::clear(const int16_t x, const int16_t y, const int16_t width, const int16_t height)
{
	const std::string clear_buffer(width, ' ');
	for (size_t i = 0; i < height; i++)
		fill_horizontal(x, y + i, ' ', width);
}

void console_controller::draw(const int16_t x, const int16_t y, const uint16_t character)
{
	// SET POSITION AND WRITE
	this->set_position(x, y);
	printf("%lc", character);
}

uint16_t console_controller::read(const int16_t x, const int16_t y)
{
	CHAR_INFO buffer;
	SMALL_RECT rectangle{ x, y, x, y };
	COORD coords{ x, y };
	COORD size{ 1,1 };

	return ReadConsoleOutput(this->console_handle, &buffer, size, coords, &rectangle) ? buffer.Char.UnicodeChar : L' ';
}

void console_controller::fill_horizontal(const int16_t x, const int16_t y, const uint16_t character, const uint16_t count)
{
	CONSOLE_SCREEN_BUFFER_INFO buffer;
	if (!GetConsoleScreenBufferInfo(this->console_handle, &buffer))
		return;

	auto coords = COORD{ x, y };

	uint32_t written_count;
	if (!FillConsoleOutputCharacterW(this->console_handle, character, count, coords, reinterpret_cast<LPDWORD>(&written_count)))
		return;
}

void console_controller::fill_vertical(const int16_t x, const int16_t y, const uint16_t character, const uint16_t count)
{
	for (size_t i = 0; i < count; i++)
		this->draw(x, y + i, character);
}

void console_controller::set_position(const int16_t x, const int16_t y)
{
	const auto coord = COORD{ x, y };
	SetConsoleCursorPosition(this->console_handle, coord);
}

std::pair<int16_t, int16_t> console_controller::get_position()
{
	CONSOLE_SCREEN_BUFFER_INFO  info;
	GetConsoleScreenBufferInfo(this->console_handle, &info);

	return std::make_pair(info.dwCursorPosition.X, info.dwCursorPosition.Y);
}
