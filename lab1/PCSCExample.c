#include <stdio.h>
#include <stdlib.h>

//prototypy funkcji PC/SC
#include <winscard.h>


//wielkosc bufora na nazwe czytnika
#define MAX_READER_NAME_SIZE 40

//w systemie Windows nie ma definicji maksymalnego rozmiaru ATR
#ifndef MAX_ATR_SIZE
#define MAX_ATR_SIZE 33
#endif

int main ( int argc , char **argv )
{
	//kontekst polaczenia do zarzadcy zasobow
	SCARDCONTEXT hContext ;
	//uchwyt polaczenia do czytnika
	SCARDHANDLE hCard ;
	//stan czytnika
	SCARD_READERSTATE_A rgReaderStates [ 1 ] ;
	//pomocnicze zmienne (dlugosci buforow, stan czytnika, protokol)
	DWORD dwReaderLen , dwState , dwProt , dwAtrLen ;
	DWORD dwPref , dwReaders , dwRespLen ;
	// bufor na nazwe czytnika
	LPSTR pcReaders ;
	//bufor na liste czytnikow
	LPSTR mszReaders ;
	//bufor na ATR
	BYTE pbAtr [MAX_ATR_SIZE ] ;
	//bufor na odpowiedz karty
	BYTE pbResp [ 10 ] ;
	//pomocnicze zmienne
	LPCSTR mszGroups ;
	LONG rv ;
	int i , p , iReader ;
	int iReaders [ 16 ] ;

	//komenda GET CHALLENGE
	BYTE GET_CHALLENGE [ ] = { 0x00 , 0x84 , 0x00 , 0x00 , 0x08 } ;
	
	//nawiazanie komunikacji z lokalnym zarzadca zasobow
	printf("SCardEstablishContext : ") ;
	rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL , NULL, &hContext ) ;
	
	if ( rv != SCARD_S_SUCCESS)
	{
		printf("failed\n") ;
		return -1;
	}
	else printf("success\n");
	
	//pobranie wielkosci ciagu, jaki bedzie potrzebny na liste
	//dostepnych czytnikow w systemie
	mszGroups = 0;
	printf("SCardListReaders : ");
	rv = SCardListReaders(hContext, mszGroups, 0, &dwReaders);
	
	if(rv != SCARD_S_SUCCESS)
	{
		SCardReleaseContext(hContext);
		printf("failed\n");
		return -1;
	}
	else printf("success\n");
	
	//alokacja pamieci
	mszReaders = (LPSTR) malloc(sizeof(char) * dwReaders);
	
	//pobranie listy czytnikow
	printf("SCardListReaders : ");
	rv = SCardListReaders(hContext, mszGroups, mszReaders, &dwReaders);

	if(rv != SCARD_S_SUCCESS)
	{
		SCardReleaseContext(hContext);
		free(mszReaders);
		printf("failed\n");
		return -1;
	}
	else printf("success\n");

	//wydruk listy znalezionych czytnikow
	p = 0;
	for(i = 0; i< dwReaders - 1; ++i)
	{
		iReaders[++p]=i;
		printf("Reader %02d: %s\n", p, &mszReaders[i]);
		//przesuniecie bufora do kolejnej nazwy czytnika
		while(mszReaders[++i]!='\0');
	}
	
	//wybor czytnika do dalszych operacji
	do
	{
		printf("Select reader : ");
		scanf("%d", &iReader);
	}
	while(iReader>p || iReader <=0);
	
	//wypelnienie struktury stanu czytnika (nazwa czytnika i jego stan)
	rgReaderStates[0].szReader = &mszReaders[iReaders[iReader]];
	rgReaderStates[0].dwCurrentState = SCARD_STATE_EMPTY;
	
	printf("Insert card...\n");
	
	//oczekiwanie na zmiane stanu czytnika (wlozenie karty)
	printf("SCardStatusChange : ");
	rv = SCardGetStatusChange(hContext, INFINITE, rgReaderStates, 1);
	
	printf("[%02d]\n", rv);
	
	//nawiazanie polaczenia z czytnikiem
	printf("SCardConnect : ");
	rv = SCardConnect(hContext, &mszReaders[iReaders[iReader]], 
		SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
		&hCard, &dwPref);
		
	if(rv!=SCARD_S_SUCCESS)
	{
		SCardReleaseContext(hContext);
		free(mszReaders);
		printf("failed\n");
		return -1;
	}
	else printf("success\n");
	
	//sprawdzenie stanu karty w czytniku
	printf("SCardStatus : ");
	dwReaderLen = MAX_READER_NAME_SIZE;
	pcReaders = (LPSTR) malloc(sizeof(char) * MAX_READER_NAME_SIZE);
	
	rv = SCardStatus(hCard, pcReaders, &dwReaderLen, &dwState,
		&dwProt, pbAtr, &dwAtrLen);
		
	if(rv != SCARD_S_SUCCESS)
	{
		SCardDisconnect ( hCard , SCARD_RESET_CARD) ;
		SCardReleaseContext ( hContext ) ;
		free ( mszReaders ) ;
		free ( pcReaders ) ;
		printf("failed\n") ;
		return -1;
	}
	else printf("success\n");
	
	// wydruk pobranych informacji
	printf("Reader name : %s\n" , pcReaders ) ;
	printf("Reader state : %lx\n" , dwState ) ;
	printf("Reader protocol : %lx\n" , dwProt - 1) ;
	printf("Reader ATR size : %d\n" , dwAtrLen ) ;
	printf("Reader ATR value : ") ;
	
	//wydruk ATR
	for(i = 0; i < dwAtrLen; i++)
	{
		printf("%02X ", pbAtr[i]);
	}
	printf("\n");
	free(pcReaders);
	
	//rozpoczecie transakcji z karta
	printf("SCardBeginTranscation : ");
	rv = SCardBeginTransaction ( hCard ) ;
	if( rv != SCARD_S_SUCCESS)
	{
		SCardDisconnect ( hCard , SCARD_RESET_CARD) ;
		SCardReleaseContext ( hContext ) ;
		printf("failed\n") ;
		free ( mszReaders ) ;
		return -1;
	}
	else printf("success\n") ;
	
	//przeslanie do karty komendy GET CHALLENGE
	printf("SCardTransmit : ");
	dwRespLen = 10;
	rv = SCardTransmit(hCard, SCARD_PCI_T0, GET_CHALLENGE,
		5, NULL, pbResp, &dwRespLen);
		
	if( rv != SCARD_S_SUCCESS)
	{
		SCardDisconnect ( hCard , SCARD_RESET_CARD) ;
		SCardReleaseContext ( hContext ) ;
		printf("failed\n") ;
		free ( mszReaders ) ;
		return -1;
	}
	else printf("success\n") ;
	printf("Response APDU : ");
	
	//wydruk odpowiedzi karty
	for(i = 0; i < dwRespLen; i++)
	{
		printf("%02X ", pbResp[i]);
	}
	printf("\n");
	
	//zakonczenie transmisji z karta
	printf("SCardEndTransaction : ");
	rv = SCardEndTransaction(hCard, SCARD_LEAVE_CARD);
	if(rv != SCARD_S_SUCCESS)
	{
		SCardDisconnect(hCard, SCARD_RESET_CARD);
		SCardReleaseContext ( hContext ) ;
		printf("failed\n") ;
		free ( mszReaders ) ;
		return -1;
	}
	else printf("success\n") ;
	
	//odlaczenie od czytnika
	printf("SCardDisconnect : ");
	rv = SCardDisconnect(hCard, SCARD_UNPOWER_CARD);
	
	if(rv != SCARD_S_SUCCESS)
	{
		SCardReleaseContext ( hContext ) ;
		printf("failed\n") ;
		free ( mszReaders ) ;
		return -1;
	}
	else printf("success\n") ;

	//odlaczenie od zarzadcy zasobow PS/SC
	printf("SCardReleaseContext : ");
	rv = SCardReleaseContext ( hContext ) ;
	
	if ( rv != SCARD_S_SUCCESS)
	{
		printf("failed\n") ;
		free ( mszReaders ) ;
		return -1;
	}
	else printf("success\n") ;
	
	//zwolnienie pamieci
	free(mszReaders);
	
	return 0;
}

//source rewritten from: http://home.elka.pw.edu.pl/~pnazimek/ipki.pdf
