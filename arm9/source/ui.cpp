#include "ui.h"
#include <nds.h>
#include "font.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void InitializeScreens(void) {
	REG_DISPCNT = MODE_3_2D | DISPLAY_BG3_ACTIVE;
	REG_DISPCNT_SUB = MODE_3_2D | DISPLAY_BG3_ACTIVE;

	// Set bg3 on both displays large enough to fill the screen, use 15 bit
	// RGB color values, and look for bitmap data at character base block 1.
	REG_BG3CNT = BgSize_R_256x256 | BG_15BITCOLOR | BG_CBB1;
	REG_BG3CNT_SUB = BgSize_R_256x256 | BG_15BITCOLOR | BG_CBB1;

	// Don't scale bg3 (set its affine transformation matrix to [[1,0],[0,1]])
	REG_BG3PA = 1 << 8;
	REG_BG3PD = 1 << 8;

	REG_BG3PA_SUB = 1 << 8;
	REG_BG3PD_SUB = 1 << 8;
}

void setPixel(u16 *screen, int row, int col, u16 color) {
	screen[row * SCREEN_WIDTH + col] = color | BIT(15);
}

void ClearScreen(u16 *screen, u16 color) {
	int width = SCREEN_HEIGHT * SCREEN_WIDTH;
	for (int i = 0; i < width; i++) {
		screen[i] = color | BIT(15);
	}
}

void DrawRectangle(u16 *screen, int x, int y, int width, int height, u16 color) {
	for (int c = x; c < x + width; c++) {
		for (int r = y; r < y + height; r++) {
			setPixel(screen, r, c, color);
		}
	}
}

void DrawCharacter(u16 *screen, int character, int x, int y, u16 color) {
	for (int yy = 0; yy < FONT_HEIGHT; yy++) {
		uint8_t charPos = font[character * FONT_HEIGHT + yy];
		for (int xx = (8 - FONT_WIDTH); xx <= 7; xx++) {
			if ((charPos >> xx) & 1) {
				// Bit 7 (MSB) is the glyph's leftmost pixel, bit (8-FONT_WIDTH)
				// its rightmost -- offset must be 7-xx (0 for bit 7, up to
				// FONT_WIDTH-1 for bit 8-FONT_WIDTH). `FONT_WIDTH - xx` was off
				// by one, drawing every glyph shifted 1px left of `x` (columns
				// -1..FONT_WIDTH-2 instead of 0..FONT_WIDTH-1) -- present since
				// this project's first commit, never actually visible because
				// the shift is uniform across every pixel of every glyph.
				setPixel(screen, y + yy, x + 7 - xx, color);
			}
		}
	}
}

void DrawString(u16* screen, int x, int y, u16 color, const char *str)
{
	const int startx = x;
	const size_t len = strlen(str);
	for (size_t i = 0; i < len; i++)
	{
		if (str[i] == '\n')
		{
			x = startx;
			y += FONT_HEIGHT;
			continue;
		}
		if ((y + FONT_HEIGHT) > SCREEN_HEIGHT)
		{
			break;
		}

		if ((x + FONT_WIDTH) > SCREEN_WIDTH)
		{
			x = startx;
			y += FONT_HEIGHT;
		}
		DrawCharacter(screen, str[i], x, y, color);
		x += FONT_WIDTH;
	}
}

void DrawHeader(u16* screen, const char *str, int offset)
{
	ClearScreen(screen, COLOR_BLACK);
	DrawRectangle(screen, 0, 0, SCREEN_WIDTH, FONT_HEIGHT, COLOR_BLUE);
	DrawString(screen, offset, 0, COLOR_WHITE, str);
}

// Draws a single selectable list row: a full-width highlight bar behind the
// text when selected, so focus is obvious at a glance instead of relying on
// a text-color change alone. Always redraws the row's background so a
// previously-highlighted row doesn't leave a stray colored bar behind when
// the selection moves.
void DrawListRow(u16 *screen, int y, bool selected, u16 highlightColor, const char *text)
{
	DrawRectangle(screen, 0, y, SCREEN_WIDTH, FONT_HEIGHT, selected ? highlightColor : COLOR_BLACK);
	DrawString(screen, FONT_WIDTH, y, selected ? COLOR_BLACK : COLOR_WHITE, text);
}

// Plain button-hint line, matching every other screen -- replaces a
// permanent 3-row grey status box; version info moved to the boot screen.
// <START> still powers off from this screen (HandlePowerOffShortcut(), called
// from menu_lvl1's loop) -- just not advertised here, unlike the boot splash.
void DrawFooter(int loglevel)
{
	DrawRectangle(TOP_SCREEN, 0, SCREEN_HEIGHT - FONT_HEIGHT, SCREEN_WIDTH, FONT_HEIGHT, COLOR_BLACK);
	const char *loglevel_str;
	//I use a bunch of if statements here because the array that has strings over at ntrboot_flasher's `platform.cpp` is not available here
	if (loglevel == 0) { loglevel_str = "DEBUG"; }
	if (loglevel == 1) { loglevel_str = "INFO"; }
	if (loglevel == 2) { loglevel_str = "NOTICE"; }
	if (loglevel == 3) { loglevel_str = "WARN"; }
	if (loglevel == 4) { loglevel_str = "ERROR"; }
	DrawStringF(TOP_SCREEN, FONT_WIDTH, SCREEN_HEIGHT - FONT_HEIGHT, COLOR_YELLOW, "<A> Select   <Y> Log: %s", loglevel_str);
}

// Horizontally centres a single line. Same arithmetic the DrawHeader callers
// already open-code; single-line only, since a '\n' would throw the width off.
void DrawStringCentered(u16 *screen, int y, u16 color, const char *str)
{
	DrawString(screen, (SCREEN_WIDTH - (int)(strlen(str) * FONT_WIDTH)) / 2, y, color, str);
}

void DrawStringF(u16 *screen, int x, int y, u16 color, const char *format, ...)
{
	char str[256];
	char *p = str;
	va_list va, va_retry;

	va_start(va, format);
	// A va_list is spent after one vsnprintf() call -- reusing it for the
	// retry below without a fresh copy is undefined behavior (silently
	// "worked" on this ARM EABI target since va_list is just a stack
	// pointer, but not guaranteed). va_copy() before the first call gives
	// the overflow path its own valid, unconsumed list.
	va_copy(va_retry, va);
	int w = vsnprintf(str, sizeof(str), format, va);
	va_end(va);
	if (w < 0) {
		// printf failed
		va_end(va_retry);
		return;
	}
	else if ((unsigned int)w > sizeof(str) - 1) {
		// cry now
		// allocate a buffer big enough
		char *m = (char *)malloc(w + 1);
		if (m) {
			vsnprintf(m, w + 1, format, va_retry);
			p = m;
		} // if malloc fails, we just write the truncated string i guess
	}
	va_end(va_retry);

	DrawString(screen, x, y, color, p);
	if (p != str && p) {
		free(p);
	}
}

uint32_t progress_current_override = 0;
uint32_t progress_total_override = 0;

void SetProgressOverride(uint32_t current, uint32_t total)
{
	progress_current_override = current;
	progress_total_override = total;
}

//https://github.com/ntrteam/ntrboot_flasher/blob/master/source/common/ui.cpp#L201
void ShowProgress(u16 *screen, uint32_t current, uint32_t total, const char* status)
{
	// Leaves FONT_WIDTH either side, so the bar and its label line up with the
	// left margin every other screen uses. bar_pos_x and text_pos_x derive from it.
	const uint8_t bar_width = SCREEN_WIDTH - (2 * FONT_WIDTH);
	const uint8_t bar_height = 12;
	const uint16_t bar_pos_x = (SCREEN_WIDTH - bar_width) / 2;
	const uint16_t bar_pos_y = (SCREEN_HEIGHT / 2) - (bar_height / 2);
	const uint16_t text_pos_x = bar_pos_x + (bar_width / 2) - (FONT_WIDTH * 2);
	const uint16_t text_pos_y = bar_pos_y + 1;

	// Apply overrides
	if (progress_total_override) total = progress_total_override;
	current += progress_current_override;
	// Clamp instead of zeroing: an overshooting caller must not rewind the bar.
	if (current > total) current = total;

	static uint32_t last_prog_width = 1;
	static uint32_t last_prog_percent = 101;
	static char last_status[48];
	uint32_t prog_width = (total > 0) ? (current * (bar_width - 4)) / total : 0;
	uint32_t prog_percent = (total > 0) ? (current * 100) / total : 0;

	// Fresh operation: paint the backdrop and bar outline once.
	if (current == 0)
	{
		ClearScreen(screen, STD_COLOR_BG);
		DrawRectangle(screen, bar_pos_x, bar_pos_y, bar_width, bar_height, STD_COLOR_FONT);
		DrawRectangle(screen, bar_pos_x + 1, bar_pos_y + 1, bar_width - 2, bar_height - 2, STD_COLOR_BG);
		last_prog_width = 0;
		last_prog_percent = 101;
		last_status[0] = '\0';
	}

	// Status label: redraw only when the text actually changes. Redrawing it
	// on every call made the label visibly flicker, worst when two callers
	// alternated different strings for the same operation.
	if (status && strncmp(status, last_status, sizeof(last_status) - 1) != 0)
	{
		DrawRectangle(screen, bar_pos_x, bar_pos_y - FONT_HEIGHT - 4, bar_width, FONT_HEIGHT, STD_COLOR_BG);
		DrawString(screen, bar_pos_x, bar_pos_y - FONT_HEIGHT - 4, STD_COLOR_FONT, status);
		strncpy(last_status, status, sizeof(last_status) - 1);
		last_status[sizeof(last_status) - 1] = '\0';
	}

	// Bar and percent: repaint only on change. A shrinking width (multi-pass
	// operations like erase-then-write) repaints just the bar interior,
	// never the whole screen.
	if (prog_width != last_prog_width || prog_percent != last_prog_percent)
	{
		DrawRectangle(screen, bar_pos_x + 1, bar_pos_y + 1, bar_width - 2, bar_height - 2, STD_COLOR_BG); // Clear the progress bar before re-rendering.
		DrawRectangle(screen, bar_pos_x + 2, bar_pos_y + 2, prog_width, bar_height - 4, COLOR_GREEN);
		DrawStringF(screen, text_pos_x, text_pos_y, STD_COLOR_FONT, "%3lu%%", prog_percent);
	}

	last_prog_width = prog_width;
	last_prog_percent = prog_percent;
}