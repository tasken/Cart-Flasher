FROM devkitpro/devkitarm:20230622

# The repo is bind-mounted from the host, so its owning UID won't match the
# container's — git refuses to touch it ("dubious ownership") unless told
# it's trusted, which breaks the `git describe` version-string lookups.
RUN git config --global --add safe.directory '*'

# Set up the working directory inside the container
WORKDIR /work
