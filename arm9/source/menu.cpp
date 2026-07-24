#include "menu.h"
#include <nds.h>
#include "ui.h"
#include "nds_platform.h"
#include "device.h"
#include "filebrowser.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib> // rand(); the seed itself lives in main()

// Wording follows Sanras's flashcart guide, which quotes this tool verbatim
// ("flashrom", "key combo", "Back up flash", "Write flash") -- don't reword.
#define bootmsg "This tool writes directly to your\n" 					\
				"flashcart's flashrom. A bad write\n" 					\
				"can brick your cart, so always keep\n" 				\
				"a backup first.\n\n" 									\
				"Not every cart has been tested. If\n" 					\
				"you can't dump your cart's flashrom,\n" 				\
				"or the dump is nonsense, STOP and\n" 					\
				"open a GitHub issue."

using namespace flashcart_core;
using namespace ncgc;

int global_loglevel = 1; //https://github.com/ntrteam/flashcart_core/blob/master/platform.h#L6

// <START> power-off shortcut, checked once per frame from the boot splash
// and the cart list -- deliberately not everywhere in the app (see the
// switch-flash confirm/combo screens, which don't offer it). GodMode9i's own
// startMenu() was checked as precedent for the *action* itself: it offers
// exactly "Power off" (systemShutDown()) and a DSi-specific "Reboot", never
// a generic "return to loader" action. That's deliberate, not an oversight
// -- libnds's exit(0) can hang on flashcart menus (confirmed with AKMenu)
// that don't properly support or clear its loader-return protocol, so this
// app doesn't offer that path anywhere either. Never returns if <START> was
// pressed.
void HandlePowerOffShortcut(void)
{
	if (keysDown() & KEY_START)
	{
		systemShutDown();
		while (true) { swiWaitForVBlank(); }
	}
}

void print_boot_msg(void)
{
	// Plain black + the same blue header bar every other screen uses, not a
	// full alarm-red screen — this is a heads-up, not a hazard warning.
	char header_title[64];
	sprintf(header_title, "Cart-Flasher %s", CART_FLASHER_VERSION);
	DrawHeader(TOP_SCREEN, header_title, ((SCREEN_WIDTH - (strlen(header_title) * FONT_WIDTH)) / 2));
	DrawString(TOP_SCREEN, FONT_WIDTH, FONT_HEIGHT * 2, COLOR_WHITE, bootmsg);
	DrawString(TOP_SCREEN, FONT_WIDTH, FONT_HEIGHT * 13, COLOR_YELLOW, "<A> Continue   <START> Power off");
	DrawStringF(TOP_SCREEN, FONT_WIDTH, FONT_HEIGHT * 16, COLOR_GREY, "Developed by @tasken\n%s build - Commit: %s\nBased on work by jason0597 & DS-Homebrew", CART_FLASHER_BUILD_KIND, CART_FLASHER_COMMIT);

	while (true)
	{
		swiWaitForVBlank();
		scanKeys();
		HandlePowerOffShortcut();
		if (keysDown() & KEY_A)
		{
			break;
		}
	}
}

void WaitPress(u32 KEY) {
	while (true) { swiWaitForVBlank(); scanKeys(); if (keysDown() & KEY) { break; } }
}

// <A> to go ahead, <B> to back out. WaitPress() only ever waits for one key, so
// it can't express a choice.
static bool WaitConfirm(void) {
	while (true) {
		swiWaitForVBlank();
		scanKeys();
		if (keysDown() & KEY_A) { return true; }
		if (keysDown() & KEY_B) { return false; }
	}
}

bool ntrCardReset()
{
	if (isDSiMode())
	{
		// Reset card slot
		disableSlot1();
		for(int i = 0; i < 25; i++) { swiWaitForVBlank(); }
		enableSlot1();
		for(int i = 0; i < 15; i++) { swiWaitForVBlank(); }
	}
	else
	{
		REG_ROMCTRL = 0;
		REG_AUXSPICNT = 0;
		for (int i = 0; i < 25; i++) swiWaitForVBlank();
		REG_AUXSPICNT = CARD_CR1_ENABLE | CARD_CR1_IRQ;
		REG_ROMCTRL = CARD_nRESET | CARD_SEC_SEED;
		while (REG_ROMCTRL & CARD_BUSY) ;
		cardReset();
		while (REG_ROMCTRL & CARD_BUSY) ;
	}
	return true;
}

void menu_lvl1(Flashcart* cart)
{
	// Remove R4iSDHC.hk if not in DSi mode
	if (!isDSiMode()) {
		for (auto it = flashcart_list->begin(); it != flashcart_list->end(); ) {
			if (strcmp((*it)->getShortName(), "R4iSDHC.hk") == 0) {
				it = flashcart_list->erase(it);
			} else {
				++it;
			}
		}
	}

	// Sort alphabetically by name
	std::sort(flashcart_list->begin(), flashcart_list->end(), [](Flashcart* a, Flashcart* b) {
		return strcasecmp(a->getName(), b->getName()) < 0;
	});

	u32 menu_sel = 0;
	
	NTRCard card(ntrCardReset);
	DrawHeader(TOP_SCREEN, "Choose your flashcart", ((SCREEN_WIDTH - (strlen("Choose your flashcart") * FONT_WIDTH)) / 2));
	DrawFooter(global_loglevel);
	DrawHeader(BOTTOM_SCREEN, "Flashcart info", ((SCREEN_WIDTH - (14 * FONT_WIDTH)) / 2));
	DrawStringF(BOTTOM_SCREEN, FONT_WIDTH, FONT_HEIGHT * 2, COLOR_WHITE, "%s\n\n%s", flashcart_list->at(0)->getAuthor(), flashcart_list->at(0)->getDescription());
	u32 flashcart_list_size = flashcart_list->size();

	// Redraws only on change, not every frame -- full-width highlight bars
	// redrawn every frame with no vsync tear visibly on hardware.
	for (u32 i = 0; i < flashcart_list_size; i++)
	{
		DrawListRow(TOP_SCREEN, (i + 2) * FONT_HEIGHT, i == menu_sel, COLOR_ACCENT, flashcart_list->at(i)->getName());
	}

	while (true) //This will be our MAIN loop
	{
		swiWaitForVBlank();
		bool reprintFlag = false;

		scanKeys();
		HandlePowerOffShortcut();
		if (keysDown() & KEY_DOWN && menu_sel < (flashcart_list_size - 1))
		{
			menu_sel++;
			reprintFlag = true;
		}
		if (keysDown() & KEY_UP   && menu_sel > 0)
		{
			menu_sel--;
			reprintFlag = true;
		}
		if (keysDown() & KEY_Y) {
			if (global_loglevel == 4) {
				global_loglevel = 0; //if you scroll past the end it puts you back at the top
			}
			else {
				global_loglevel++;
			}
			DrawFooter(global_loglevel);
			// Entering DEBUG snapshots hardware state (launch mode, CPU speed,
			// cart-bus ownership) to the log and screen -- the screen copy is
			// all that exists if the SD card itself is what failed.
			if (global_loglevel == 0) {
				// Prompt goes on the top footer row like every other screen.
				// Blanking first is required -- the footer it replaces is
				// longer, so drawing over it would leave a stale tail.
				DrawRectangle(TOP_SCREEN, 0, SCREEN_HEIGHT - FONT_HEIGHT, SCREEN_WIDTH, FONT_HEIGHT, COLOR_BLACK);
				DrawString(TOP_SCREEN, FONT_WIDTH, SCREEN_HEIGHT - FONT_HEIGHT, COLOR_YELLOW, "<B> Back to the cart list");
				DrawHeader(BOTTOM_SCREEN, "Hardware probe", ((SCREEN_WIDTH - (strlen("Hardware probe") * FONT_WIDTH)) / 2));
				LogHardwareProbe(2);
				WaitPress(KEY_B);
				DrawFooter(global_loglevel);
				reprintFlag = true; // redraws the flashcart info the probe covered
			}
		}
		if (keysDown() & KEY_A)
		{
			cart = flashcart_list->at(menu_sel); //Set the cart equal to whatever we had selected from before

			// One row past the last cart -- fixed would sit flush against the
			// list in DSi mode, since R4iSDHC.hk is hidden outside it. Also
			// reused by the detection-error message below, which overwrites
			// this same row once it's known whether detection succeeded.
			const int errorRow = flashcart_list_size + 3;

			// card.init()/cart->initialize() below do real SPI/cart-bus probing,
			// which can take a noticeable moment -- without this, the screen just
			// sits on the list with no sign the button press registered, which
			// reads as a freeze rather than a wait. White, in the content area:
			// matches every other in-progress status line in this app (e.g.
			// DumpFlash/WriteFlash's "Backing up.../Writing to..."). Yellow is
			// reserved for button prompts everywhere else, never plain status.
			DrawStringF(TOP_SCREEN, FONT_WIDTH, errorRow * FONT_HEIGHT, COLOR_WHITE, "Detecting %s...", cart->getName());

			if (isDSiMode() || strcmp(cart->getShortName(), "DSTT") == 0) {
				// __ncgc_must_check. Not fatal here -- initialize() below fails
				// too and shows the detection error -- but the reason only
				// exists here, so log it instead of discarding it.
				const Err err = card.init();
				if (err) {
					platform::logMessage(LOG_ERR, "menu: card.init failed: %s", err.desc());
				}
			} else {
				// DS mode, non-DSTT: assume the cart's own menu already took the
				// card through KEY1 into KEY2, so just record that instead of
				// redoing the handshake. Breaks under nds-bootstrap -- no KEY2
				// to inherit there.
				card.state(NTRState::Key2);
			}
			if (!cart->initialize(&card)) //If cart initialization fails, do all this and then break to main menu
			{
				// Overwrites the "Detecting..." line above -- blanked first since
				// the error's own first line isn't guaranteed longer than every
				// possible "Detecting <cart name>..." line.
				DrawRectangle(TOP_SCREEN, 0, errorRow * FONT_HEIGHT, SCREEN_WIDTH, FONT_HEIGHT, COLOR_BLACK);
				// Message and "press <B>" instruction combined into one red
				// string, matching every error case in menu_lvl2's switch below.
				DrawString(TOP_SCREEN, FONT_WIDTH, errorRow * FONT_HEIGHT, COLOR_RED, "Couldn't detect this flashcart.\nCheck it's inserted firmly, then\npress <B> to go back.");
				WaitPress(KEY_B);
				ClearScreen(TOP_SCREEN, COLOR_BLACK);
				DrawHeader(TOP_SCREEN, "Choose your flashcart", ((SCREEN_WIDTH - (strlen("Choose your flashcart") * FONT_WIDTH)) / 2));
				DrawFooter(global_loglevel);
				reprintFlag = true;
			}
			else
			{
				menu_lvl2(cart); //There is a while loop over at menu_lvl2(), the statements underneath won't get executed immediately
				DrawHeader(TOP_SCREEN, "Choose your flashcart", ((SCREEN_WIDTH - (strlen("Choose your flashcart") * FONT_WIDTH)) / 2));
				DrawFooter(global_loglevel);
				reprintFlag = true;
			}
		}

		if (reprintFlag)
		{
			for (u32 i = 0; i < flashcart_list_size; i++)
			{
				DrawListRow(TOP_SCREEN, (i + 2) * FONT_HEIGHT, i == menu_sel, COLOR_ACCENT, flashcart_list->at(i)->getName());
			}
			cart = flashcart_list->at(menu_sel);
			DrawHeader(BOTTOM_SCREEN, "Flashcart info", ((SCREEN_WIDTH - (14 * FONT_WIDTH)) / 2));
			DrawStringF(BOTTOM_SCREEN, FONT_WIDTH, FONT_HEIGHT * 2, COLOR_WHITE, "%s\n\n%s", cart->getAuthor(), cart->getDescription());
		}
	}
}

void menu_lvl2(Flashcart* cart)
{
	DrawHeader(TOP_SCREEN, cart->getName(), ((SCREEN_WIDTH - (strlen(cart->getName()) * FONT_WIDTH)) / 2));
	DrawString(TOP_SCREEN, FONT_WIDTH, SCREEN_HEIGHT - FONT_HEIGHT, COLOR_YELLOW, "<A> Select   <B> Back");
	int menu_sel = 0;
	bool dirty = true;

	while (true)
	{
		swiWaitForVBlank();
		// Only redraw the highlight-bar rows when the selection (or the screen
		// underneath them) actually changed, not every single frame — redrawing
		// full-width rectangles unconditionally with no vsync tears visibly.
		if (dirty) {
			DrawListRow(TOP_SCREEN, 2 * FONT_HEIGHT, menu_sel == 0, COLOR_ACCENT, "Back up flash");	//0
			DrawListRow(TOP_SCREEN, 3 * FONT_HEIGHT, menu_sel == 1, COLOR_TINTEDRED, "Write flash");	//1
			dirty = false;
		}

		scanKeys();

		if (keysDown() & KEY_DOWN && menu_sel < 1)
		{
			menu_sel++;
			dirty = true;
		}
		if (keysDown() & KEY_UP   && menu_sel > 0)
		{
			menu_sel--;
			dirty = true;
		}
		if (keysDown() & KEY_B)
		{
			break;
		}
		int ntrboot_return = 0;

		if (keysDown() & KEY_A)
		{
			char writePath[512];
			if (menu_sel == 1) {
				if (!BrowseForFile("/cart-backups", ".bin", writePath, sizeof(writePath))) {
					DrawHeader(TOP_SCREEN, cart->getName(), ((SCREEN_WIDTH - (strlen(cart->getName()) * FONT_WIDTH)) / 2));
					DrawString(TOP_SCREEN, FONT_WIDTH, SCREEN_HEIGHT - FONT_HEIGHT, COLOR_YELLOW, "<A> Select   <B> Back");
					dirty = true;
					continue;
				}
				// No DrawHeader here: the confirm below redraws it anyway, and
				// clears the file browser off the screen while it's at it.
			}

			// Confirm takes the whole screen -- DrawHeader clears the menu
			// behind it (footer included) for free. Both paths are 10 rows
			// centred at row 5; the row numbers below follow from that.
			//
			// Only writing is gated behind the combo: reading can't damage the
			// cart (no driver reaches erase/program from readFlash), and
			// gating it the same way would train people to mash through the
			// combo before the destructive path.
			DrawHeader(TOP_SCREEN, cart->getName(), ((SCREEN_WIDTH - (strlen(cart->getName()) * FONT_WIDTH)) / 2));
			const int WARN_X = 4 * FONT_WIDTH;
			bool confirmed;
			if (menu_sel == 0)
			{
				DrawString(TOP_SCREEN, WARN_X, (5 * FONT_HEIGHT), COLOR_WHITE,
					"Dumping this cart's flashrom to\n/cart-backups on your SD card.\n\nNothing is written to the cart.\n\nIf it fails, or the dump is\nnonsense, STOP and open a GitHub\nissue.");
				DrawStringCentered(TOP_SCREEN, (14 * FONT_HEIGHT), COLOR_YELLOW, "<A> Start backup   <B> Cancel");
				confirmed = WaitConfirm();
			}
			else
			{
				// Banner/icon note is write-only: restoring an untouched dump
				// leaves the banner byte-identical, so it can't break stock
				// DSi/3DS loading.
				DrawString(TOP_SCREEN, WARN_X, (5 * FONT_HEIGHT), COLOR_WHITE,
					"This overwrites the cart's flashrom\nand can't be undone.\n\nA changed icon or banner is blocked\nby stock DSi/3DS firmware unless CFW\nis installed. NDS/DS Lite are fine.");
				DrawStringCentered(TOP_SCREEN, (12 * FONT_HEIGHT), COLOR_YELLOW, "Enter the key combo to confirm:");
				confirmed = d0k3_buttoncombo(14 * FONT_HEIGHT);
			}

			if (confirmed)
			{
				ClearScreen(BOTTOM_SCREEN, COLOR_BLACK);
				if (menu_sel == 0) {
					ntrboot_return = DumpFlash(cart);
				} else if (menu_sel == 1) {
					ntrboot_return = WriteFlash(cart, writePath);
				}

				switch (ntrboot_return) {
					case FAT_MOUNT_FAILED:
						DrawString(TOP_SCREEN, FONT_WIDTH, (15 * FONT_HEIGHT), COLOR_RED, "Couldn't access the SD card.\nMake sure it's inserted, then\npress <B> to go back.");
						WaitPress(KEY_B);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						break;

					case FILE_OPEN_FAILED:
						DrawString(TOP_SCREEN, FONT_WIDTH, (15 * FONT_HEIGHT), COLOR_RED, (menu_sel == 0) ?
							"Couldn't create the backup file.\nCheck the SD card isn't full or\nlocked, then press <B> to go back." :
							"Couldn't open that file.\nIt may have been moved or deleted.\nPress <B> to go back.");
						WaitPress(KEY_B);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						break;

					case FILE_IO_FAILED:
						DrawString(TOP_SCREEN, FONT_WIDTH, (15 * FONT_HEIGHT), COLOR_RED, (menu_sel == 0) ?
							"Something went wrong writing the\nbackup file. Check the SD card has\nfree space, then press <B> to go back." :
							"Something went wrong reading that\nfile, it may be damaged, or the\nSD card's loose. Press <B> to go back.");
						WaitPress(KEY_B);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						break;

					case FLASH_OP_FAILED:
						DrawString(TOP_SCREEN, FONT_WIDTH, (15 * FONT_HEIGHT), COLOR_RED, (menu_sel == 0) ?
							"Reading from the cart failed\npartway through. Try reseating it.\nPress <B> to return to the menu." :
							"Writing to the cart failed\npartway through. Try reseating it.\nPress <B> to return to the menu.");
						WaitPress(KEY_B);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						break;

					case MEM_ALLOC_FAILED:
						DrawString(TOP_SCREEN, FONT_WIDTH, (15 * FONT_HEIGHT), COLOR_RED,
							"Not enough free console memory\nto buffer the firmware. Try a\nsmaller backup file, press <B>.");
						WaitPress(KEY_B);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						break;

					case ALL_OK:
						DrawString(TOP_SCREEN, FONT_WIDTH, (15 * FONT_HEIGHT), COLOR_GREEN, (menu_sel == 0) ?
							"Backup complete! Your dump was\nsaved. Press <A> to continue." :
							"All done! Your flash was written\nsuccessfully. Press <A> to continue.");
						WaitPress(KEY_A);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						ClearScreen(BOTTOM_SCREEN, COLOR_BLACK);
						break;
					}
				// A completed operation (whatever the result) always returns
				// to the cart list, not just this cart's own menu -- matches
				// the ALL_OK/error screens above, which all wait for a
				// keypress then fall through to here.
				break;
			}
			// Cancelled at the confirm/combo screen -- same landing spot as
			// cancelling the file browser above: back to this cart's own
			// Back up/Write flash list, not all the way out to the cart
			// list. No separate "nothing was touched" screen: <B> already
			// means cancel on both prompts.
			DrawHeader(TOP_SCREEN, cart->getName(), ((SCREEN_WIDTH - (strlen(cart->getName()) * FONT_WIDTH)) / 2));
			DrawString(TOP_SCREEN, FONT_WIDTH, SCREEN_HEIGHT - FONT_HEIGHT, COLOR_YELLOW, "<A> Select   <B> Back");
			dirty = true;
			continue;
		}
	}
}

// Prints a GodMode9-style "input this combo" prompt and checks the presses.
const char rancombo_symbols[5] = { '\x1B', '\x18', '\x1A', '\x19', 'A' }; // Left, Up, Right, Down
const u32 rancombo_inputs[5] = { KEY_LEFT, KEY_UP, KEY_RIGHT, KEY_DOWN, KEY_A };

bool d0k3_buttoncombo(int cur_r)
{
	// Always 5 slots wide, so it centres itself instead of making callers work
	// the column out; the last slot has no trailing gap.
	const int combo_width = (4 * (4 * FONT_WIDTH)) + (3 * FONT_WIDTH);
	const int cur_c = (SCREEN_WIDTH - combo_width) / 2;

	// Seeded once in main(), not here -- re-seeding per call from time(NULL)
	// tied the combo to the clock second instead of advancing it.
	//
	// No symbol repeats back-to-back, matching GodMode9 (ui.c: `while (lsh ==
	// lastlsh) lsh = (PRNG & 0x3)`) -- doubled arrows misread as one, and this
	// keeps the double-tap tolerance below unambiguous.
	int num_rancombo[5] = { 0, 0, 0, 0, 4 }; // zero based, '4' is the 5th item (A)
	int last_symbol = -1;
	for (int i = 0; i < 4; i++) {
		int symbol = last_symbol;
		while (symbol == last_symbol) { symbol = rand() % 4; }
		num_rancombo[i] = symbol;
		last_symbol = symbol;
	}
	char print_rancombo[5] = { ' ', ' ', ' ', ' ', ' ' };
	u32 check_rancombo[5] = { 0, 0, 0, 0, 0 };
	for (int i = 0; i < 5; i++) {
		print_rancombo[i] = rancombo_symbols[num_rancombo[i]];
	}
	for (int i = 0; i < 5; i++) {
		check_rancombo[i] = rancombo_inputs[num_rancombo[i]];
	}
	int depth = 0; // combo progress, 0-based

	while (true) {
		int temp_c = cur_c;
		u16 cur_color = COLOR_GREEN;
		for (int i = 0; i < 5; i++) {
			if (i >= depth) { cur_color = COLOR_WHITE; }
			d0k3_buttoncombo_print_chars(temp_c, cur_r, cur_color, print_rancombo[i]);
			temp_c += 4 * FONT_WIDTH; //3 for our printout ('<', 'arrow', '>'), and one for the space that follows it
		}

		scanKeys();
		if (keysDown()) {
			if (keysDown() & check_rancombo[depth]) {
				depth++;
			}
			else if (keysDown() & KEY_B) {
				return false;
			}
			else if (depth > 0 && (keysDown() & check_rancombo[depth - 1])) {
				// Double-tap forgiveness, matching GodMode9 (ui.c: `!(pad_state &
				// sequence[lvl-1])`). Safe since no symbol repeats back-to-back;
				// this only suppresses a reset, it never advances the combo.
			}
			else {
				// Says what went wrong instead of silently rewinding -- a silent
				// reset looks identical to presses not registering. The combo
				// isn't re-rolled (generated once above), so a retry redraws the
				// same glyphs.
				//
				// Two rows under the combo, keeping its blank separator. Red
				// states the problem, yellow offers the choice, matching every
				// other screen's split.
				const int msg_r = cur_r + (2 * FONT_HEIGHT);
				DrawStringCentered(TOP_SCREEN, msg_r, COLOR_RED, "Wrong key combo, nothing was touched.");
				DrawStringCentered(TOP_SCREEN, msg_r + FONT_HEIGHT, COLOR_YELLOW, "<A> Retry   <B> Cancel");
				if (!WaitConfirm()) { return false; }
				DrawRectangle(TOP_SCREEN, 0, msg_r, SCREEN_WIDTH, 2 * FONT_HEIGHT, COLOR_BLACK);
				depth = 0;
			}
		}

		// this is sorta hacky but otherwise the A button doesnt go green
		if (depth == 5) {
			depth++;
		}
		else if (depth == 6) {
			return true;
		}
	}
}

void d0k3_buttoncombo_print_chars(int collumn, int row, u16 color, char character)
{
	DrawCharacter(TOP_SCREEN, '<', collumn, row, color);
	collumn += FONT_WIDTH;
	DrawCharacter(TOP_SCREEN, character, collumn, row, color);
	collumn += FONT_WIDTH;
	DrawCharacter(TOP_SCREEN, '>', collumn, row, color);
}
