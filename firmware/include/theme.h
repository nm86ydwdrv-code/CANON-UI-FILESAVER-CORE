#pragma once
#include <M5Stack.h>

// "Bruce"-style pixelated amber/orange-on-black theme
#define THEME_BG       0x0000  // black background
#define THEME_PANEL    0x0000  // panels are black with an orange border
#define THEME_ACCENT   0xFC40  // bright orange (#FF8800) - borders/header/footer
#define THEME_TEXT     0xFC40  // orange text
#define THEME_HILIGHT  0xFE60  // pale orange highlight
#define THEME_SELECT   0xFC40  // selection bar fill (orange, text drawn in black)
#define THEME_SELTEXT  0x0000  // text color when drawn over THEME_SELECT

// Sage-mode toad pet palette (Kawazu Kumite)
#define FROG_BODY      0xF344  // toad orange
#define FROG_DARK      0x28A0  // dark brown outline / eyebrows / mouth
#define FROG_VEST      0x5B6F  // blue-grey vest straps
#define FROG_BELLY     0xF6F6  // cream belly
#define FROG_EYE       0xFFFF  // eye white
#define FROG_PUPIL     0x0000  // pupil black
#define FROG_TONGUE    0xF800  // tongue red
