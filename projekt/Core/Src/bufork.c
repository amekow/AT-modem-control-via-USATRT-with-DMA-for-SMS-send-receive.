/*
 * bufork.c
 *
 *  Created on: Nov 13, 2024
 *      Author: amekow
 */
#include "main.h"
#include "bufork.h"

uint8_t BUF_RX1[BUF_LEN];
uint8_t BUF_TX1[BUF_LEN];
uint8_t BUF_RX2[BUF_LEN];
uint8_t BUF_TX2[BUF_LEN];
bufork_t rxBufor1 = {BUF_RX1, 0, 0};
bufork_t txBufor1 = {BUF_TX1, 0, 0};
//do uarta drugiego
bufork_t rxBufor2 = {BUF_RX2, 0, 0};
bufork_t txBufor2 = {BUF_TX2, 0, 0};


int8_t bufork_zapisz(bufork_t *buf, uint8_t dane) {
	uint16_t tmp;
	tmp = (buf->start + 1) % BUF_LEN;	// Przypisujemy do zmiennej następny indeks start
	// Jeśli był to ostatni element tablicy to ustawiamy wskaźnik na jej początek

	// Sprawdzamy czy jest miejsce w buforze.
	// Jeśli bufor jest pełny to wychodzimy z funkcji i zwracamy błąd (-1).
	if ( tmp == buf->end ) {
		return -1;
	} else {
		// Jeśli jest miejsce w buforze to przechodzimy dalej:
		buf->bufor[buf->start] = dane;	// Wpisujemy wartość do bufora
		buf->start = tmp;				// Zapisujemy nowy indeks start
	}
	return 0;	// wszystko przebiegło pozytywnie, więc zwracamy 0
}

int8_t bufork_odczyt(bufork_t *buf, uint8_t *data)
{
	// Sprawdzamy czy w buforze jest coś do odczytania
	// Jeśli bufor jest pusty to wychodzimy z funkcji i zwracamy błąd.
	if (buf->start == buf->end)
		return -1;

	// Jeśli jest coś do odczytania to przechodzimy dalej:
	*data = buf->bufor[buf->end];		// Odczytujemy wartość z bufora
	buf->end++; // Inkrementujemy indeks end
	// Jeśli był to ostatni element tablicy to ustawiamy wskaźnik na jej początek
	if (buf->end == BUF_LEN) {
		buf->end = 0;
	}


	return 0;	// wszystko przebiegło pozytywnie, więc zwracamy 0

}

/*
void opoznienie(uint16_t tm){ //tic - milisekunda
	uint32_t tp = HAL_GetTick(); //systick ma 24 bity
	 while(HAL_GetTick()-tp <tm){
	 }
	 return;
}
*/
