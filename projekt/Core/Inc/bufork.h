/*
 * bufork.h
 *
 *  Created on: Nov 14, 2024
 *      Author: amekow
 */

#ifndef INC_BUFORK_H_
#define INC_BUFORK_H_

#include "main.h"

#define BUF_LEN 4096

typedef struct {
	uint8_t* bufor;
	uint16_t start;
	uint16_t end;
} bufork_t;

extern bufork_t rxBufor2;
extern bufork_t txBufor2;

extern bufork_t rxBufor1;
extern bufork_t txBufor1;



int8_t bufork_zapisz(bufork_t *buf, uint8_t dane);
int8_t bufork_odczyt(bufork_t *buf, uint8_t *data);
#endif /* INC_BUFORK_H_ */
