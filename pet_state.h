/*
 * pet_state.h
 *
 *  Created on: Jun 2, 2025
 *      Author: kavya
 */

#ifndef PET_STATE_H
#define PET_STATE_H

#include <stdbool.h>
#include <stdint.h>

// Demo constants (adjust as needed)
#define STAT_MAX             100
#define STAT_MIN             0
#define SICK_THRESHOLD       10
#define SICK_DECAY_RATE      2    // per tick
#define NORMAL_DECAY_RATE    1    // per tick
#define SICK_TICKS_TO_DEATH  15   // number of ticks pet can remain sick
#define PET_NAME_MAX_LEN    16
#define HUNGER_THRESHOLD    70
#define SLEEP_THRESHOLD     50
#define HAPPINESS_THRESHOLD 80

typedef enum {
    PET_IDLE,
    PET_FEED,
    PET_PLAY,
    PET_SLEEP,
    PET_SICK,
    PET_DEAD
} PetMode;

typedef enum {
    NORMAL,
    HUNGRY,
    SLEEPY,
    LONELY,
    SICK,
    DEAD,
} PetStatus;

typedef struct {
    int      hunger;
    int      sleepiness;
    int      happiness;
    bool     isSick;
    bool     isAlive;
    int      sick_counter;
    PetMode  mode;
    PetStatus status;
    char name[PET_NAME_MAX_LEN];
} PetState;

// Initialize everything to “full” and alive
void init_pet_state(PetState *pet);

// Action triggers (adjust stats + switch mode)
void feed_pet(PetState *pet);
void play_with_pet(PetState *pet);
void put_pet_to_sleep(PetState *pet);

// Called periodically (e.g., every Timer tick) to decay stats & handle sickness/death
void update_pet_state(PetState *pet);

#endif // PET_STATE_H


