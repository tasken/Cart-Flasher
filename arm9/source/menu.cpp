#include "menu.h"
#include <nds.h>
#include "ui.h"
#include "nds_platform.h"
#include "device.h"
#include "filebrowser.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib> // rand(); the seed itself lives in main()

#define bootmsg "This tool writes directly to your\n" 					\
				"flashcart's memory. If something\n" 					\
				"interrupts a write (power loss,\n" 					\
				"wrong file), your cart could stop\n" 					\
				"working.\n\n" 											\
				"It's nothing to worry about if\n" 					\
				"you're careful: always keep a\n" 						\
				"backup, and read each screen\n" 						\
				"before confirming."

using namespace flashcart_core;
using namespace ncgc;

int global_loglevel = 1; //https://github.com/ntrteam/flashcart_core/blob/master/platform.h#L6 

void print_boot_msg(void)
{
	// Plain black + the same blue header bar every other screen uses, not a
	// full alarm-red screen — this is a heads-up, not a hazard warning.
	char header_title[64];
	sprintf(header_title, "Cart-Flasher %s", CART_FLASHER_VERSION);
	DrawHeader(TOP_SCREEN, header_title, ((SCREEN_WIDTH - (strlen(header_title) * FONT_WIDTH)) / 2));
	DrawString(TOP_SCREEN, FONT_WIDTH * 2, FONT_HEIGHT * 2, COLOR_WHITE, bootmsg);
	// Spacing of 1 empty line after welcome message. A/B instructions at row 13.
	DrawString(TOP_SCREEN, FONT_WIDTH * 2, FONT_HEIGHT * 13, COLOR_YELLOW, "<A> Continue   <B> Power off");
	// Spacing of 2 empty lines after A/B instructions. Credits at row 16.
	DrawStringF(TOP_SCREEN, FONT_WIDTH * 2, FONT_HEIGHT * 16, COLOR_GREY, "Developed by @tasken\nCommit: %s\nOriginal by jason0597 & DS-Homebrew", CART_FLASHER_COMMIT);

	while (true)
	{
		swiWaitForVBlank();
		scanKeys();
		if (keysDown() & KEY_B)
		{
			exit(0);
		}
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

	// Draw the list once up front; the loop below only redraws it when the
	// selection or screen actually changes, instead of every single frame.
	// (Every frame would mean redrawing full-width highlight-bar rectangles
	// with no vsync, which tears visibly on real hardware.)
	for (u32 i = 0; i < flashcart_list_size; i++)
	{
		DrawListRow(TOP_SCREEN, (i + 2) * FONT_HEIGHT, i == menu_sel, COLOR_ACCENT, flashcart_list->at(i)->getName());
	}

	while (true) //This will be our MAIN loop
	{
		swiWaitForVBlank();
		bool reprintFlag = false;

		scanKeys();
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
			// Entering DEBUG snapshots the hardware state into the log, so
			// every debug capture starts with the launch mode, real CPU speed
			// and cart-bus ownership without needing a special build.
			if (global_loglevel == 0) {
				LogHardwareProbe();
			}
		}
		if (keysDown() & KEY_A)
		{
			cart = flashcart_list->at(menu_sel); //Set the cart equal to whatever we had selected from before
			if (isDSiMode() || strcmp(cart->getShortName(), "DSTT") == 0) {
				// init() is __ncgc_must_check. Not fatal on its own -- initialize()
				// below fails too and shows the detection error -- but the reason
				// only exists here, so log it rather than discarding it. Turning on
				// DEBUG then re-trying is the whole diagnostic workflow.
				const Err err = card.init();
				if (err) {
					platform::logMessage(LOG_ERR, "menu: card.init failed: %s", err.desc());
				}
			} else {
				// DS mode, non-DSTT: assume the cart's own menu already took the
				// card through KEY1 and left it in KEY2, so just record that state
				// without redoing the handshake. Launching via nds-bootstrap breaks
				// the assumption -- detection then fails with no KEY2 to inherit.
				card.state(NTRState::Key2);
			}
			if (!cart->initialize(&card)) //If cart initialization fails, do all this and then break to main menu
			{
				DrawString(TOP_SCREEN, FONT_WIDTH, 8 * FONT_HEIGHT, COLOR_RED, "Couldn't detect this flashcart.\nCheck it's inserted firmly, then\npress <B> to go back to the list.");
				while (true) { scanKeys(); if (keysDown() & KEY_B) { DrawHeader(TOP_SCREEN, "Choose your flashcart", ((SCREEN_WIDTH - (strlen("Choose your flashcart") * FONT_WIDTH)) / 2)); DrawFooter(global_loglevel); break; } }
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
	DrawString(TOP_SCREEN, 0, SCREEN_HEIGHT - FONT_HEIGHT, COLOR_GREY, "<A> Select   <B> Back");
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
					DrawString(TOP_SCREEN, 0, SCREEN_HEIGHT - FONT_HEIGHT, COLOR_GREY, "<A> Select   <B> Back");
					dirty = true;
					continue;
				}
				DrawHeader(TOP_SCREEN, cart->getName(), ((SCREEN_WIDTH - (strlen(cart->getName()) * FONT_WIDTH)) / 2));
			}

			// The "<A> Select <B> Back" footer no longer applies once the button-combo
			// prompt takes over input, so make sure that row is blank before showing it.
			DrawRectangle(TOP_SCREEN, 0, SCREEN_HEIGHT - FONT_HEIGHT, SCREEN_WIDTH, FONT_HEIGHT, COLOR_BLACK);
			// Only writing gets the random-combo gate. Reading can't harm the cart:
			// no driver reaches an erase or program command from readFlash, and
			// AK2i's even re-locks the flash before it reads. Gating the safe
			// operation just as heavily would train people to mash through the
			// combo before it shows up on the one that does destroy data.
			bool confirmed;
			if (menu_sel == 0)
			{
				DrawString(TOP_SCREEN, (2 * FONT_WIDTH), (8 * FONT_HEIGHT), COLOR_WHITE,
					"About to read a full backup from\nthis cart.");
				DrawString(TOP_SCREEN, (2 * FONT_WIDTH), (12 * FONT_HEIGHT), COLOR_YELLOW, "<A> Start backup   <B> Cancel");
				confirmed = WaitConfirm();
			}
			else
			{
				DrawString(TOP_SCREEN, (2 * FONT_WIDTH), (8 * FONT_HEIGHT), COLOR_WHITE,
					"This will overwrite the cart's\nflash memory and can't be undone.\nEnter the combo below to confirm:");
				confirmed = d0k3_buttoncombo(10 * FONT_WIDTH, 12 * FONT_HEIGHT);
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
						DrawString(TOP_SCREEN, (2 * FONT_WIDTH), (15 * FONT_HEIGHT), COLOR_RED, "Couldn't access the SD card.\nMake sure it's inserted, then\npress <B> to go back.");
						WaitPress(KEY_B);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						break;

					case FILE_OPEN_FAILED:
						DrawString(TOP_SCREEN, (2 * FONT_WIDTH), (15 * FONT_HEIGHT), COLOR_RED, (menu_sel == 0) ?
							"Couldn't create the backup file.\nCheck the SD card isn't full or\nlocked, then press <B> to go back." :
							"Couldn't open that file.\nIt may have been moved or deleted.\nPress <B> to go back.");
						WaitPress(KEY_B);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						break;

					case FILE_IO_FAILED:
						DrawString(TOP_SCREEN, (2 * FONT_WIDTH), (15 * FONT_HEIGHT), COLOR_RED, (menu_sel == 0) ?
							"Something went wrong writing the\nbackup file. Check the SD card has\nfree space, then press <B> to go back." :
							"Something went wrong reading that\nfile, it may be damaged, or the\nSD card's loose. Press <B> to go back.");
						WaitPress(KEY_B);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						break;

					case FLASH_OP_FAILED:
						DrawString(TOP_SCREEN, (2 * FONT_WIDTH), (15 * FONT_HEIGHT), COLOR_RED, (menu_sel == 0) ?
							"Reading from the cart failed\npartway through. Try reseating it.\nPress <B> to return to the menu." :
							"Writing to the cart failed\npartway through. Try reseating it.\nPress <B> to return to the menu.");
						WaitPress(KEY_B);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						break;

					case MEM_ALLOC_FAILED:
						DrawString(TOP_SCREEN, (2 * FONT_WIDTH), (15 * FONT_HEIGHT), COLOR_RED,
							"Not enough free console memory\nto buffer the firmware. Try a\nsmaller backup file, press <B>.");
						WaitPress(KEY_B);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						break;

					case ALL_OK:
						DrawString(TOP_SCREEN, (1 * FONT_WIDTH), (15 * FONT_HEIGHT), COLOR_GREEN, (menu_sel == 0) ?
							"Backup complete! Your dump was\nsaved. Press <A> to continue." :
							"All done! Your flash was written\nsuccessfully. Press <A> to continue.");
						WaitPress(KEY_A);
						ClearScreen(TOP_SCREEN, COLOR_BLACK);
						ClearScreen(BOTTOM_SCREEN, COLOR_BLACK);
						break;
					}
			}
			// No "nothing was touched" screen on the way out: <B> is the cancel
			// key on both prompts, so the user already knows they backed out.
			// A mistyped combo says its piece in the retry loop above.
			break;
		}
	}
}

//Will print out a gm9/sb9si like "input button combo to continue" prompt, also does the button checking for it
//The RNG is seeded once in main(), not here

const char rancombo_symbols[5] = { '\x1B', '\x18', '\x1A', '\x19', 'A' }; // Left, Up, Right, Down
const u32 rancombo_inputs[5] = { KEY_LEFT, KEY_UP, KEY_RIGHT, KEY_DOWN, KEY_A };

bool d0k3_buttoncombo(int cur_c, int cur_r)
{
	// Seeded once in main(), never here: re-seeding per call from time(NULL) tied
	// the combo to the clock second instead of advancing the sequence.
	//
	// No symbol may repeat back-to-back, same as GodMode9 (ui.c: `while (lsh ==
	// lastlsh) lsh = (PRNG & 0x3)`). A doubled arrow is easy to misread as one,
	// and it keeps the double-tap tolerance below unambiguous.
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
	int depth = 0; //How far in we are, how many buttons have been pressed so far, how *deep* we are

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
				// Re-pressing the button that was just accepted is a double-tap,
				// not a mistake, so don't punish it. GodMode9 forgives the same
				// case (ui.c: `!(pad_state & sequence[lvl-1])`). Unambiguous
				// because no symbol repeats back-to-back, and it only suppresses
				// a reset -- it never advances the combo.
			}
			else {
				// Say what went wrong instead of silently rewinding to depth 0 --
				// GodMode9 resets in place, but a combo that quietly restarts
				// looks identical to one that isn't registering presses at all.
				// The combo itself is NOT re-rolled: it was generated once above,
				// so a retry redraws the same glyphs in place and asks the user
				// for attention, not for memory.
				//
				// The message goes two rows under the combo so it keeps its blank
				// separator wherever the caller placed it.
				const int msg_r = cur_r + (2 * FONT_HEIGHT);
				DrawString(TOP_SCREEN, (2 * FONT_WIDTH), msg_r, COLOR_RED, "Wrong key combo. <A> Retry <B> Cancel");
				if (!WaitConfirm()) { return false; }
				DrawRectangle(TOP_SCREEN, 0, msg_r, SCREEN_WIDTH, FONT_HEIGHT, COLOR_BLACK);
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
