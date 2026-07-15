<p align="center">
  <img src="resources/logo.png" alt="Cart-Flasher Logo" width="128">
</p>

# Cart-Flasher

A DS/DSi homebrew application to backup and restore raw flash images to/from Slot-1 flashcarts.

## Getting started

Grab the latest `.nds` from [Releases](https://github.com/tasken/Cart-Flasher/releases/latest), copy it to your SD card, and launch it from any DS/DSi homebrew launcher. Pick your flashcart from the list, then Backup to dump its flash to `/cart-backups` on your SD card, or Restore to write an image back.

Supports Ace3DS Plus, AK2i, DSTT, R4iSDHC family, and R4i Gold 3DS, in both DS and DSi mode.

## Building

```shell
git clone https://github.com/tasken/Cart-Flasher.git
cd Cart-Flasher/
sudo ./build.sh
```

This builds inside Docker (BlocksDS toolchain included) and produces `cart_flasher-<commit>.nds`. If you already have BlocksDS installed locally, `make` works directly without Docker.

## Credits

*   Developed by `Tasken` (`Nimbo` on Discord)
*   Upstream base: repurposed as a general-purpose flashcart dump/write tool, built on top of:
    *   [ntrboot_flasher_nds](https://github.com/jason0597/ntrboot_flasher_nds) by `jason0597` (original project)
    *   [ntrboot_flasher_nds](https://github.com/DS-Homebrew/ntrboot_flasher_nds) by `DS-Homebrew` (enhanced fork)
*   Drivers & core:
    *   [flashcart_core](https://github.com/ntrteam/flashcart_core) by `ntrteam`, for the per-flashcart device drivers
    *   [libncgc](https://github.com/angelsl/libncgc) by `angelsl`, for the NTR/CTR card protocol layer

Special thanks to [Sanras](https://github.com/Sanrax) for feedback and pre-release testing.

The button-combo confirmation before any destructive flash operation is styled after [GodMode9](https://github.com/d0k3/GodMode9)'s `d0k3` prompt.

## License

GPL-3.0 - see [LICENSE](LICENSE).
