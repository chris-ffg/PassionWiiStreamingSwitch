# Dockerfile for building DangoPlayer Switch
FROM devkitpro/devkita64:latest

# Install required Switch homebrew libraries
RUN dkp-pacman -Syu --noconfirm && \
    dkp-pacman -S --noconfirm \
        switch-dev \
        switch-sdl2 \
        switch-sdl2_ttf \
        switch-sdl2_image \
        switch-sdl2_gfx \
        switch-sdl2_mixer \
        switch-mesa \
        switch-glad \
        switch-freetype \
        switch-libpng \
        switch-libjpeg-turbo \
        switch-libwebp \
        switch-zlib \
        switch-bzip2 \
        switch-mpv \
        switch-ffmpeg \
        switch-libass \
        switch-libfribidi \
        switch-dav1d \
        switch-curl \
        switch-mbedtls

WORKDIR /project

# Default command builds the project
CMD ["make"]
