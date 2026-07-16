# Modifications

Vendored from [libncgc](https://github.com/angelsl/libncgc) at commit
`166dc42`. Changes by `@tasken` (build files only; the C/C++ sources are
unmodified):

- `platform.ntr.make`: BlocksDS-only toolchain; removed the devkitARM fallback.
- `Makefile`: `clean` now also removes `lib/`.
