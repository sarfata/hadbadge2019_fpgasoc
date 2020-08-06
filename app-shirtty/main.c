#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "mach_defines.h"
#include "sdk.h"
#include "gfx_load.h"
#include "cache.h"

extern char _binary_tv_png_start;
extern char _binary_tv_png_end;
extern char _binary_laser_png_start;
extern char _binary_laser_png_end;

//Pointer to the framebuffer memory.
uint8_t *fbmem;

#define FB_WIDTH 512
#define FB_HEIGHT 320
#define FB_PAL_OFFSET 256

struct shirtty_mode {
    uint16_t *button_a_pulses;
    uint16_t *button_b_pulses;
    uint16_t *button_left_pulses;
    uint16_t *button_right_pulses;
    uint16_t *button_up_pulses;
    uint16_t *button_down_pulses;
    void *png_start;
    void *png_end;
};

uint16_t team_1[] = {2957, 5996, 2974, 2009, 991, 1987, 987, 2008, 992, 1987, 983, 2008, 1962, 2012, 987, 2008, 966, 0};
uint16_t team_2[] = {2935, 6037, 2957, 2038, 940, 2047, 961, 2039, 914, 2073, 1957, 2042, 910, 2094, 940, 2042, 936, 0};
uint16_t vizio_off[] = {
    9024, 4574, 536, 542, 566, 655, 479, 1780, 535, 625, 509, 573, 509, 655, 484, 650, 509, 651, 483,
    1729, 509, 1750, 535, 625, 484, 1776, 513, 1750, 510, 1724, 509, 1776, 565, 1673, 509, 599, 565,
    599, 535, 573, 565, 1694, 566, 573, 561, 598, 536, 629, 483, 599, 561, 1724, 535, 1698, 566, 1698,
    587, 577, 587, 1642, 565, 1725, 535, 1724, 539, 1694, 587, 0
};

uint16_t climb[] = {
    299, 260, 295, 260, 270, 260, 248, 790, 269, 785, 274, 260, 295, 760, 295, 264, 244, 
    264, 270, 260, 295, 759, 308, 751, 299, 760, 269, 790, 269, 786, 273, 260, 299, 760, 
    270, 260, 273, 261, 269, 264, 295, 760, 299, 760, 269, 786, 269, 269, 295, 0
};
uint16_t dive[] = {
    294, 265, 295, 260, 269, 260, 270, 768, 294, 760, 269, 265, 295, 759, 295, 265, 269, 
    264, 270, 260, 243, 786, 269, 789, 303, 756, 295, 760, 269, 785, 274, 264, 295, 260, 
    274, 755, 274, 260, 295, 264, 270, 264, 269, 260, 270, 768, 286, 264, 244, 0
};
uint16_t left[] = {
    300, 260, 273, 256, 274, 260, 269, 764, 299, 756, 308, 251, 273, 756, 299, 230, 308, 
    256, 273, 260, 248, 781, 300, 768, 290, 760, 299, 756, 299, 760, 299, 230, 310, 254, 
    247, 257, 299, 755, 299, 231, 307, 256, 274, 755, 299, 261, 273, 233, 301, 0
};
uint16_t right[] = {
    282, 277, 278, 255, 274, 256, 259, 773, 304, 751, 303, 256, 252, 776, 303, 260, 274, 
    256, 252, 255, 300, 751, 303, 755, 308, 751, 303, 751, 278, 781, 303, 260, 274, 256, 
    277, 252, 278, 256, 273, 755, 308, 251, 278, 751, 303, 751, 282, 781, 303, 0
};

uint16_t pulses_nop[] = {0};

struct shirtty_mode modes[] = {
    // Basic Shark Mode
    {
        .button_a_pulses = pulses_nop,
        .button_b_pulses = pulses_nop,
        .button_left_pulses = left,
        .button_right_pulses = right,
        .button_up_pulses = climb,
        .button_down_pulses = dive,
        .png_start = &_binary_laser_png_start,
        .png_end = &_binary_laser_png_end
    },
    // Upgraded Shark mode
    {
        .button_a_pulses = pulses_nop,
        .button_b_pulses = pulses_nop,
        .button_left_pulses = left,
        .button_right_pulses = right,
        .button_up_pulses = climb,
        .button_down_pulses = dive,
        .png_start = &_binary_tv_png_start,
        .png_end = &_binary_tv_png_end
    },
    // Nerf Gun - Laser Tag mode
    {
        // Team 1
        .button_a_pulses = team_1,
        // Team 2
        .button_b_pulses = team_2,
        .button_left_pulses = pulses_nop,
        .button_right_pulses = pulses_nop,
        .button_up_pulses = pulses_nop,
        .button_down_pulses = pulses_nop,
        .png_start = &_binary_laser_png_start,
        .png_end = &_binary_laser_png_end
    }
};


// more fine grained sleep
static void delay_us(int us)
{
    // Our best scientists figured out this constant via rigorous theoretical work
    us = us * 1.16;
    for (volatile int t=0; t<us; t++);
}

uint32_t counter60hz(void) {
	return GFX_REG(GFX_VBLCTR_REG);
}

uint16_t counter_sec(void) {
    return counter60hz() / 60;
}

float counter_min(void) {
    return (counter60hz() / 60.0) * 0.01666;
}

uint32_t counter_ms(void) {
    return (counter60hz() / 60.0) * 1000;
}

/*
 * Send an IR command provided in the form of a list of pulses duration (in uS).
 * Look at the irremote circuitpython module to easily measure/generate new patterns.
 *
 * Note that this assumes that you have the ir_modulator module in your FPGA.
 * If you don't have it, nothing will happen. If you have it, the 38kHZ carrier
 * is continuously generated and we modulate the SHUTDOWN pin of the IR
 * transceiver to form the pulses.
 */
void send_ir_pulses(uint16_t pulses[]) {
    MISC_REG(MISC_GPEXT_W2S_REG) = 0xff00;

    for (int i = 0; pulses[i] > 0; i++) {
        MISC_REG(i%2 == 0 ? MISC_GPEXT_W2C_REG:MISC_GPEXT_W2S_REG) = MISC_GPEXT_OUT_REG_IRDA_SD;
        // Start by clearing (to turn on) and then alternate
        delay_us(pulses[i]);
    }
    // Set the POWERDOWN signal HIGH to shutdown the IR transmission.
    MISC_REG(MISC_GPEXT_W2S_REG) = MISC_GPEXT_OUT_REG_IRDA_SD;
}

// fprintf(f, "\033C\0335X\0335Y");

void periodic_pulse(uint32_t on_time_ms, uint16_t pulses[]) {
    uint32_t start_time_ms = counter_ms();
    while (counter_ms() < start_time_ms + on_time_ms) {
        send_ir_pulses(pulses);
    }
}

void flap_right(FILE *f, int speed) {
    // fprintf(f, "ON RIGHT!");
    // periodic_pulse(750, current_mode.button_right_pulses);
    // fprintf(f, "OFF RIGHT!");
    // delay_us(750000);// 500ms

    // fprintf(f, "\033C FAST RIGHT!");
    fprintf(f, "\0336X\0334Y*");

    periodic_pulse(500, right);
    delay_us(50000);//debounce
    // fprintf(f, "\033C FAST RIGHT RESET!"); 
    periodic_pulse(250, left);
    delay_us(50000);//debounce
    fprintf(f, "\0336X\0334YF");

}

void flap_left(FILE *f, int speed) {
    if (speed == 1) {
        // fprintf(f, "\033C ON LEFT!");
        periodic_pulse(750, left);
        // fprintf(f, "\033C OFF LEFT!");
        delay_us(750000);// 500ms
    } else if (speed == 2) {
        // fprintf(f, "\033C MID LEFT!");
        periodic_pulse(500, left);
        // fprintf(f, "\033C MID LEFT!");
        delay_us(500000);// 500ms
    } else if (speed == 3) {
        fprintf(f, "\0332X\0334Y*");
        // fprintf(f, "\033C FAST LEFT!");
        periodic_pulse(500, left);
        delay_us(50000);//debounce
        // fprintf(f, "\033C FAST LEFT RESET!"); 
        periodic_pulse(250, right);
        delay_us(50000);//debounce
        fprintf(f, "\0332X\0334YF");
    }
}

void flap_forward(FILE *f) {
    // fprintf(f, "FORWARD LEFT!");
    periodic_pulse(500, left);
    delay_us(250000);// 250ms
    // fprintf(f, "FORWARD RIGHT!");
    periodic_pulse(500, right);
    delay_us(250000);// 250ms
}

void basic_left(FILE *f, struct shirtty_mode current_mode) {
    fprintf(f, "\0332X\0334Y*");
    send_ir_pulses(left);
    delay_us(50000);//debounce
    fprintf(f, "\0332X\0334Y ");
}

void basic_right(FILE *f) {
    fprintf(f, "\0336X\0334Y*");
    send_ir_pulses(right);
    delay_us(50000);//debounce
    fprintf(f, "\0336X\0334Y ");
}

void basic_dive(FILE *f) {
    // fprintf(f, "DIVE!");
    send_ir_pulses(dive);
    delay_us(50000);//debounce
}

void basic_climb(FILE *f) {
    // fprintf(f, "CLIMB!");
    send_ir_pulses(climb);
    delay_us(50000);//debounce
}

void tag_team_a(FILE *f) {
    // fprintf(f, "\033C\0335X\0335Y");
    // fprintf(f, "Tag Team 1!");
    send_ir_pulses(team_1);
    // fprintf(f, "\033C");
    delay_us(500000);//debounce
}

void controls(FILE *f, int current_mode_index) {
    if (current_mode_index == 0) {
        fprintf(f, "\033C\0331X\0331Y");
        fprintf(f, "Sharks Can Fly!");
        fprintf(f, "\0334X\0333Y");
        fprintf(f, "C");
        fprintf(f, "\0333X\0334Y");
        fprintf(f, "L R");
        fprintf(f, "\0334X\0335Y");
        fprintf(f, "D");
    } else if (current_mode_index == 1) {
        fprintf(f, "\033C\0331X\0331Y");
        fprintf(f, "Sharky BEAST MODE!");

        fprintf(f, "\0335X\0333Y");
        fprintf(f, "C");
        fprintf(f, "\0333X\0334Y");
        fprintf(f, "FL FR");
        fprintf(f, "\0335X\0335Y");
        fprintf(f, "D");
        fprintf(f, "\03310X\0334Y");
        fprintf(f, "FORWARD");
    } else if (current_mode_index == 2) {
        fprintf(f, "\033C\0331X\0331Y");
        fprintf(f, "Norf Laser Tag!");
    }
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
    int png_size=(&_binary_laser_png_end-&_binary_laser_png_start);
    gfx_load_fb_mem(fbmem, &GFXPAL[FB_PAL_OFFSET], 4, 512, &_binary_laser_png_start, png_size);

    //Flush the memory region to psram so the GFX hw can stream it from there.
    cache_flush(fbmem, fbmem+FB_WIDTH*FB_HEIGHT);

    //The IPL leaves us with a tileset that has tile 0 to 127 map to ASCII characters, so we do not need to
    //load anything specific for this.

    GFX_REG(GFX_LAYEREN_REG)=GFX_LAYEREN_FB|GFX_LAYEREN_TILEA;

    // fprintf(f, "\033C");


    int current_mode_index = 0;
    struct shirtty_mode current_mode = modes[current_mode_index];

    controls(f, current_mode_index);

    // Press LEFT (BACK) to exit the program and go back to main menu
    while ((MISC_REG(MISC_BTN_REG) & BUTTON_START)==0) {
        // fprintf(f, "\033C");
        // 60Hz = ~17ms
        // (1000/16.666)*(1-0.16) = ~49 
        // The magic 16% faster is back! 49
        // if (counter60hz() % 3 == 0) { 
        //     fprintf(f, "\033C%d %d %.2f", counter_sec(), counter_ms(), counter_min());
        // }
        if (MISC_REG(MISC_BTN_REG) & BUTTON_A) {
            if (current_mode_index == 0) {

            } else if (current_mode_index == 1) {
                flap_forward(f);
            } else if (current_mode_index == 2) {
                tag_team_a(f);
            }
        }
        else if (MISC_REG(MISC_BTN_REG) & BUTTON_B) {
            if (current_mode_index == 0) {

            } else if (current_mode_index == 1) {

            } else if (current_mode_index == 2) {
                // fprintf(f, "\033C\0335X\0335Y");
                // fprintf(f, "Tag Team B!");
                send_ir_pulses(team_2);
                // fprintf(f, "\033C");
                delay_us(50000);//debounce
            }     
        } else if (MISC_REG(MISC_BTN_REG) & BUTTON_LEFT) {
            if (current_mode_index == 0) {
                basic_left(f, current_mode);
            } else if (current_mode_index == 1) {
                flap_left(f, 3);
            }
        } else if (MISC_REG(MISC_BTN_REG) & BUTTON_RIGHT) {
            if (current_mode_index == 0) {
                basic_right(f);
            } else if (current_mode_index == 1) {
                flap_right(f, 1);
            }
        } else if (MISC_REG(MISC_BTN_REG) & BUTTON_UP) {
            if (current_mode_index == 0 || current_mode_index == 1) {
                basic_dive(f);
            }
        } else if (MISC_REG(MISC_BTN_REG) & BUTTON_DOWN) {
            if (current_mode_index == 0 || current_mode_index == 1) {
                basic_climb(f);
            }
        }
        else if (MISC_REG(MISC_BTN_REG) & BUTTON_SELECT) {
            current_mode_index = (current_mode_index + 1) % (sizeof(modes)/sizeof(modes[0]));
            current_mode = modes[current_mode_index];

            GFX_REG(GFX_LAYEREN_REG)=0;
            gfx_load_fb_mem(fbmem, &GFXPAL[FB_PAL_OFFSET], 4, 512, current_mode.png_start, current_mode.png_end - current_mode.png_start);
            GFX_REG(GFX_LAYEREN_REG)=GFX_LAYEREN_FB|GFX_LAYEREN_TILEA;

            //Flush the memory region to psram so the GFX hw can stream it from there.
            cache_flush(fbmem, fbmem+FB_WIDTH*FB_HEIGHT);

            controls(f, current_mode_index);
        }

    }
}
