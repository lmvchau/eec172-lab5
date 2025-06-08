/*
 * pet_logic.h
 *
 *  Created on: Jun 2, 2025
 *      Author: luuanne
 */

#ifndef PET_LOGIC_H_
#define PET_LOGIC_H_


#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "pet_state.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "i2c_if.h"
#include "gpio_if.h"
#include "oled_test.h"
#include "uart_if.h"
#include "utils.h"

#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define YELLOW  0xFFE0
#define PINK       0xFE19
#define LIGHTGREEN 0x9772

#define MAX_TAPS        5

#define HUNGER_Y    70
#define SLEEP_Y     90
#define HAPPY_Y     110

#define BAR_X       30
#define BAR_MAX_W   90
#define BAR_H       8



// external globals from main.c / other modules:
extern volatile unsigned char ir_intflag;
extern volatile unsigned int  ir_signal;
extern volatile uint8_t       is_repeat;
extern uint8_t getButton(void);
extern volatile unsigned int flag_aws_sync;
extern int g_aws_sock;
extern volatile int lowercaseNext;

extern char button_chars[11][MAX_TAPS];
extern int  max_taps[11];

extern PetState myPet;


// Type representing our high‐level FSM states:
typedef enum {
    STATE_ONBOARD,         // Show the “egg” until shake
    STATE_SELECT_PET,      // Let user pick one of 3 faces
    STATE_NAME_PET,        // Let user enter a name via multi‐tap
    STATE_PET_EXISTENCE,   // Normal “alive” loop: decay stats, draw hunger/sleep/happiness
    STATE_FEED,            // Temporary sub‐state: feed_pet(&pet)
    STATE_PLAY,            // Temporary sub‐state: play_with_pet(&pet)
    STATE_SLEEP,           // Temporary sub‐state: put_pet_to_sleep(&pet)
    STATE_SICK,            // Pet is sick (show some “!” or warning)
    STATE_DEAD             // Pet is dead (show an X or red screen forever)
} GameState;

extern PetState pet;    // our single global PetState instance

int select_pet(void);
int stat_to_width(int stat);
int update_bar(int x, int y, int h,
                      int prev_w, int new_w,
                      uint16_t color);
void drawArt(const char **art, int lines, int x, int y);
int8_t read_accel_register(uint8_t reg_addr);
bool check_shake(void);
void name_pet(char *name_buf);
void drawTwoDigit(int x, int y, int value);
void draw_pet_stats(const PetState *p);
void pet_logic_main(void);

#endif /* PET_LOGIC_H_ */
