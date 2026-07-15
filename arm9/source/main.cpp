#include <dirent.h>
#include <fat.h>
#include <nds.h>
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
	fatInitDefault();

	Flashcart *cart = nullptr; //We define our main cart variable right here, and we will pass it along from function to function until the very end

	print_boot_msg();

	menu_lvl1(cart); //menu_lvl1() will later call menu_lvl2()

	return 0;
}
