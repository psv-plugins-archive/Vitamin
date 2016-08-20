#pragma once

typedef unsigned char u8;
typedef unsigned u32;
typedef u32 Color;

// allocates memory for framebuffer and initializes it
void psvDebugScreenInit();

// clears screen with a given color
void psvDebugScreenClear(int bg_color);

void psvDebugScreenClearMargin(int bg_color);

// printf to the screen
void psvDebugScreenPrintf(const char *format, ...);

// set foreground (text) color
Color psvDebugScreenSetFgColor(Color color);

// set background color
Color psvDebugScreenSetBgColor(Color color);

void *psvDebugScreenGetVram();

int psvDebugScreenGetX();
int psvDebugScreenGetY();
void psvDebugScreenSetXY(int x, int y);

void psvDebugScreenResetMargin();
void psvDebugScreenSetLeftMargin(int left_margin);
void psvDebugScreenSetRightMargin(int right_margin);
void psvDebugScreenSetTopMargin(int top_margin);
void psvDebugScreenSetBottomMargin(int bottom_margin);

#define DARKBLUE 0x003F1F00
#define YELLOW 0x007FFFFF
#define WHITE 0x00FFFFFF
#define CYAN 0x00FFFF7F
#define GREEN 0x007FFF00
#define RED 0x000000FF

#define BLUE 0x00FF7F00

#define BLACK 0x00000000

#define GRAY 0x007F7F7F