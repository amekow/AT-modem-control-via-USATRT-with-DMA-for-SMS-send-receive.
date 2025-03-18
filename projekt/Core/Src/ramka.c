/*
 * ramka.c
 *
 *  Created on: Jan 7, 2025
 *      Author: amekow
 */

#include "ramka.h"
#include "usart.h"
#include "bufork.h"
#include "stdio.h"
#include "ctype.h"



uint8_t dma_transfer_complete = 1;
uint8_t tresc[161];
uint8_t czy_liczba(uint8_t * liczba){
	for(uint8_t i =0; i< strlen(liczba); i++){
		if (isdigit(liczba[i]) == 0) {
			return 1;
		}
	}
	return 0;
}
uint8_t czy_HEX_ok(uint8_t * liczba){
	uint8_t ok = 2;
	if((liczba[2]>='A'&&liczba[2]<='F')||(liczba[2]>='a'&&liczba[2]<='f')||(liczba[2]>='0'&&liczba[2]<='9')){
		ok --;
	}
	if((liczba[3]>='A'&&liczba[3]<='F')||(liczba[3]>='a'&&liczba[3]<='f')||(liczba[3]>='0'&&liczba[3]<='9')){
		ok --;
	}
	return ok;
}

// inicjowanie SIM800


void initSIM800() {
    sendMA("AT");
    char pom[10];
	while (rxBufor1.start == rxBufor1.end);  // zaczekaj na odpowiedź SIM800
    odczytSIM(pom);
    uint32_t tp = HAL_GetTick();
    while (strstr(pom, "OK") == NULL) {
        sendMA("AT");
    	while (rxBufor1.start == rxBufor1.end);  // zaczekaj na odpowiedź SIM800
        odczytSIM(pom);
        if(HAL_GetTick()-tp <30000){ //jeśli próbuję pół minuty
        	HAL_UART_Transmit(&huart2, "Connection failed\n", 18, 1500);
//        	sendf("Connection failed\n");
        	HAL_NVIC_SystemReset(); //reset mikroprocesora
        }
    }
    sendMA("ATE0");  // Żeby moduł GSM nie zwracał wysłanej komendy
	while (rxBufor1.start == rxBufor1.end);  // zaczekaj na odpowiedź SIM800
    odczytSIM(pom);
    sendMA("AT+CMGF=1"); //utawienie w tryb tekstowy
	while (rxBufor1.start == rxBufor1.end);  // zaczekaj na odpowiedź SIM800
    odczytSIM(pom);
    uint8_t buff1[40];
    uint8_t buff2[20];
    sendMA("AT+CPIN?");
	while (rxBufor1.start == rxBufor1.end);  // zaczekaj na odpowiedź SIM800
    odczytSIM(&buff1);
    while(strncmp(&buff1, "+CPIN: READY", 12)!= 0){
    	uint8_t puk[9];
    	uint8_t pukpin[18];
    	uint8_t pin[9];

    	if(strcmp(&buff1, "+CPIN: SIM PUK")== 0){
    		sendf("PUK & PIN required\n");
    	  	if (dekoduj_ramke(&pukpin)!=0){
    	  		continue;
    	    }
    		if(pukpin[11]!=':' || pukpin[2]!=':'){
    			sendf("Error");
    		    continue;
    		}
    		wybierz(&pukpin, &puk, 2);
    		if(strcmp(pukpin, "PU")!= 0){
    			sendf("Invalid command\n");
    			continue;
    		}
    		wybierz(&pukpin+3, &puk, 8);
    		wybierz(&pukpin+12, &pin, strlen(&pukpin)-12);
    		if(czy_liczba(&puk)==1){
    			sendf("Wrong PUK\n");
    		}
    		if(czy_liczba(&pin)==1){
    			sendf("Wrong PIN\n");
    		}
    		sprintf(&buff2, "AT+CPIN=\"%s\",\"%s\"",&puk,&pin);
    		sendMA(&buff2);
    		while (rxBufor1.start == rxBufor1.end);  // zaczekaj na odpowiedź SIM800
    		odczytSIM(&buff1);
    	}else if(strcmp(&buff1, "+CPIN: SIM PIN")== 0||(strcmp(&buff1, "ERROR")== 0)){
    		sendf("PIN required\n");
    	  	if (dekoduj_ramke(&pukpin)!=0){
    	  		continue;
    	    }
    		if(pukpin[2]!=':'){
    			sendf("Error");
    			continue;
    		}
    	    wybierz(&pukpin, &pin, 2);
    	    if(strcmp(pukpin, "PI")!= 0){
    	    	sendf("Invalid command\n");
    	        continue;
    	    }
    	    wybierz(&pukpin+3, &pin, strlen(&pukpin)-3);
    		if(czy_liczba(&pin)==1){
    			sendf("Wrong PIN\n");
    		}
    	    sprintf(&buff2, "AT+CPIN=\"%s\"",&pin);
       		sendMA(&buff2);
       		while (rxBufor1.start == rxBufor1.end);  // zaczekaj na odpowiedź SIM800
    	    odczytSIM(&buff1);
    	}
    	sendMA("AT+CPIN?");
    	while (rxBufor1.start == rxBufor1.end);  // zaczekaj na odpowiedź SIM800
    	odczytSIM(&buff1);
    }

    // Komenda powoduje, że przychodzący SMS zapisany jest na karcie SIM a UART zwróci: +CMTI: "SM",<nr SMS w pamięci>
    sendMA("AT+CNMI=1,1,0,0,0");
	while (rxBufor1.start == rxBufor1.end);  // zaczekaj na odpowiedź SIM800
	odczytSIM(&buff1);

    // Poniższa komenda powoduje, że SMS nie zapisuje się na karcie pamięci, ale jest bezpośrednio zwracany przez UART
    // i można go wyświetlić.
//      sendMA("AT+CNMI=1,2,0,0,0");

//	  scanDMA(&recBuff1);
//      sendf("OK\n");

}

void sendMA(char* message){
	uint8_t pom[1024];
	HAL_StatusTypeDef stat;

	sprintf((char *)&pom, "%s\r\n", message);
//	while (dma_transfer_complete == 0);
//	dma_transfer_complete = 0;
	stat = HAL_UART_Transmit_DMA(&huart1,&pom,strlen(pom));
}

void odczytSIM(uint8_t* pom){
	uint16_t i = 0;

	while (rxBufor1.start != rxBufor1.end && isspace(rxBufor1.bufor[rxBufor1.end])){
		// początkowe białe znaki nie są zapisywane do bufora wynikowego
		bufork_odczyt(&rxBufor1, &pom[0]);
	}
	while (bufork_odczyt(&rxBufor1, &pom[i++]) == 0);
	pom[i-1] = 0; //bo i mimo wszystko się zwiększyło to trzeba je zmniejszyć
	i = strlen(pom) - 1;
    while (isspace(pom[i])) {
    	// Usunięcie białych znaków z końca bufora
    	pom[i--] = 0;
    }
//	dma_transfer_complete = 1;

}

uint8_t dopisz_znak_ramki(uint8_t* ramka) {
	if(rxBufor2.start==rxBufor2.end){
		// nie ma znaku do odczytania
		return 2;
	}
	uint8_t znak;
	uint8_t len_r = strlen(ramka);
	bufork_odczyt(&rxBufor2, &znak);
	if (len_r == 0 && znak == '@') { //if - służy do rozpoznania początku ramki - ignorowania wszystkich przed
		ramka[0] = znak;
		ramka[1] = 0;
	}else if(len_r>0){ //obsługa znaków po początku ramki
		if (len_r==1 && ramka[0] == '@') {
			// znak początku ramki w buforze ramka zostanie zastąpiony pierwszym znakiem po znaku początku
			len_r = 0;
		}
		if (znak == '@') {
			// Pojawił się znak początku ramki, a nie było znaku końca - więc do tej pory odczytano śmieci
			ramka[0] = '@';
			ramka[1] = 0;
		} else if (znak == ';') {
			// Odczytano znak końca ramki - można obsłużyć
			uint8_t crc[5]="0x  ";
			uint8_t obliczone_crc, przeslane_crc, i;
			uint8_t nadawca[3];
			uint8_t odbiorca[3];
			uint8_t komenda[200];
			// sprawdzenie crc
			crc[2]=ramka[len_r-2];
			crc[3]=ramka[len_r-1];
			if(czy_HEX_ok(&crc)!=0){
				sendf("Data corruption\n");
				return 2;
			}
			ramka[len_r-2]=0;
			przeslane_crc = strtol(&crc,NULL,0);
			obliczone_crc = oblicz_crc(ramka, strlen(ramka));
			  //sprawdzenie CRC
			if (obliczone_crc != przeslane_crc) {
				  // Obsługa błędu CRC
				sendf("Data corruption\n");
				ramka[0] = 0;
				return 2;
			  } else {
				  // CRC OK, można działać
				  wybierz(ramka, nadawca, 3);
				  wybierz(ramka+3, odbiorca, 3);
				  if(/*strcmp(nadawca, "TER")== 0 &&*/ strcmp(odbiorca, "GSM")!= 0){
					  sendf("Recipient unknown\n");
	  				  ramka[0] = 0;
	  				  return 2;
				  } else {
					  // Pobrano caą ramkę i jest OK
					  return 0;
				  }

			  }
		} else {
			// Nadszedł kolejny znak ramki
			if(ramka[len_r-1]=='^') {
				if (znak=='{') {
					ramka[len_r-1] = '@';
					ramka[len_r] = 0;
				} else if(znak=='}'){
					ramka[len_r-1] = ';';
					ramka[len_r] = 0;
				} else if(znak=='#'){
					ramka[len_r-1] = '^';
					ramka[len_r] = 0;
				}else{
					sendf("Encoding error\n");
					return 2;
				}
			} else {
				ramka[len_r] = znak;
				ramka[len_r+1] = 0;
			}
		}

	}
	return 1;

}


uint8_t dekoduj_ramke( uint8_t* komenda ){
	uint8_t ramka[BUF_LEN];
	uint8_t pom;

	ramka[0]=0;
	while (rxBufor2.start != rxBufor2.end){
		pom = dopisz_znak_ramki(&ramka);
	}
	if (pom== 0) {
		// Odczytano prawidłowo ramkę
		wybierz(&ramka[6],komenda ,strlen(ramka)-6);
		return 0;
	} else {
		// ramka nieprawidłowa
		sendf('Error data');
		return 1;
	}
}


void sendSIM(uint8_t* kom) {
	bufork_t tmp_txBuf = txBufor1; //zmienna typu strutury tymczasowo wykorzystywana do zapisu bufora kołowego
	for (int i =0; i < strlen(kom); i++) {
		bufork_zapisz(&tmp_txBuf, kom[i]); //zapisuje w buforze docelowym, tylko jeszcze nie zmienia zmiennej
	}
	txBufor1.start = tmp_txBuf.start;
}

/*OBSLUGA KOMEND*/
/*ctrl z \x1A*/
void obsluga_komend(uint8_t* komenda){
//	uint8_t komenda[BUF_LEN];
	uint8_t ramka[BUF_LEN];

//	if(dekoduj_ramke((uint8_t*)&komenda)==1){
//		return;
//	}
//	wybierz((uint8_t*)&ramka[6], (uint8_t*)&komenda, strlen((char*)&ramka) - 6);
	//sendMA("test\n");
	if(strlen((char*)komenda)==2){

/**********Odczyt wszystkich sms zapisanych w pamięci*/
		if(strcmp((char*)komenda, "RR")== 0){
			sendSIM("AT+CMGL=\"ALL\"\r");
//			sendMA("AT+CNUM");
			//wyświetlenie wszystkich SMS-ów
		}else if(strcmp(komenda, "RM")== 0){
			//usunięcie wszystkich wiadomosci
			sendSIM("AT+CMGDA=\"DEL ALL\"\r");
		}
	}else if(komenda[2]/**(komenda+2)*/==':'){
		uint8_t rozkaz[12];
		//uint8_t parametr;
		wybierz(komenda,&rozkaz,2);

/************************************wysyłanie sms*/
		if(strcmp(rozkaz, "RS")== 0){
			char tmp[200];
			char odp[100];
//			sendMA("RS\n");
			uint8_t tel[13];

			if(komenda[3]=='+'){
				if(komenda[15]==':'){
					wybierz(komenda+3, &tel,12);
					wybierz (komenda + 16 ,&tresc,strlen(komenda)-16 );

					}
				}else{
					if(komenda[12]==':'){
						tel[0]='+';
						tel[1]='4';
						tel[2]='8';
						wybierz(komenda+3,&tel[3],9);
						wybierz (komenda + 13 ,tresc,strlen(komenda)-13 );
					}
				}
				if(czy_liczba(&tel[1])==1){
    				sendf("Error\n");
    				return;
    			}
				if (strlen(tresc) > 0)
				sprintf(&tmp, "AT+CMGS=\"%s\"\r", tel);
				sendSIM(&tmp);
		}else{
			uint8_t parametr[5];
			char tmp[20];
//			strncpy(&parametr, komenda+3,strlen(komenda)-3);
			wybierz(komenda + 3, &parametr,strlen(komenda)-3 );
			if(strcmp(rozkaz, "RR")==0){

/*******************************Odczyt sms przychodzacych*/
				if(strcmp(parametr, "i")==0){
					//odczytanie sms przychodzących
					sendSIM("AT+CMGL=\"REC UNREAD\"\n");
					sendSIM("AT+CMGL=\"REC READ\"\n");
/****************************Odczyt SMS o zadanym indeksie*/
				}else{
					if(czy_liczba(&parametr)==1){
						sendf("Invalid command\n");
						return;
					}
					sprintf(&tmp, "AT+CMGR=%s\r",&parametr);
					sendSIM(&tmp);
					//wyswietlenie sms o okreslonym indexie
				}
/**************************Usuniecie SMS o zadanym indeksie*/
			}else if(strcmp(rozkaz, "RM")==0){
				//usuniecie o zadanym indexie
				sprintf(&tmp, "AT+CMGD=%s\n",&parametr);
				sendSIM(&tmp);
			}else{
				sendf("Invalid command\n");
					//nie rozpoznano rozkazu
				}
			}
		}


	}


/*RZECZY ZWIAZANE Z KOMENDAMI**/

/*CRC*/
uint8_t oblicz_crc(const uint8_t *buf, uint8_t len){
	 uint8_t crc = CRC_INITIAL;
	    for (uint8_t i = 0; i < len; i++) {
	        crc ^= buf[i]; // XOR z bieżącym bajtem
	        for (uint8_t bit = 0; bit < 8; bit++) {
	            if (crc & 0x80) {
	                crc = (crc << 1) ^ POLYNOMIAL; // XOR z wielomianem jeśli bit najbardziej na lewo = 1
	            } else {
	                crc <<= 1; // Przesunięcie w lewo
	            }
	        }
	    }
	    return crc;
}

/*DZIAŁANIA NA STRINGACH*/

void wybierz(uint8_t* str,uint8_t* sub,  uint8_t len) { //z tablicy str bierze len pierwszych znaków i umieszcza do sub
	strncpy(sub, str, len);
	sub[len] = 0;
}
