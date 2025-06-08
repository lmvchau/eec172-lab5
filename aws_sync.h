/*
 * aws_sync.h
 *
 *  Created on: Jun 1, 2025
 *      Author: luuanne
 */

#ifndef AWS_SYNC_H_
#define AWS_SYNC_H_



/**
 * @brief  ISR is triggered when TimerA1 counts down to 0.
 *         Will call AWS Synchronization.
 */
void SyncAWSIntHandler(void);

/**
 * @brief  Initialize TimerA1 in periodic mode.
 *         When the period elapses, SyncAWSInt_Handler will fire.
 */
void TimerA1_Init(void);

/**
 * @brief  Open a TLS socket, POST the given sJSON payload to AWS SHadow,
 *         then do HTTP GET, close the socket
 *
 */
//int UpdateAWS_Shadow(const char *message);
int UpdateAWS_Shadow(const PetState *pet);


#endif /* AWS_SYNC_H_ */
