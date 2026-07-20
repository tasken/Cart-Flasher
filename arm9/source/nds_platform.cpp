#include "nds_platform.h"
#include <nds.h>
#include <nds/arm9/dldi.h> // dldiGetMode(), io_dldi_data -- not pulled in by nds.h
#include <fat.h>
#include "device.h"
#include "blowfish_ntr_bin.h"
#include "blowfish_dev_bin.h"
#include "blowfish_retail_bin.h"
#include "ui.h"
#include <cstdio>
#include <cstdarg>
#include <sys/stat.h>
#include <unistd.h>
#include <new>

int progressCount = 0;

bool file_exists(const char* filename) {
	return access(filename, F_OK) == 0;
}

return_codes_t mount_fat(void) {
	if(!file_exists("sd:/") && !file_exists("fat:/")) {
		if (fatInitDefault()) {
			mkdir("/cart-backups", 0777);
			return ALL_OK;
		}
		return FAT_MOUNT_FAILED;
	}
	mkdir("/cart-backups", 0777);
	return ALL_OK;
}

return_codes_t unmount_fat(void) {
	return ALL_OK;
}

namespace flashcart_core {
	namespace platform {
		void showProgress(std::uint32_t current, std::uint32_t total, const char *status) {
			if (progressCount < 1000) {
				progressCount++;
				return;
			}
			else {
				progressCount = 0;
			}
			ShowProgress(BOTTOM_SCREEN, current, total, status);
		}

		int logMessage(log_priority priority, const char *fmt, ...)
		{
			if (priority < global_loglevel) { return 0; }

			// FAT stays mounted for the program's lifetime (main() already
			// mounted it, unmount_fat() is a no-op) -- avoids redundant
			// access()+mkdir() on every log call.
			static bool mounted = false;
			if (!mounted) {
				if (mount_fat() != ALL_OK) { return -1; }
				mounted = true;
			}

			// Must close on every call: on BlocksDS's FatFs, a file's size is
			// only committed by fclose() -- fflush()/fsync() didn't do it in
			// testing. Keeping the file open (tried first) left the log's
			// reported size at 0 bytes forever.
			static bool first_open = true;
			FILE *logfile = fopen("/cart_flasher.log", first_open ? "w" : "a");
			if (!logfile) { return -1; }
			first_open = false;

			va_list args;
			va_start(args, fmt);

			const char *priority_str;
			//I use a bunch of if statements here because the array that has strings over at ntrboot_flasher's `platform.cpp` is not available here
			if (priority == 0) { priority_str = "DEBUG"; }
			if (priority == 1) { priority_str = "INFO"; }
			if (priority == 2) { priority_str = "NOTICE"; }
			if (priority == 3) { priority_str = "WARN"; }
			if (priority == 4) { priority_str = "ERROR"; }
			if (priority >= 5) { priority_str = "?!#$"; }

			char string_to_write[100]; //just do 100, should be enough for any kind of log message we get...
			sprintf(string_to_write, "[%s]: %s\n", priority_str, fmt);

			int result = vfprintf(logfile, string_to_write, args);
			fclose(logfile);
			va_end(args);

			return result;
		}

		auto getBlowfishKey(BlowfishKey key) -> const std::uint8_t(&)[0x1048]
		{
			switch (key) {
				default:
				case BlowfishKey::NTR:
					return *static_cast<const std::uint8_t(*)[0x1048]>(static_cast<const void *>(blowfish_ntr_bin));
				case BlowfishKey::B9Retail:
					return *static_cast<const std::uint8_t(*)[0x1048]>(static_cast<const void *>(blowfish_retail_bin));
				case BlowfishKey::B9Dev:
					return *static_cast<const std::uint8_t(*)[0x1048]>(static_cast<const void *>(blowfish_dev_bin));
			}
		}
	}
}

// Hardware-state probe, logged when the log level switches to DEBUG. The
// timer measurement is CPU-speed-independent: it ticks at the fixed 33.5MHz
// bus clock, so ~8200 ticks means 67MHz, ~4100 means 134MHz, regardless of
// whether SCFG is readable. EXMEMCNT bit 11 set means the ARM7 owns Slot-1,
// the BlocksDS DLDI-on-ARM7 failure mode that broke DS-mode detection.
//
// Goes to the log *and* the screen: a cart that won't detect leaves a
// readable log, but an SD card that won't mount leaves none at all.
void LogHardwareProbe(int firstRow)
{
	TIMER0_CR = 0;
	TIMER0_DATA = 0;
	TIMER0_CR = TIMER_ENABLE | TIMER_DIV_256;
	ncgc::delay(0x200000);
	TIMER0_CR = 0;
	u16 ticks = TIMER0_DATA;

	// EXMEMCNT bit 11 is spelled out rather than left as hex: it decides who owns
	// Slot-1, and it has explained every cart-detection failure so far.
	const bool arm9OwnsCart = !(REG_EXMEMCNT & 0x800);
	// DLDI: we force ARM9 so the ARM7 can't take Slot-1 out from under libncgc,
	// but that only works if the driver tolerates it. If the SD won't mount, the
	// mode and the ARM7_CAPABLE flag are the first things to look at.
	const DLDI_MODE mode = dldiGetMode();
	const bool arm7Capable = io_dldi_data
		&& (io_dldi_data->ioInterface.features & FEATURE_ARM7_CAPABLE);
	const char *const dldiModeStr =
		mode == DLDI_MODE_ARM9 ? "ARM9" : mode == DLDI_MODE_ARM7 ? "ARM7" : "auto";
	const char *const dldiName = io_dldi_data ? io_dldi_data->friendlyName : "(none)";

	// The log carries every field the screen shows, but packed for a log: dense,
	// one grep-friendly line per group, no colours. logMessage() needs a mounted
	// SD, so when the mount is what failed only the screen below will have it.
	flashcart_core::platform::logMessage(flashcart_core::LOG_DEBUG,
		"probe: dsi=%d SCFG_CLK=0x%04X SCFG_EXT=0x%08lX delayTicks=%u",
		isDSiMode(), REG_SCFG_CLK, (unsigned long)REG_SCFG_EXT, ticks);
	flashcart_core::platform::logMessage(flashcart_core::LOG_DEBUG,
		"probe: EXMEMCNT=0x%04X ROMCTRL=0x%08lX AUXSPICNT=0x%04X cart=%s",
		REG_EXMEMCNT, (unsigned long)REG_ROMCTRL, REG_AUXSPICNT,
		arm9OwnsCart ? "ARM9" : "ARM7");
	flashcart_core::platform::logMessage(flashcart_core::LOG_DEBUG,
		"probe: DLDI=%s arm7capable=%d name=%s", dldiModeStr, arm7Capable, dldiName);

	// The screen keeps its own layout: one field per line, colour-coded, spaced
	// for glancing at -- photograph it when there is no SD to log to.
	DrawStringF(BOTTOM_SCREEN, FONT_WIDTH, (firstRow + 0) * FONT_HEIGHT, COLOR_WHITE,
		"dsi=%d  clk=0x%04X  ticks=%u", isDSiMode(), REG_SCFG_CLK, ticks);
	DrawStringF(BOTTOM_SCREEN, FONT_WIDTH, (firstRow + 1) * FONT_HEIGHT, COLOR_WHITE,
		"SCFG_EXT=0x%08lX", (unsigned long)REG_SCFG_EXT);
	DrawStringF(BOTTOM_SCREEN, FONT_WIDTH, (firstRow + 2) * FONT_HEIGHT, arm9OwnsCart ? COLOR_GREEN : COLOR_RED,
		"EXMEMCNT=0x%04X  cart: %s", REG_EXMEMCNT, arm9OwnsCart ? "ARM9" : "ARM7");
	DrawStringF(BOTTOM_SCREEN, FONT_WIDTH, (firstRow + 3) * FONT_HEIGHT, COLOR_WHITE,
		"ROMCTRL=0x%08lX", (unsigned long)REG_ROMCTRL);
	DrawStringF(BOTTOM_SCREEN, FONT_WIDTH, (firstRow + 4) * FONT_HEIGHT, COLOR_WHITE,
		"AUXSPICNT=0x%04X", REG_AUXSPICNT);
	DrawStringF(BOTTOM_SCREEN, FONT_WIDTH, (firstRow + 5) * FONT_HEIGHT, COLOR_WHITE,
		"DLDI on %s, arm7capable=%d", dldiModeStr, arm7Capable);
	DrawStringF(BOTTOM_SCREEN, FONT_WIDTH, (firstRow + 6) * FONT_HEIGHT, COLOR_WHITE,
		"DLDI: %s", dldiName);
}

static char* calculate_backup_path(const char *cart_name) {
    int path_len = snprintf(NULL, 0, "/cart-backups/%s-backup.bin", cart_name) + 1;
    char *path = (char *)malloc(path_len);
    snprintf(path, path_len, "/cart-backups/%s-backup.bin", cart_name);
    return path;
}

return_codes_t DumpFlash(flashcart_core::Flashcart* cart)
{
	u32 Flash_size = cart->getMaxLength(); //Get the flashrom size
	const u32 chunkSize = 0x8000; // 32KB chunks

	u8 *chunkBuffer = new(std::nothrow) u8[chunkSize];
	if (!chunkBuffer) {
		return MEM_ALLOC_FAILED;
	}

	if (mount_fat() != ALL_OK) {
		delete[] chunkBuffer;
		return FAT_MOUNT_FAILED; //Fat init failed
	}

	char* backup_path = calculate_backup_path(cart->getShortName());
	FILE *FileOut = fopen(backup_path, "wb");
	free(backup_path);
	if (!FileOut) {
		delete[] chunkBuffer;
		return FILE_OPEN_FAILED; //File opening failed
	}

	DrawRectangle(TOP_SCREEN, 0, 2 * FONT_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - 2 * FONT_HEIGHT, COLOR_BLACK);
	DrawString(TOP_SCREEN, FONT_WIDTH, 6 * FONT_HEIGHT, COLOR_WHITE, "Backing up your cart...");

	progressCount = 0; // start the driver-side draw throttle from a known phase
	ShowProgress(BOTTOM_SCREEN, 0, Flash_size, "Reading flash");

	for (u32 chunkOffset = 0; chunkOffset < Flash_size; chunkOffset += chunkSize) {
		SetProgressOverride(chunkOffset, Flash_size);
		char addrStr[64];
		sprintf(addrStr, "Reading address: 0x%08lX", chunkOffset);
		DrawRectangle(TOP_SCREEN, FONT_WIDTH, 8 * FONT_HEIGHT, SCREEN_WIDTH - (2 * FONT_WIDTH), FONT_HEIGHT, COLOR_BLACK);
		DrawString(TOP_SCREEN, FONT_WIDTH, 8 * FONT_HEIGHT, COLOR_WHITE, addrStr);

		u32 currentChunkSize = std::min(chunkSize, Flash_size - chunkOffset);
		if (!cart->readFlash(chunkOffset, currentChunkSize, chunkBuffer)) {
			delete[] chunkBuffer;
			fclose(FileOut);
			SetProgressOverride(0, 0); // Reset override
			return FLASH_OP_FAILED; //Flash reading failed
		}

		if (fwrite(chunkBuffer, 1, currentChunkSize, FileOut) != currentChunkSize) {
			delete[] chunkBuffer;
			fclose(FileOut);
			SetProgressOverride(0, 0); // Reset override
			return FILE_IO_FAILED; //File writing failed
		}

		SetProgressOverride(0, 0); // Reset override before drawing absolute progress
		ShowProgress(BOTTOM_SCREEN, chunkOffset + currentChunkSize, Flash_size, "Reading flash");
	}

	fclose(FileOut);
	delete[] chunkBuffer;

	SetProgressOverride(0, 0); // Reset override
	ShowProgress(BOTTOM_SCREEN, Flash_size, Flash_size, "Reading flash");

	return ALL_OK;
}

return_codes_t WriteFlash(flashcart_core::Flashcart* cart, const char* filepath)
{
	u32 Flash_size = cart->getMaxLength(); //Get the flashrom size
	const u32 chunkSize = 0x8000; // 32KB chunks

	if (mount_fat() != ALL_OK) { return FAT_MOUNT_FAILED; }

	FILE *FileIn = fopen(filepath, "rb");
	if (!FileIn) {
		return FILE_OPEN_FAILED;
	}

	// Validate size before touching the cart -- streaming would otherwise only
	// notice a truncated file mid-loop, aborting with the firmware half overwritten.
	fseek(FileIn, 0, SEEK_END);
	long fileSize = ftell(FileIn);
	rewind(FileIn);
	if (fileSize < 0 || (u32)fileSize < Flash_size) {
		fclose(FileIn);
		return FILE_IO_FAILED;
	}

	u8 *chunkBuffer = new(std::nothrow) u8[chunkSize];
	if (!chunkBuffer) {
		fclose(FileIn);
		return MEM_ALLOC_FAILED;
	}

	DrawRectangle(TOP_SCREEN, 0, 2 * FONT_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - 2 * FONT_HEIGHT, COLOR_BLACK);
	DrawString(TOP_SCREEN, FONT_WIDTH, 6 * FONT_HEIGHT, COLOR_WHITE, "Writing to your cart...");

	progressCount = 0; // start the driver-side draw throttle from a known phase
	ShowProgress(BOTTOM_SCREEN, 0, Flash_size, "Writing flash");

	for (u32 chunkOffset = 0; chunkOffset < Flash_size; chunkOffset += chunkSize) {
		SetProgressOverride(chunkOffset, Flash_size);
		char addrStr[64];
		sprintf(addrStr, "Writing address: 0x%08lX", chunkOffset);
		DrawRectangle(TOP_SCREEN, FONT_WIDTH, 8 * FONT_HEIGHT, SCREEN_WIDTH - (2 * FONT_WIDTH), FONT_HEIGHT, COLOR_BLACK);
		DrawString(TOP_SCREEN, FONT_WIDTH, 8 * FONT_HEIGHT, COLOR_WHITE, addrStr);

		u32 currentChunkSize = std::min(chunkSize, Flash_size - chunkOffset);
		if (fread(chunkBuffer, 1, currentChunkSize, FileIn) != currentChunkSize) {
			delete[] chunkBuffer;
			fclose(FileIn);
			SetProgressOverride(0, 0); // Reset override
			return FILE_IO_FAILED; //File reading failed
		}

		if (!cart->writeFlash(chunkOffset, currentChunkSize, chunkBuffer)) {
			delete[] chunkBuffer;
			fclose(FileIn);
			SetProgressOverride(0, 0); // Reset override
			return FLASH_OP_FAILED; //Flash writing failed
		}
		SetProgressOverride(0, 0); // Reset override before drawing absolute progress
		ShowProgress(BOTTOM_SCREEN, chunkOffset + currentChunkSize, Flash_size, "Writing flash");
	}

	fclose(FileIn);
	delete[] chunkBuffer;

	SetProgressOverride(0, 0); // Reset override
	ShowProgress(BOTTOM_SCREEN, Flash_size, Flash_size, "Writing flash");

	return ALL_OK;
}
