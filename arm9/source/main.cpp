#include <dirent.h>
#include <fat.h>
#include <nds.h>
#include <nds/arm9/dldi.h>
#include "device.h"
#include "ui.h"
#include "menu.h"

using namespace flashcart_core;
using namespace ncgc;


int main(void)
{
	// BlocksDS boosts the ARM9 to 134MHz on DSi by default (devkitARM never did).
	// libncgc's cart-protocol delays are raw cycle-count busy-waits, so doubling the
	// clock halves their real time -- force back to 67MHz to match the old timing.
	setCpuClock(false);

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
	}

	Flashcart *cart = nullptr; //We define our main cart variable right here, and we will pass it along from function to function until the very end

	print_boot_msg();

	menu_lvl1(cart); //menu_lvl1() will later call menu_lvl2()

	return 0;
}
