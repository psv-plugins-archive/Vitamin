#include "graphics.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define SCE_DISPLAY_UPDATETIMING_NEXTVSYNC SCE_DISPLAY_SETBUF_NEXTFRAME
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/threadmgr.h>

#define FONT_SIZE 16

enum {
	SCREEN_WIDTH = 960,
	SCREEN_HEIGHT = 544,
	LINE_SIZE = 960,
	FRAMEBUFFER_SIZE = 2 * 1024 * 1024,
	FRAMEBUFFER_ALIGNMENT = 256 * 1024
};

typedef union
{
	int rgba;
	struct
	{
		char r;
		char g;
		char b;
		char a;
	} c;
} color_t;

extern u8 msx[];
void* g_vram_base;
static int gX = 0;
static int gY = 0;
static int g_left_margin = 0, g_right_margin = 60, g_top_margin = 0, g_bottom_margin = 34;

static Color g_fg_color;
static Color g_bg_color;

static Color* getVramDisplayBuffer()
{
	Color* vram = (Color*) g_vram_base;
	return vram;
}

void *psvDebugScreenGetVram() {
	return g_vram_base;
}

int psvDebugScreenGetX() {
	return gX / FONT_SIZE;
}

int psvDebugScreenGetY() {
	return gY / FONT_SIZE;
}

void psvDebugScreenSetXY(int x, int y) {
	gX = x * FONT_SIZE;
	gY = y * FONT_SIZE;

	if (gX < g_left_margin * FONT_SIZE)
		gX = g_left_margin * FONT_SIZE;
	
	if (gX > g_right_margin * FONT_SIZE)
		gX = g_right_margin * FONT_SIZE;
	
	if (gY < g_top_margin * FONT_SIZE)
		gY = g_top_margin * FONT_SIZE;
	
	if (gY > g_bottom_margin * FONT_SIZE)
		gY = g_bottom_margin * FONT_SIZE;
}

void psvDebugScreenResetMargin() {
	g_left_margin = 0;
	g_right_margin = 60;
	g_top_margin = 0;
	g_bottom_margin = 34;
}

void psvDebugScreenSetLeftMargin(int left_margin) {
	g_left_margin = left_margin;
	
	if (gX < g_left_margin * FONT_SIZE)
		gX = g_left_margin * FONT_SIZE;
}

void psvDebugScreenSetRightMargin(int right_margin) {
	g_right_margin = right_margin;
	
	if (gX > g_right_margin * FONT_SIZE)
		gX = g_right_margin * FONT_SIZE;
}

void psvDebugScreenSetTopMargin(int top_margin) {
	g_top_margin = top_margin;
	
	if (gY < g_top_margin * FONT_SIZE)
		gY = g_top_margin * FONT_SIZE;
}

void psvDebugScreenSetBottomMargin(int bottom_margin) {
	g_bottom_margin = bottom_margin;
	
	if (gY > g_bottom_margin * FONT_SIZE)
		gY = g_bottom_margin * FONT_SIZE;
}

 // #define LOG(args...)  		vita_logf (__FILE__, __LINE__, args)  ///< Write a log entry

int g_log_mutex;

void psvDebugScreenInit() {
	g_log_mutex = sceKernelCreateMutex("log_mutex", 0, 0, NULL);

	SceKernelAllocMemBlockOpt opt = { 0 };
	opt.size = sizeof(opt);
	opt.attr = 0x00000004;
	opt.alignment = FRAMEBUFFER_ALIGNMENT;
	SceUID displayblock = sceKernelAllocMemBlock("display", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, FRAMEBUFFER_SIZE, &opt);
	printf("displayblock: 0x%08x", displayblock);
	void *base;
	sceKernelGetMemBlockBase(displayblock, &base);
	// LOG("base: 0x%08x", base);

	SceDisplayFrameBuf framebuf = { 0 };
	framebuf.size = sizeof(framebuf);
	framebuf.base = base;
	framebuf.pitch = SCREEN_WIDTH;
	framebuf.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
	framebuf.width = SCREEN_WIDTH;
	framebuf.height = SCREEN_HEIGHT;

	g_vram_base = base;

	sceDisplaySetFrameBuf(&framebuf, SCE_DISPLAY_UPDATETIMING_NEXTVSYNC);

	g_fg_color = 0xFFFFFFFF;
	g_bg_color = 0x00000000;
}

void psvDebugScreenClear(int bg_color)
{
	gX = g_left_margin * FONT_SIZE;
	gY = g_top_margin * FONT_SIZE;

	int i;
	color_t *pixel = (color_t *)getVramDisplayBuffer();
	for(i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
		pixel->rgba = bg_color;
		pixel++;
	}
}

void psvDebugScreenClearMargin(int bg_color) {
	gX = g_left_margin * FONT_SIZE;
	gY = g_top_margin * FONT_SIZE;

	color_t *pixel = (color_t *)getVramDisplayBuffer();

	int y;
	for (y = gY; y < ((g_bottom_margin + 1) * FONT_SIZE); y++) {
		int x;
		for (x = gX; x < ((g_right_margin + 1) * FONT_SIZE); x++) {
			pixel[x + y * LINE_SIZE].rgba = bg_color;
		}
	}
}

void psvDebugScreenClearLine(int line_y, int bg_color) {
	color_t *pixel = (color_t *)getVramDisplayBuffer();

	int y;
	for (y = 0; y < FONT_SIZE; y++) {
		int x;
		for (x = 0; x < (60 * FONT_SIZE); x++) {
			pixel[x + ((line_y * FONT_SIZE) + y) * LINE_SIZE].rgba = bg_color;
		}
	}
}

void psvDebugScreenClearLineMargin(int line_y, int bg_color) {
	color_t *pixel = (color_t *)getVramDisplayBuffer();

	int y;
	for (y = 0; y < FONT_SIZE; y++) {
		int x;
		for (x = (g_left_margin * FONT_SIZE); x < ((g_right_margin + 1) * FONT_SIZE); x++) {
			pixel[x + ((line_y * FONT_SIZE) + y) * LINE_SIZE].rgba = bg_color;
		}
	}
}

static void printTextScreen(const char * text)
{
	int c, i, j, l;
	u8 *font;
	Color *vram_ptr;
	Color *vram;

	for (c = 0; c < strlen(text); c++) {
		if (gX > (g_right_margin * FONT_SIZE)) {
			gY += FONT_SIZE;
			gX = g_left_margin * FONT_SIZE;
			psvDebugScreenClearLineMargin(gY / FONT_SIZE, g_bg_color);
		}
		if (gY > (g_bottom_margin * FONT_SIZE)) {
			gY = g_top_margin * FONT_SIZE;
			psvDebugScreenClearLineMargin(gY / FONT_SIZE, g_bg_color);
		}
		char ch = text[c];
		if (ch == '\n') {
			gX = g_left_margin * FONT_SIZE;
			gY += FONT_SIZE;
			psvDebugScreenClearLineMargin(gY / FONT_SIZE, g_bg_color);
			continue;
		} else if (ch == '\r') {
			gX = g_left_margin * FONT_SIZE;
			continue;
		} else if (ch >= 0x80) {
			continue;
		}

		vram = getVramDisplayBuffer() + gX + gY * LINE_SIZE;

		font = &msx[ (int)ch * 8];
		for (i = l = 0; i < 8; i++, l += 8, font++) {
			vram_ptr  = vram;
			for (j = 0; j < 8; j++) {
				if ((*font & (128 >> j))) {
					*(uint32_t *)(vram_ptr) = g_fg_color;
					*(uint32_t *)(vram_ptr + 1) = g_fg_color;
					*(uint32_t *)(vram_ptr + LINE_SIZE) = g_fg_color;
					*(uint32_t *)(vram_ptr + LINE_SIZE + 1) = g_fg_color;
				} else {
					*(uint32_t *)(vram_ptr) = g_bg_color;
					*(uint32_t *)(vram_ptr + 1) = g_bg_color;
					*(uint32_t *)(vram_ptr + LINE_SIZE) = g_bg_color;
					*(uint32_t *)(vram_ptr + LINE_SIZE + 1) = g_bg_color;
				}
				vram_ptr += 2;
			}
			vram += 2 * LINE_SIZE;
		}
		gX += FONT_SIZE;
	}
}

void psvDebugScreenPrintf(const char *format, ...) {
	char buf[1024];

	sceKernelLockMutex(g_log_mutex, 1, NULL);

	va_list opt;
	va_start(opt, format);
	vsnprintf(buf, sizeof(buf), format, opt);
	printTextScreen(buf);
	va_end(opt);

	sceKernelUnlockMutex(g_log_mutex, 1);
}

Color psvDebugScreenSetFgColor(Color color) {
	Color prev_color = g_fg_color;
	g_fg_color = color;
	return prev_color;
}

Color psvDebugScreenSetBgColor(Color color) {
	Color prev_color = g_bg_color;
	g_bg_color = color;
	return prev_color;
}