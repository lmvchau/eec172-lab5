/*
 * pet_logic.c
 *
 *  Created on: Jun 2, 2025
 *      Author: kavya
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "Adafruit_GFX.h"
#include "pet_state.h"
#include "pet_logic.h"
#include "Adafruit_SSD1351.h"
#include "i2c_if.h"
#include "gpio_if.h"
#include "uart_if.h"
#include "utils.h"

#define MAX_TAPS    5
#define ACCEL_ADDR  0x18
#define X_REG       0x03
#define Y_REG       0x05
#define CHAR_W      6
#define CHAR_H      8


extern int UpdateAWS_Shadow(const PetState *p);

const char *egg_art[] = {
    "     ____     ",
    "    /    \\    ",
    "   /      \\   ",
    "  /        \\  ",
    "  |  ???   |  ",
    "  \\        /  ",
    "   \\______/   "
};

static const char *pets[3] = {
    "(>^_^)>",  // choice #1
    "(^o^)",    // choice #2
    "('v')"     // choice #3
};

static const char *sick_pets[3] = {
   "(>x_x)>",  // choice #1
   "(x_x)",    // choice #2
   "(;_;)"     // choice #3
};

//const char *lil_guy = {
//
//};

static int selectedPetIdx = 0;

PetState myPet;


int stat_to_width(int stat) {
    if (stat < 0)   stat = 0;
    if (stat > 99)  stat = 99;
    return (stat * BAR_MAX_W) / 99;
}

int update_bar(int x, int y, int h,
                      int prev_w, int new_w,
                      uint16_t color)
{
    if (new_w > prev_w) {
        int added = new_w - prev_w;
        fillRect(x + prev_w, y, added, h, color);
    }
    else if (new_w < prev_w) {
        int removed = prev_w - new_w;
        fillRect(x + new_w, y, removed, h, BLACK);
    }
    return new_w;
}

void drawArt(const char **art, int lines, int x, int y)
{
    int i, j;
    for (i = 0; i < lines; i++) {
        for (j = 0; art[i][j] != '\0'; j++) {
            drawChar(x + j * 6, y + i * 8, art[i][j], WHITE, BLACK, 1);
        }
    }
}
int8_t read_accel_register(uint8_t reg_addr) {
    uint8_t raw = 0;
    I2C_IF_Write(ACCEL_ADDR, &reg_addr, 1, 0);
    I2C_IF_Read (ACCEL_ADDR, &raw, 1);
    return (int8_t) raw;
}

bool check_shake(void) {
    const int8_t THRESHOLD = 20;
    int8_t x = read_accel_register(X_REG);
    int8_t y = read_accel_register(Y_REG);
    Report("Accel raw: X = %d,  Y = %d\n\r", x, y);
    return ( (x >  THRESHOLD) || (x < -THRESHOLD) ||
             (y >  THRESHOLD) || (y < -THRESHOLD) );
}

int select_pet(void)
{
    int selected = 0;
    int k;
    fillScreen(BLACK);

    while (1) {

        // 1a) Draw “select pet” centered at y=20:
        {
            const char *msg = "Select your pet";
            int len = strlen(msg);             // 10 chars
            int x0  = (128 - (len * 6)) / 2;    // each char is 6 px wide
            int y0  = 20;
            int i;
            for (i = 0; i < len; i++) {
                drawChar(x0 + i * 6, y0, msg[i], WHITE, BLACK, 1);
            }
        }
        // 1b) Draw “Press OK to Select” centered at y=20:
        {
            const char *msg = "Press MUTE to confirm";
            int len = strlen(msg);             // 10 chars
            int x0  = (128 - (len * 6)) / 2;    // each char is 6 px wide
            int y0  = 30;
            int i;
            for (i = 0; i < len; i++) {
                drawChar(x0 + i * 6, y0, msg[i], WHITE, BLACK, 1);
            }
        }


        // 2) Draw the pet’s face centered at y=60:
        const char *face = pets[selected];
        int x0 = (128 - ((int)strlen(face) * 6)) / 2;
        for (k = 0; face[k]; k++) {
            drawChar(x0 + k * 6, 60, face[k], PINK, BLACK, 1);
        }

        // 3) Draw “<” / “>” hints below the pet—say at y = 60
        //    Only draw "<" if selected > 0, only draw ">" if selected < 2.
        int hintY = 80;
        const int arrowColor = BLUE;
        // Clear any previous arrows (just overwrite in black background):
        //   (If you clear the entire bottom line each iteration, you can simply do:)
        fillRect(0, hintY, 128, 8, BLACK);

        if (selected > 0) {
            // draw “<” at, say, x = 20
            drawChar(20, hintY, '-', arrowColor, BLACK, 1);
        }
        if (selected < 2) {
            // draw “>” at, say, x = (128 - 6 - 20) = 102
            drawChar(102, hintY, '+', arrowColor, BLACK, 1);
        }


        // 4) Wait until the next IR event
        while (!ir_intflag);
        ir_intflag = 0;
        uint8_t btn = getButton();

        if (btn == 14 && selected > 0)      selected--;
        else if (btn == 13 && selected < 2) selected++;
        else if (btn == 11)                { /*OK*/  break; }
        // else ignore any other key
    }
    return selected;
}

void name_pet(char *name_buf)
{
    int idx = 0;
    uint8_t last_btn = 255;
    int tap_count = 0;

    int curX = 5;
    int curY = 48;
    int prevX = 0;
    int prevY = 0;

    const char *prompt = "Enter a name";
    int prompt_len = strlen(prompt);          // number of characters
    int prompt_x  = curX;
    int prompt_y  = 20;

    fillScreen(BLACK);
    {
        int i;
        for (i = 0; i < prompt_len; i++) {
            drawChar(prompt_x + i * 6, prompt_y, prompt[i], WHITE, BLACK, 1);
        }
    }
    const char *second_prompt = "Press MUTE to confirm";
    prompt_len = strlen(second_prompt);
    {
            int i;
            for (i = 0; i < prompt_len; i++) {
                drawChar(prompt_x + i * 6, prompt_y+8, second_prompt[i], WHITE, BLACK, 1);
            }
        }

    drawChar(curX, curY, '>', WHITE, BLACK, 1);  // prompt arrow
    curX += CHAR_W;
    fillRect(curX, curY, 1, CHAR_H, GREEN);

    while (idx < PET_NAME_MAX_LEN - 1) {
        // 1) wait for IR input
        while (!ir_intflag);
        ir_intflag = 0;
        uint8_t btn = getButton();

        // 2) If it’s a digit (0..9), pick character from multi‐tap table
        if (btn <= 9 || btn == 13) {
            if (btn == last_btn && is_repeat) {
                tap_count++;
                idx--;  // overwrite the prior char
            }
            else {
                tap_count = 0;
            }
            char c;
            // pick the character
            if (btn <= 9) {
                c = button_chars[btn]
                                 [ tap_count % max_taps[btn] ];
            } else {
                c = button_chars[10][ tap_count % max_taps[10] ];  // special case for button_digit = 13
            }

            if (lowercaseNext && c >= 'A' && c <= 'Z'){
                c = c - 'A' + 'a';
            }

            // draw it on the OLED
            if (is_repeat) {
                // use previous location
                drawChar(prevX, prevY, c, BLACK, BLACK, 1);
                drawChar(prevX, prevY, c, BLUE, BLACK, 1);
            } else {
                // use updated location
                drawChar(curX, curY, c, BLUE, BLACK, 1);
                prevX = curX;  prevY = curY;
                curX += CHAR_W;
                if (curX > 128 - CHAR_W) {
                    curX = 0;  curY += CHAR_H;
                }
                if (curY > 127) {
                    fillRect(0, 40, 128, 88, BLACK);
                    curX = 0;  curY = 40;
                }
            }

            // draw vertical line in green for cursor
            fillRect(curX, curY, 1, CHAR_H, GREEN);


//            // Draw (or overwrite) the letter
//            drawChar(base_x + 6 + (idx * 6), base_y, c, BLUE, BLACK, 1);
            name_buf[idx] = c;
            idx++;
            last_btn = btn;
        }
        // 3) If “delete” (10) and idx>0, remove previous char
        else if (btn == 10 && idx > 0) {
            fillRect(curX, curY, 1, CHAR_H, BLACK);

            // go back 1 character's worth (a CHAR_W x CHAR_H block) , and update current position
            if (curX == 0) {
                curY -= CHAR_H;
                curX = 128 - CHAR_W;
            }
            else {
                curX -= CHAR_W;
            }

            // fill next CHAR_W x CHAR_H block in BLACK to "delete"
            fillRect(curX, curY, CHAR_W, CHAR_H, BLACK);

            // fill new location with green vertical line for updated cursor
            fillRect(curX, curY, 1, CHAR_H, GREEN);


            idx--;
//            drawChar(base_x + 6 + (idx * 6), base_y, ' ', BLACK, BLACK, 1);
            prevX = curX;
            prevY = curY;
            last_btn = 255;
        }
        // 4) If “OK” (11) and at least one char typed, finish
        else if (btn == 11 && idx > 0) {
            break;
        }


        else if (btn == 14){
            if (!lowercaseNext) {lowercaseNext = 1;}
            else {lowercaseNext = 0;}

        }
        // Otherwise ignore
    }
    name_buf[idx] = '\0';
}

// ----------------------------------------------------
// Draw a two‐digit number at (x,y), with leading zero if needed
// ----------------------------------------------------
void drawTwoDigit(int x, int y, int value)
{
    if (value < 0) value = 0;
    if (value > 99) value = 99;
    drawChar(x,     y, '0' + (value / 10), WHITE, BLACK, 1);
    drawChar(x + 6, y, '0' + (value % 10), WHITE, BLACK, 1);
}

// ----------------------------------------------------
// Draw the pet’s current H, S, L bars in the top half.
// Clears only that top half so we don’t erase the rest.
// ----------------------------------------------------
void draw_pet_stats(const PetState *p)
{
    // 1) Clear the top‐left 128×64 region
    fillRect(0, 0, 128, 64, BLACK);

    // 2) Hunger at (10,10)
    drawChar(10, 10, 'H', WHITE, BLACK, 1);
    drawTwoDigit(30, 10, p->hunger);

    // 3) Sleepiness at (10, 30)
    drawChar(10, 30, 'S', WHITE, BLACK, 1);
    drawTwoDigit(30, 30, p->sleepiness);

    // 4) Happiness at (10, 50)
    drawChar(10, 50, 'L', WHITE, BLACK, 1);
    drawTwoDigit(30, 50, p->happiness);

    // 5) If the pet is sick, draw a big “!” in red at (100,10)
    if (p->isSick) {
        drawChar(100, 10, '!', RED, BLACK, 2);
    }
}

void pet_logic_main(void)
{
    // 1) Begin in the ONBOARD state
    GameState state = STATE_ONBOARD;

    // Spare buffers:
    char chosenName[PET_NAME_MAX_LEN];


    int prev_hunger_w     = 0;
    int prev_sleep_w      = 0;
    int prev_happy_w      = 0;


    while (1) {


        switch (state) {
        //---------------------------------------
        // STATE_ONBOARD: show the “egg” art,
        // wait for tilt/shake to break out.
        //---------------------------------------
        case STATE_ONBOARD:
            I2C_IF_Open(I2C_MASTER_MODE_FST);   // enable accel
            fillScreen(BLACK);

            // Draw “shake to hatch” centered at y = 20
            {
                const char *msg = "Shake to hatch!";
                int len = strlen(msg);         // 13 characters
                int x0  = (128 - (len * 6)) / 2; // each character is 6 pixels wide
                int y0  = 20;                   // pick a Y‐position above the egg
                int i;
                for (i = 0; i < len; i++) {
                    drawChar(x0 + i * 6, y0, msg[i], WHITE, BLACK, 1);
                }
            }
            // Draw the larger ASCII‐egg
            drawArt(egg_art, 7, (128 - 13*6)/2,  40);

            // spin here until a shake is detected
            while (!check_shake()) {
                UtilsDelay(200000);
            }

            UtilsDelay(2000000);
            // Draw “shake to hatch” centered at y = 20
            {
                const char *msg = "Something is hatching!";
                int len = strlen(msg);         // 13 characters
                int x0  = (128 - (len * 6)) / 2; // each character is 6 pixels wide
                int y0  = 20;                   // pick a Y‐position above the egg
                int i;
                for (i = 0; i < len; i++) {
                    drawChar(x0 + i * 6, y0, msg[i], WHITE, BLACK, 1);
                }
            }

            const char *egg_crack = "//\\//\\//\\";
            {
                int i;
                int len = strlen(egg_crack);
                int x0 = (128 - (len * 6)) / 2;
                int y0 = 72;
                for (i = 0; i < len; i++){
                    drawChar(x0 + i*6, y0, egg_crack[i], WHITE, BLACK, 1);
                }
            }
            UtilsDelay(2000000);
            state = STATE_SELECT_PET;
            break;

        //---------------------------------------
        // STATE_SELECT_PET: show 3 pet faces,
        // allow < / > to cycle, OK to confirm.
        //---------------------------------------
        case STATE_SELECT_PET:
            selectedPetIdx = select_pet();        // sets the chosen face into a local variable (not stored)
            state = STATE_NAME_PET;
            break;

        //---------------------------------------
        // STATE_NAME_PET: let user type & confirm
        //---------------------------------------
        case STATE_NAME_PET:
            {
                fillScreen(BLACK);
                name_pet(chosenName);


                // Now initialize all other fields of the pet
                init_pet_state(&myPet);

                // Copy the chosenName into our global pet struct:
                memset(myPet.name, 0, sizeof(myPet.name));
                strncpy(myPet.name, chosenName, PET_NAME_MAX_LEN - 1);

                // After naming, draw the chosen pet’s face + name once:
                fillScreen(BLACK);
                const char *face = pets[selectedPetIdx];
                int x0 = (128 - ((int)strlen(face) * 6)) / 2;
                int i;
                for (i = 0; face[i]; i++) {
                    drawChar(x0 + i * 6, 10, face[i], WHITE, BLACK, 1);
                }


                // Draw the name below the face:
                {
                    int x0 = 10, y0 = 28;
                    int i;
                    for (i = 0; myPet.name[i]; i++) {
                        drawChar(x0 + i * 6, y0, myPet.name[i], WHITE, BLACK, 1);
                    }
                }
                // Draw static labels for bars (H, S, L):
                drawChar(10, HUNGER_Y, 'H', WHITE, BLACK, 1);
                drawChar(10, SLEEP_Y,  'S', WHITE, BLACK, 1);
                drawChar(10, HAPPY_Y,  'L', WHITE, BLACK, 1);

                // Go to “pet‐existence” main loop
                state = STATE_PET_EXISTENCE;

            }
            break;

        //---------------------------------------
        // STATE_PET_EXISTENCE: the normal “alive”
        // loop.  Show H/S/L, update state, react
        // to IR (feed/play/sleep), auto‐tick timers,
        // sync with AWS, etc.
        //---------------------------------------
        case STATE_PET_EXISTENCE:
            {
                // 1) Update decay & sickness/death logic
                update_pet_state(&myPet);
                if (!myPet.isSick && myPet.isAlive) {
                                drawChar(120, 0, '!', BLACK, BLACK, 2);

                            }


                // 2) Compute new bar widths:
                int new_hunger_w = stat_to_width(myPet.hunger);
                int new_sleep_w  = stat_to_width(myPet.sleepiness);
                int new_happy_w  = stat_to_width(myPet.happiness);

                // 3) Update each bar by only redrawing changed portions:
                prev_hunger_w = update_bar(
                    BAR_X, HUNGER_Y, BAR_H,
                    prev_hunger_w, new_hunger_w,
                    GREEN
                );
                prev_sleep_w  = update_bar(
                    BAR_X, SLEEP_Y, BAR_H,
                    prev_sleep_w,  new_sleep_w,
                    YELLOW
                );
                prev_happy_w  = update_bar(
                    BAR_X, HAPPY_Y, BAR_H,
                    prev_happy_w,  new_happy_w,
                    BLUE
                );




                // 5) Check IR buttons for user‐triggered transitions:
                if (ir_intflag) {
                    ir_intflag = 0;
                    uint8_t btn = getButton();

                    if (btn == 1) {
                        state = STATE_FEED;
                        break;
                    }
                    else if (btn == 3) {
                        state = STATE_PLAY;
                        break;
                    }
                    else if (btn == 2) {
                        state = STATE_SLEEP;
                        break;
                    }
                }

                if (flag_aws_sync){
                    // Clear flag to 0
                    flag_aws_sync = 0;

                    update_pet_state(&myPet);

                    if (UpdateAWS_Shadow(&myPet) < 0 ){
                        Report("Periodic sync failed: %d\n\r", g_aws_sock);
                        sl_Close(g_aws_sock);
                    } else {
                        Report("Periodic sync success!");
                    }
                }

                // 7) Check for Sick or Dead status transitions:
                if (!myPet.isAlive) {
                    state = STATE_DEAD;
                    break;
                }
                if (myPet.isSick) {
                    const char *sick_face = sick_pets[selectedPetIdx];
                    int x0 = (128 - (int)strlen(sick_face) * 6) / 2;
                    int i;
                    for (i = 0; sick_face[i]; i++) {
                        drawChar(x0 + i * 6, 10, sick_face[i], RED, BLACK, 1);
                    }
                    state = STATE_SICK;
                    break;
                }

                // 8) Delay a bit before next loop iteration:
                UtilsDelay(2000000);
            }
            break;

        //---------------------------------------
        // STATE_FEED: run feed_pet(&pet), then
        // immediately return to PET_EXISTENCE.
        //---------------------------------------
        case STATE_FEED:
            feed_pet(&myPet);
            if (!myPet.isSick){
                state = STATE_PET_EXISTENCE;
            }
            else {
                state = STATE_SICK;
            }
            break;

        //---------------------------------------
        // STATE_PLAY: run play_with_pet(&pet),
        // then back to PET_EXISTENCE.
        //---------------------------------------
        case STATE_PLAY:
            play_with_pet(&myPet);
            if (!myPet.isSick){
                state = STATE_PET_EXISTENCE;
            }
            else {
                state = STATE_SICK;
            }
            break;

        //---------------------------------------
        // STATE_SLEEP: run put_pet_to_sleep(&pet),
        // then back to PET_EXISTENCE.
        //---------------------------------------
        case STATE_SLEEP:
            put_pet_to_sleep(&myPet);
            if (!myPet.isSick){
                state = STATE_PET_EXISTENCE;
            }
            else {
                state = STATE_SICK;
            }
            break;

        //---------------------------------------
        // STATE_SICK: show a “! Pet is sick” screen
        // until pet.isSick clears (update_pet_state
        // eventually cures or kills the pet).
        //---------------------------------------
        case STATE_SICK:
        {

            // 1) Draw/update the three bars exactly as in PET_EXISTENCE:
            int new_hunger_w = stat_to_width(myPet.hunger);
            int new_sleep_w  = stat_to_width(myPet.sleepiness);
            int new_happy_w  = stat_to_width(myPet.happiness);

            prev_hunger_w = update_bar(
                BAR_X, HUNGER_Y, BAR_H,
                prev_hunger_w, new_hunger_w,
                GREEN
            );
            prev_sleep_w  = update_bar(
                BAR_X, SLEEP_Y, BAR_H,
                prev_sleep_w,  new_sleep_w,
                YELLOW
            );
            prev_happy_w  = update_bar(
                BAR_X, HAPPY_Y, BAR_H,
                prev_happy_w,  new_happy_w,
                BLUE
            );


            if (ir_intflag) {
                ir_intflag = 0;
                uint8_t btn = getButton();

                if (btn == 1) {
                    state = STATE_FEED;
                    break;
                }
                else if (btn == 3) {
                    state = STATE_PLAY;
                    break;
                }
                else if (btn == 2) {
                    state = STATE_SLEEP;
                    break;
                }
            }

            fillRect(120, 0, 8, 16, BLACK);
            drawChar(120, 0, '!', RED, BLACK, 2);
            UtilsDelay(2000000);


            // Let update_pet_state handle whether it gets better or dies:
            update_pet_state(&myPet);
            if (!myPet.isSick && myPet.isAlive) {
                const char *face = pets[selectedPetIdx];
                                int x0 = (128 - ((int)strlen(face) * 6)) / 2;
                                int i;
                                for (i = 0; face[i]; i++) {
                                    drawChar(x0 + i * 6, 10, face[i], WHITE, BLACK, 1);
                                }
                drawChar(120, 0, '!', BLACK, BLACK, 2);
                state = STATE_PET_EXISTENCE;
            }
            else if (!myPet.isAlive) {
                fillScreen(RED);
                drawChar(50, 30, 'X', WHITE, RED, 4);


                // 2) Draw a two‐line message beneath the “X”
                //    Line 1: "pet died :("
                //    Line 2: "press 1 to restart"

                int x0_line1 = (128 - (11 * 6)) / 2;  // = 31
                int y0_line1 = 80;                    // just below the 'X'

                // "press 1 to restart" is 19 chars → width = 19*6 = 114 pixels
                int x0_line2 = (128 - (19 * 6)) / 2;  // = 7
                int y0_line2 =  90;               // one row below line1 (e.g., y0_line1 + 10)

                const char *line1 = "pet died :(";
                const char *line2 = "press 1 to restart";

                int i;
                for (i = 0; i < 11; i++) {
                    drawChar(x0_line1 + i*6, y0_line1, line1[i], WHITE, RED, 1);
                }

                // Draw line2 in WHITE on RED
                for (i = 0; i < 19; i++) {
                    drawChar(x0_line2 + i*6, y0_line2, line2[i], WHITE, RED, 1);
                }

                state = STATE_DEAD;
            }

            break;
        }

        //---------------------------------------
        // STATE_DEAD: final “X” screen—pet died.
        // We stay here forever.
        //---------------------------------------
        case STATE_DEAD:

            if (ir_intflag) {
                ir_intflag = 0;
                uint8_t btn = getButton();

                if (btn == 1) {
                    state = STATE_ONBOARD;
                }
            }
            break;


        } // end switch
    } // end while(1)
}
