# Modifications

Vendored from [flashcart_core](https://github.com/ntrteam/flashcart_core) at
commit `03d464c` (tag `v1.1.0`). Changes by `@tasken`:

- `devices/ace3dsplus.cpp`: removed the post-init `rdid == 0xFFFFFF` failure
  guard, so Macronix "sleeping flash" clones (which report all-`FF` during
  init) are accepted. Added them to the description.
- `devices/*.cpp`: reworded the author and description strings to a consistent
  "Works with:" format. No behaviour change.
- Removed upstream `.github/`.
