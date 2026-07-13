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
echo "Running: sudo docker compose run --rm --build builder sh -c \"$CMD\" (log: $BUILD_LOG)"
echo ""
# Remove any stale log from a previous run before tee opens a fresh one. Doing
# this here (not in the Makefile's `clean` target) matters: that target runs
# *inside* the same piped command below, after tee has already opened this
# file, so it must never remove build.log itself -- unlinking a file a
# running process still holds open doesn't stop that process from writing to
# it, but the file vanishes entirely once the pipeline finishes and tee closes
# its handle.
rm -f "$BUILD_LOG"
sudo docker compose run --rm --build builder sh -c "$CMD" 2>&1 | tee "$BUILD_LOG"
