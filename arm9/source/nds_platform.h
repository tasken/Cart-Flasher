#include "device.h"

enum return_codes_t {ALL_OK, FAT_MOUNT_FAILED, FILE_OPEN_FAILED, FILE_IO_FAILED, FLASH_OP_FAILED, MEM_ALLOC_FAILED};

return_codes_t mount_fat(void);
return_codes_t unmount_fat(void);
return_codes_t DumpFlash(flashcart_core::Flashcart* cart);
return_codes_t WriteFlash(flashcart_core::Flashcart* cart, const char* filepath);
// Logs the probe and draws it on the bottom screen from `firstRow` down, taking
// seven rows. The caller owns the screen around it: at boot it sits under the
// SD-failure notice, from the menu it gets the screen to itself.
void LogHardwareProbe(int firstRow);
