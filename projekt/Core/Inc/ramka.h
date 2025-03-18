/*
 * ramka.h
 *
 *  Created on: Jan 7, 2025
 *      Author: amekow
 */

#ifndef INC_RAMKA_H_
#define INC_RAMKA_H_

#include "main.h"
#include "string.h"
#include "stdarg.h"

//do crc
#define POLYNOMIAL 0x80 // Wielomian 100000001
#define CRC_INITIAL 0x00

extern uint8_t dma_transfer_complete;
extern uint8_t tresc[161];

//void sendMA(char* message);
void odczytSIM(/*uint8_t* komenda, bufork_t* rxBufor*/);
uint8_t oblicz_crc(const uint8_t *buf, uint8_t len);
uint8_t dopisz_znak_ramki(uint8_t* ramka);
uint8_t dekoduj_ramke(uint8_t* komenda);
void wybierz(uint8_t* str,uint8_t* sub,  uint8_t len);
void initSIM800();
void obsluga_komend(uint8_t* komenda);

#endif /* INC_RAMKA_H_ */
