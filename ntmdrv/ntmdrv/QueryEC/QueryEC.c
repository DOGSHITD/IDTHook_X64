#include<stdio.h>
#include<stdlib.h>
#include <conio.h>
#include <dos.h> 

typedef unsigned char UINT8;
typedef unsigned int  UINT16;
typedef unsigned long UINT32;

#define EC_SC     0x66
#define EC_DATA   0x62

void WriteCommand(UINT8 SMBAddr, UINT8 SMBData)
{
	while (_inp(EC_SC) & 0x02);
	_outp(EC_SC, 0x81);
	while (_inp(EC_SC) & 0x02);
	_outp(EC_DATA, SMBAddr);
	while (_inp(EC_SC) & 0x02);
	_outp(EC_DATA, SMBData);
}

UINT8 ReadCommand(UINT8 SMBAddr)
{
	UINT8 Data;
	while (_inp(EC_SC) & 0x02);
	_outp(EC_SC, 0x80);
	while (_inp(EC_SC) & 0x02);
	_outp(EC_DATA, SMBAddr);
	while (!(_inp(EC_SC) & 0x01));
	Data = _inp(EC_DATA);
	return Data;
}
int main(void)
{
	UINT8 Count, SMBDataBase = 0x1C;

	WriteCommand(0X19, 0X00);           //reset SMB_status
	WriteCommand(0x1A, 0x16);           //write SMB_Addr
	WriteCommand(0x1B, 0X20);           //SBS Command ManufacturerName
	WriteCommand(0X18, 0X0B);           //protocol
	while (ReadCommand(0X19) != 0x80);    //check status 
	Count = ReadCommand(0x3C);         //block count
	printf("the Count is %d\n", Count);

	for (int i = 0; i < Count; ++i)          // read
	{
		printf("%c", ReadCommand(SMBDataBase + i));
	}

	return 0;
}