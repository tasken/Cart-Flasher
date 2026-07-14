#!/bin/bash
set -eo pipefail

# Change to the script's directory
cd "$(dirname "$0")"

BUILD_LOG="build.log"

# Check for Docker installation
if ! command -v docker &> /dev/null; then
    echo "Error: docker is not installed. Please install Docker."
    exit 1
fi

case "$1" in
    clean)
        MSG="Cleaning Cart-Flasher via Docker"
        CMD="make clean"
        ;;
    build|"")
        MSG="Building Cart-Flasher via Docker"
        CMD="make clean && make"
        ;;
    *)
        echo "Usage: $0 [clean|build]"
        exit 1
        ;;
esac

echo "=== $MSG ==="
# Resolve the real invoking user's UID/GID, not the shell's current one:
# when this whole script is run as `sudo ./build.sh`, `id -u`/`id -g` at this
# point would already report 0:0 (root), silently defeating --user below.
# sudo exports SUDO_UID/SUDO_GID for exactly this case; fall back to id for a
# plain (non-sudo) invocation.
BUILD_UID="${SUDO_UID:-$(id -u)}"
BUILD_GID="${SUDO_GID:-$(id -g)}"
echo "Running: sudo docker compose run --rm --build --user \"$BUILD_UID:$BUILD_GID\" builder sh -c \"$CMD\" (log: $BUILD_LOG)"
echo ""
# Remove any stale log from a previous run before tee opens a fresh one. Doing
# this here (not in the Makefile's `clean` target) matters: that target runs
# *inside* the same piped command below, after tee has already opened this
# file, so it must never remove build.log itself -- unlinking a file a
# running process still holds open doesn't stop that process from writing to
# it, but the file vanishes entirely once the pipeline finishes and tee closes
# its handle.
rm -f "$BUILD_LOG"
# --user matches the container to the host UID/GID: docker-compose.yml has no
# `user:` directive, so without this the container runs as the base image's
# default (root), and every build artifact it writes into the bind-mounted
# .:/work (obj/, out/, .elf/.nds outputs) ends up root-owned on the host --
# blocking any later non-sudo command (e.g. the host-side PLATFORM=test build)
# from touching those files without another sudo call.
sudo docker compose run --rm --build --user "$BUILD_UID:$BUILD_GID" builder sh -c "$CMD" 2>&1 | tee "$BUILD_LOG"
