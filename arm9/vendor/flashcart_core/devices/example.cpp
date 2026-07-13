#ifdef DONT_INCLUDE_EXAMPLE_PLEASE

#include "../device.h"

namespace flashcart_core {
using ntrcard::sendCommand;
using platform::logMessage;
using platform::showProgress;

class Example : Flashcart {
    public:
        // Name & Size of Flash Memory
        Example() : Flashcart("Example Name", "Example", 0x400000) { }

        const char* getAuthor() { return "your name"; }
        // Keep the "Works with:" + bullet-list format other devices use, for
        // a consistent look in the flashcart-select screen.
        const char* getDescription() {
            return  "Works with:\n"
                    " * Some Cart (example.com)\n"
                    " * Some Other Cart (example.com)";
        }

        /*
        called when the user selects this flashcart
        return false to error
        return true otherwise
        */
        bool initialize() {
            return true;
        }

        void shutdown() { }

        bool readFlash(uint32_t address, uint32_t length, uint8_t *buffer) { return true; }
        bool writeFlash(uint32_t address, uint32_t length, const uint8_t *buffer) { return true; }
        bool injectNtrBoot(uint8_t *blowfish_key, uint8_t *firm, uint32_t firm_size) { return true; }
};

// adds your cart to the list
Example example;
}
#endif
