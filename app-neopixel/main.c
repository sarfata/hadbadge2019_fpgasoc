#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "mach_defines.h"
#include "sdk.h"
#include "gfx_load.h"
#include "cache.h"

extern char _binary_tv_png_start;
extern char _binary_tv_png_end;

//Pointer to the framebuffer memory.
uint8_t *fbmem;

#define FB_WIDTH 512
#define FB_HEIGHT 320
#define FB_PAL_OFFSET 256

// more fine grained sleep
static void delay_us(int us)
{
    // Our best scientists figured out this constant via rigorous theoretical work
    us = us * 1.16;
    for (volatile int t=0; t<us; t++);
}

static void neopixel_write(int index, uint8_t r, uint8_t g, uint8_t b) {
    uint32_t data = (r << 16) | (g << 8) | (b);

    // Write the index first because we will actually write to memory fb when you write the data.
    MISC_REG(MISC_GPEXT_NEOPIXEL_INDEX_REG) = index;
    MISC_REG(MISC_GPEXT_NEOPIXEL_DATA_REG) = data;
}

void main(int argc, char **argv) {
    fbmem=calloc(FB_WIDTH,FB_HEIGHT);
    //Set up the framebuffer address
    GFX_REG(GFX_FBADDR_REG)=((uint32_t)fbmem);

    //We're going to use a pitch of 512 pixels, and the fb palette will start at 256.
    GFX_REG(GFX_FBPITCH_REG)=(FB_PAL_OFFSET<<GFX_FBPITCH_PAL_OFF)|(512<<GFX_FBPITCH_PITCH_OFF);

    GFX_REG(GFX_LAYEREN_REG)=0; //disable all gfx layers


    FILE *f;
    f=fopen("/dev/console", "w");
    setvbuf(f, NULL, _IONBF, 0); //make console line unbuffered

    //Now, use a library function to load the image into the framebuffer memory. This function will also set up the palette entries,
    //we tell it to start writing from entry 128.
    int png_size=(&_binary_tv_png_end-&_binary_tv_png_start);
    gfx_load_fb_mem(fbmem, &GFXPAL[FB_PAL_OFFSET], 4, 512, &_binary_tv_png_start, png_size);

    //Flush the memory region to psram so the GFX hw can stream it from there.
    cache_flush(fbmem, fbmem+FB_WIDTH*FB_HEIGHT);

    //The IPL leaves us with a tileset that has tile 0 to 127 map to ASCII characters, so we do not need to
    //load anything specific for this.

    GFX_REG(GFX_LAYEREN_REG)=GFX_LAYEREN_FB|GFX_LAYEREN_TILEA;

    // fprintf(f, "\033C");


    // initialize something
    for (int i = 0; i < 64; i++) {
        int column = i % 8;
        int row = i / 8;

        neopixel_write(i, row * 20, column * 20, i);
    }

    int shift = 0;

    // Press LEFT (BACK) to exit the program and go back to main menu
    while ((MISC_REG(MISC_BTN_REG) & BUTTON_SELECT)==0) {
        if (MISC_REG(MISC_BTN_REG) & BUTTON_A) {
            shift++;

            for (int i = 0; i < 64; i++) {
                int column = i % 8;
                int row = i / 8;

                neopixel_write((i + shift)%64, row * 20, column * 20, i);
            }
            delay_us(50000);
        }
    }
}
