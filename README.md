<p align="center">
  <img src="resources/logo.png" alt="Cart-Flasher Logo" width="128">
</p>

# Cart-Flasher

A DS/DSi homebrew application to dump and write raw flash memory to/from Slot-1 flashcarts.

To compile, clone the repository and simply run `make`. A Docker-based build is also available via `./build.sh` if you don't have devkitARM installed locally.

## Credits & Attribution

*   **Developed by**: [@tasken](https://github.com/tasken)
*   **Upstream Base**: Repurposed as a general-purpose flashcart dump/write tool, built on top of:
    *   [ntrboot_flasher_nds](https://github.com/jason0597/ntrboot_flasher_nds) by jason0597 (original project)
    *   [ntrboot_flasher_nds](https://github.com/DS-Homebrew/ntrboot_flasher_nds) by DS-Homebrew (enhanced fork)
*   **Drivers & Core**:
    *   [flashcart_core](https://github.com/ntrteam/flashcart_core) by ntrteam, for the per-flashcart device drivers
    *   [libncgc](https://github.com/angelsl/libncgc) by angelsl, for the NTR/CTR card protocol layer

Special thanks to:
*   [Sanrax](https://github.com/Sanrax) for feedback and pre-release testing.

The button-combo confirmation before any destructive flash operation is styled after GodMode9/SafeMode9's `d0k3` prompt.
