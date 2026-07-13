#include "nds_platform.h"
#include <nds.h>
#include <fat.h>
#include "device.h"
#include "blowfish_ntr_bin.h"
#include "blowfish_dev_bin.h"
#include "blowfish_retail_bin.h"
#define FONT_WIDTH  6
#define FONT_HEIGHT 10
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

			static bool first_open = true;
			if (mount_fat() != ALL_OK) { return -1; }

			// Overwrite if this is our first time opening the file.
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
			unmount_fat();
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

static char* calculate_backup_path(const char *cart_name) {
    int path_len = snprintf(NULL, 0, "/cart-backups/%s-backup.bin", cart_name) + 1;
    char *path = (char *)malloc(path_len);
    snprintf(path, path_len, "/cart-backups/%s-backup.bin", cart_name);
    return path;
}

return_codes_t DumpFlash(flashcart_core::Flashcart* cart)
{
	u32 Flash_size = cart->getMaxLength(); //Get the flashrom size

	u8 *fileBuffer = new(std::nothrow) u8[Flash_size];
	if (!fileBuffer) {
		return MEM_ALLOC_FAILED;
	}

	DrawRectangle(TOP_SCREEN, 0, 2 * FONT_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - 2 * FONT_HEIGHT, COLOR_BLACK);
	DrawString(TOP_SCREEN, FONT_WIDTH * 2, 6 * FONT_HEIGHT, COLOR_WHITE, "Backing up your cart...");

	ShowProgress(BOTTOM_SCREEN, 0, Flash_size, "Backing up your cart...");

	const u32 chunkSize = 0x8000; // 32KB chunks
	for (u32 chunkOffset = 0; chunkOffset < Flash_size; chunkOffset += chunkSize) {
		SetProgressOverride(chunkOffset, Flash_size);
		char addrStr[64];
		sprintf(addrStr, "Reading address: 0x%08lX", chunkOffset);
		DrawRectangle(TOP_SCREEN, FONT_WIDTH * 2, 8 * FONT_HEIGHT, SCREEN_WIDTH - FONT_WIDTH * 4, FONT_HEIGHT, COLOR_BLACK);
		DrawString(TOP_SCREEN, FONT_WIDTH * 2, 8 * FONT_HEIGHT, COLOR_WHITE, addrStr);

		u32 currentChunkSize = std::min(chunkSize, Flash_size - chunkOffset);
		if (!cart->readFlash(chunkOffset, currentChunkSize, fileBuffer + chunkOffset)) {
			delete[] fileBuffer;
			SetProgressOverride(0, 0); // Reset override
			return FLASH_OP_FAILED; //Flash reading failed
		}
		SetProgressOverride(0, 0); // Reset override before drawing absolute progress
		ShowProgress(BOTTOM_SCREEN, chunkOffset + currentChunkSize, Flash_size, "Backing up your cart...");
	}

	if (mount_fat() != ALL_OK)
	{
		delete[] fileBuffer;
		return FAT_MOUNT_FAILED; //Fat init failed
	}

	char* backup_path = calculate_backup_path(cart->getShortName());
	FILE *FileOut = fopen(backup_path, "wb");
	if (!FileOut) {
		delete[] fileBuffer;
		free(backup_path);
		return FILE_OPEN_FAILED; //File opening failed
	}

	if (fwrite(fileBuffer, 1, Flash_size, FileOut) != Flash_size) {
		delete[] fileBuffer;
		fclose(FileOut);
		free(backup_path);
		return FILE_IO_FAILED; //File writing failed
	}

	fclose(FileOut);
	free(backup_path);
	delete[] fileBuffer;

	SetProgressOverride(0, 0); // Reset override
	ShowProgress(BOTTOM_SCREEN, Flash_size, Flash_size, "Backing up your cart...");

	return ALL_OK;
}

return_codes_t WriteFlash(flashcart_core::Flashcart* cart, const char* filepath)
{
	u32 Flash_size = cart->getMaxLength(); //Get the flashrom size

	if (mount_fat() != ALL_OK) { return FAT_MOUNT_FAILED; }

	FILE *FileIn = fopen(filepath, "rb");
	if (!FileIn) {
		return FILE_OPEN_FAILED;
	}

	u8 *fileBuffer = new(std::nothrow) u8[Flash_size];
	if (!fileBuffer) {
		fclose(FileIn);
		return MEM_ALLOC_FAILED;
	}

	if (fread(fileBuffer, 1, Flash_size, FileIn) != Flash_size) {
		delete[] fileBuffer;
		fclose(FileIn);
		return FILE_IO_FAILED; //File reading failed
	}

	fclose(FileIn);

	DrawRectangle(TOP_SCREEN, 0, 2 * FONT_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT - 2 * FONT_HEIGHT, COLOR_BLACK);
	DrawString(TOP_SCREEN, FONT_WIDTH * 2, 6 * FONT_HEIGHT, COLOR_WHITE, "Writing to your cart...");

	ShowProgress(BOTTOM_SCREEN, 0, Flash_size, "Writing to flash...");

	const u32 chunkSize = 0x8000; // 32KB chunks
	for (u32 chunkOffset = 0; chunkOffset < Flash_size; chunkOffset += chunkSize) {
		SetProgressOverride(chunkOffset, Flash_size);
		char addrStr[64];
		sprintf(addrStr, "Writing address: 0x%08lX", chunkOffset);
		DrawRectangle(TOP_SCREEN, FONT_WIDTH * 2, 8 * FONT_HEIGHT, SCREEN_WIDTH - FONT_WIDTH * 4, FONT_HEIGHT, COLOR_BLACK);
		DrawString(TOP_SCREEN, FONT_WIDTH * 2, 8 * FONT_HEIGHT, COLOR_WHITE, addrStr);

		u32 currentChunkSize = std::min(chunkSize, Flash_size - chunkOffset);
		if (!cart->writeFlash(chunkOffset, currentChunkSize, fileBuffer + chunkOffset)) {
			delete[] fileBuffer;
			SetProgressOverride(0, 0); // Reset override
			return FLASH_OP_FAILED; //Flash writing failed
		}
		SetProgressOverride(0, 0); // Reset override before drawing absolute progress
		ShowProgress(BOTTOM_SCREEN, chunkOffset + currentChunkSize, Flash_size, "Writing to flash...");
	}

	SetProgressOverride(0, 0); // Reset override
	ShowProgress(BOTTOM_SCREEN, Flash_size, Flash_size, "Writing to flash...");

	delete[] fileBuffer;
	return ALL_OK;
}
