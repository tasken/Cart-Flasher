#include <dirent.h>
#include <fat.h>
#include <nds.h>
#include <nds/arm9/dldi.h>
#include <cstdlib>
#include <ctime>
#include "device.h"
#include "ui.h"
#include "menu.h"
#include "nds_platform.h"

using namespace flashcart_core;
using namespace ncgc;


int main(void)
{
	// BlocksDS boosts the ARM9 to 134MHz on DSi by default (devkitARM never did).
	// libncgc's cart-protocol delays are raw cycle-count busy-waits, so doubling the
	// clock halves their real time -- force back to 67MHz to match the old timing.
	setCpuClock(false);

	// Seed once, here. The button-combo prompt used to call srand(time(NULL))
	// itself every time it ran, which reset the RNG instead of advancing it and
	// made the combo a pure function of the clock second -- two prompts in the
	// same second got a byte-identical combo. Seeding once lets each prompt walk
	// the sequence forward, so no two ever match regardless of timing.
	srand(time(NULL));

	sysSetBusOwners(true, true); //Give ARM9 access to the cart
	InitializeScreens();

	// BlocksDS autodetects ARM7-capable DLDI drivers and runs them on the ARM7,
	// which hands Slot-1 to the ARM7 (EXMEMCNT bit 11) -- after that, every
	// ARM9-side cart access in libncgc reads open-bus 0xFFFFFFFF and detection
	// fails. devkitARM always ran DLDI on the ARM9; pin that behavior back.
	dldiSetMode(DLDI_MODE_ARM9);
	if (!fatInitDefault()) {
		DrawString(BOTTOM_SCREEN, FONT_WIDTH, FONT_HEIGHT * 2, COLOR_RED,
			"SD card init failed!\nLogging, backups and writes\nwon't work this session.");
		// Nothing can be logged without a card, so put the probe straight on the
		// screen instead, under the notice. It survives until the cart list draws
		// over it, so it can be read (or photographed) from the boot screen.
		DrawString(BOTTOM_SCREEN, FONT_WIDTH, FONT_HEIGHT * 6, COLOR_GREY, "Hardware probe");
		LogHardwareProbe(8);
	}

	Flashcart *cart = nullptr; //We define our main cart variable right here, and we will pass it along from function to function until the very end

	print_boot_msg();

	menu_lvl1(cart); //menu_lvl1() will later call menu_lvl2()

	return 0;
}
