/*
 * pet_state.c
 *
 *  Created on: Jun 2, 2025
 *      Author: kavya
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "pet_state.h"

#define DECAY_SKIP  3


// Helper: clamp `val` between [min..max].
static int clamp(int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

void init_pet_state(PetState *pet) {
    pet->hunger       = STAT_MAX;
    pet->sleepiness   = STAT_MAX;
    pet->happiness    = STAT_MAX;
    pet->isSick       = false;
    pet->isAlive      = true;
    pet->sick_counter = 0;
    pet->mode         = PET_IDLE;
    pet->status       =  NORMAL;
    memset(pet->name, 0, PET_NAME_MAX_LEN);
    strncpy(pet->name, "Boots", PET_NAME_MAX_LEN - 1);
}

void feed_pet(PetState *pet) {
    if (!pet->isAlive) return;

    // If the pet was already marked sick (because a stat hit zero), feeding cures it.
    if (pet->isSick){
        pet->isSick = false;
        pet->sick_counter = 0;
    }
    pet->hunger = clamp(pet->hunger + 10, STAT_MIN, STAT_MAX);
    pet->mode   = PET_FEED;
}

void play_with_pet(PetState *pet) {
    if (!pet->isAlive) return;
    pet->happiness = clamp(pet->happiness + 10, STAT_MIN, STAT_MAX);
    pet->mode      = PET_PLAY;
}

void put_pet_to_sleep(PetState *pet) {
    if (!pet->isAlive) return;
    pet->sleepiness = clamp(pet->sleepiness + 10, STAT_MIN, STAT_MAX);
    pet->mode       = PET_SLEEP;
}

void update_pet_state(PetState *pet) {

    if (!pet->isAlive) return;

    static int decay_counter = 0;

    // 1) Only actually subtract stats every SLOW_DECAY_SKIP calls
    if (++decay_counter < DECAY_SKIP) {
        return;
    }
    decay_counter = 0;

    // 1) First, decay each stat by 1 (or by SICK_DECAY_RATE if already sick).
   int decay = pet->isSick ? SICK_DECAY_RATE : NORMAL_DECAY_RATE;
   pet->hunger     = clamp(pet->hunger     - decay, STAT_MIN, STAT_MAX);
   pet->sleepiness = clamp(pet->sleepiness - decay, STAT_MIN, STAT_MAX);
   pet->happiness  = clamp(pet->happiness  - decay, STAT_MIN, STAT_MAX);



    if (!pet->isSick) {



            if (pet->hunger == STAT_MIN ||
                        pet->sleepiness == STAT_MIN ||
                        pet->happiness == STAT_MIN)
                {
                    pet->isSick       = true;
                    pet->mode         = PET_SICK;
                    pet->sick_counter = 0;    // start counting “how long sick”
                    pet->status       = SICK;
                }

        }


        // If already sick, only advance sick_counter once all three are zero:
        if (pet->isSick)
        {
            pet->sick_counter++;
            if (pet->sick_counter >= SICK_TICKS_TO_DEATH) {
                pet->isAlive = false;
                pet->mode    = PET_DEAD;
                pet->status  = DEAD;
                return;
            }

            if (pet->hunger > STAT_MIN &&
                        pet->sleepiness > STAT_MIN &&
                        pet->happiness > STAT_MIN)
                {
                    pet->isSick       = false;
                    pet->mode         = PET_IDLE;
                    pet->sick_counter = 0;
                    pet->status       = NORMAL;
                }

        }

        if (pet->isSick) {
            pet->status = SICK;
        }
        else if (pet->hunger < HUNGER_THRESHOLD) {
            pet->status = HUNGRY;
        }
        else if (pet->sleepiness < SLEEP_THRESHOLD) {
            pet->status = SLEEPY;
        }
        else if (pet->happiness < HAPPINESS_THRESHOLD) {
            pet->status = LONELY;
        }
        else {
            pet->status = NORMAL;
        }

}

