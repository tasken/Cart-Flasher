FROM skylyrac/blocksds:slim-latest

ENV BLOCKSDS=/opt/wonderful/thirdparty/blocksds/core

# The base image is a static snapshot — Docker Hub's `latest` tag doesn't
# auto-refresh, so it can lag behind the rolling package repo (e.g. missing
# libnds fixes like ARM9 gettimeofday() support). Force a package refresh at
# image-build time so we always get current BlocksDS packages.
RUN wf-pacman -Syu --noconfirm && wf-config clean-caches --all

# The repo is bind-mounted from the host, so its owning UID won't match the
# container's — git refuses to touch it ("dubious ownership") unless told
# it's trusted, which breaks the `git describe` version-string lookups.
RUN git config --global --add safe.directory '*'

# Set up the working directory inside the container
WORKDIR /work
