DangoPlayer Switch — Font Directory
====================================

Place a TrueType (.ttf) font file here for it to be embedded into the NRO.

The app looks for fonts in this order:
  1. romfs:/fonts/font.ttf           (this directory, embedded in NRO)
  2. sdmc:/switch/DangoPlayer/fonts/font.ttf  (SD card, user-provided)
  3. romfs:/fonts/NotoSans-Regular.ttf
  4. sdmc:/switch/DangoPlayer/fonts/NotoSans-Regular.ttf

Recommended font: Noto Sans (https://fonts.google.com/noto/specimen/Noto+Sans)
Download "NotoSans-Regular.ttf" and rename it to "font.ttf", then place it here.

If no font is found, the UI will render shapes and colors but no text.
