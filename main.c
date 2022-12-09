//N503BS CC1100E module from www.coralradio.com.
//N503BS CC1100E module CW test.
#include "msp430x21x2.h"
#include "CC1100E.h"

#define COUNT               10
#define TIME_COUNT          500

#define CCxxx0_WRITE_BURST  0x40
#define CCxxx0_READ_SINGLE  0x80
#define CCxxx0_READ_BURST   0xC0

void Init_Uart(void);
void Uart_SendString(char string[],unsigned int len);
void Setup_SPI(void);
void SPI_write(unsigned char value);
unsigned char SPI_read(void);
char halSpiReadReg(unsigned char addr);
char halSpiReadStatus(char addr);

void halSpiWriteReg(unsigned char addr,unsigned char value);
void halSpiStrobe(unsigned char strobe);
void halRfWirteRfSettings_CC2500(void);
void TI_CC_Wait(unsigned int cycles);

unsigned char temp;
unsigned char data_buf[40];
unsigned char data_len = 0;
unsigned char data_flag = 0;


void Setup_SPI(void)
{
  P2DIR |= 0x01;
  P3OUT |= 0x01;                           // P3.3,2,1 USCI_B0 option select.P3SEL |= 0x11;//P3.0,4 USCI_A0 option select
  P3DIR |= 0x01;
  UCB0CTL0 |= UCMSB + UCMST + UCSYNC + UCCKPL;       // 3-pin, 8-bit SPI mstr, MSB 1st
  UCB0CTL1 |= UCSSEL_2;                     // MCLK
  UCB0BR0 = 0x02;
  UCB0BR1 = 0;
  P3SEL |= 0x0E; 
  P3DIR |= 0x0A;
  UCB0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
}

void SPI_write(unsigned char value)
{
  IFG2 &= ~UCB0RXIFG;
  UCB0TXBUF = value;
  while (!(IFG2 & UCB0RXIFG));
}

unsigned char SPI_read(void)
{
  unsigned char Value;
  UCB0TXBUF = 0x00;
  while (!(IFG2 & UCB0RXIFG));
  Value = UCB0RXBUF;
  return Value;
}

char halSpiReadReg(unsigned char addr)
{
  char temp;
  P3OUT &= ~0x01;
  while (!(IFG2&UCB0TXIFG));                // Wait for TX to finish
  UCB0TXBUF = (addr | CCxxx0_READ_SINGLE);// Send address
  while (!(IFG2&UCB0TXIFG));                // Wait for TX to finish
  UCB0TXBUF = 0;                            // Dummy write so we can read data
  // Address is now being TX'ed, with dummy byte waiting in TXBUF...
  while (!(IFG2&UCB0RXIFG));                // Wait for RX to finish
  // Dummy byte RX'ed during addr TX now in RXBUF
  IFG2 &= ~UCB0RXIFG;                       // Clear flag set during addr write
  while (!(IFG2&UCB0RXIFG));                // Wait for end of dummy byte TX
  // Data byte RX'ed during dummy byte write is now in RXBUF
  temp = UCB0RXBUF; 
  P3OUT |= 0x01;
  return temp;
}

char halSpiReadStatus(char addr)
{
  char temp;
  P3OUT &= ~0x01;
  while(P3IN & 0x04);
  IFG2 &= ~UCB0RXIFG;
  UCB0TXBUF = (addr | CCxxx0_READ_BURST);
  while (!(IFG2&UCB0RXIFG));                // Wait for TX to finish
  IFG2 &= ~UCB0RXIFG;                       // Clear flag set during last write
  UCB0TXBUF = 0;                            // Dummy write so we can read data
  while (!(IFG2&UCB0RXIFG));                // Wait for RX to finish
  temp = UCB0RXBUF;                            // Read data
  P3OUT |= 0x01;
  return temp; 
}

void halSpiWriteReg(unsigned char addr,unsigned char value)
{
  P3OUT &= ~0x01;                //Enable CC2500;
  while(P3IN & 0x04);
  SPI_write(addr);
  SPI_write(value);
  P3OUT |= 0x01;                 //Disable CC2500;
}

void halSpiStrobe(unsigned char strobe)
{
  IFG2 &= ~UCB0RXIFG;
  P3OUT &= ~0x01;                //Enable CC2500;
  while(P3IN & 0x04);
  UCB0TXBUF = strobe;
  while(!(IFG2&UCB0RXIFG));
  P3OUT |= 0x01;                 //Disable CC2500;
}

void halRfWirteRfSettings_CC1100E(void)  
{
  halSpiWriteReg(CCxxx0_FREQ2,    0x12);  //490MHZ
  halSpiWriteReg(CCxxx0_FREQ1,    0xD8);
  halSpiWriteReg(CCxxx0_FREQ0,    0x9D);
  halSpiWriteReg(CCxxx0_PKTLEN,   0xFF);			//1
  halSpiWriteReg(CCxxx0_FIFOTHR,  0x0f);
  halSpiWriteReg(CCxxx0_PKTCTRL1, 0x04);			//2     
  halSpiWriteReg(CCxxx0_PKTCTRL0, 0x05);			//3
	halSpiWriteReg(CCxxx0_FSCTRL1,  0x0B);			//4
	halSpiWriteReg(CCxxx0_FSCTRL0,  0x00);			//5
	halSpiWriteReg(CCxxx0_MDMCFG4,  0xFA);   		//
	halSpiWriteReg(CCxxx0_MDMCFG3,  0x93);			//8
	halSpiWriteReg(CCxxx0_MDMCFG2,  0x93);			//9
  halSpiWriteReg(CCxxx0_MDMCFG1,  0x22);			//10
  halSpiWriteReg(CCxxx0_MDMCFG0,  0xF8);			//11	
  halSpiWriteReg(CCxxx0_CHANNR,   0x00);			//6
	halSpiWriteReg(CCxxx0_DEVIATN,  0x35);			//12
	halSpiWriteReg(CCxxx0_FREND0,   0x10);
	halSpiWriteReg(CCxxx0_FREND1,   0x56);

  halSpiWriteReg(CCxxx0_MCSM2 ,   0x07); //CCA enbale
  halSpiWriteReg(CCxxx0_MCSM1 ,   0x30); //0x30 CCA enbale

  halSpiWriteReg(CCxxx0_MCSM0 ,   0x18); //
  halSpiWriteReg(CCxxx0_FOCCFG,   0x16);			//16
  halSpiWriteReg(CCxxx0_BSCFG,    0x6C);			//17
	halSpiWriteReg(CCxxx0_AGCCTRL2, 0x43);			//18
  halSpiWriteReg(CCxxx0_AGCCTRL1, 0x40);			//19
  halSpiWriteReg(CCxxx0_AGCCTRL0, 0x91);			//20
  halSpiWriteReg(CCxxx0_FSCAL3,   0xA9);			//23
  halSpiWriteReg(CCxxx0_FSCAL2,   0x0A);			//24
  halSpiWriteReg(CCxxx0_FSCAL1,   0x00);			//25
  halSpiWriteReg(CCxxx0_FSCAL0,   0x11);			//26
  halSpiWriteReg(CCxxx0_FSTEST,   0x59);			//27
  halSpiWriteReg(CCxxx0_TEST2,    0x88);			//30
  halSpiWriteReg(CCxxx0_TEST1,    0x31);			//31
  halSpiWriteReg(CCxxx0_TEST0,    0x0B);			//32

  halSpiWriteReg(CCxxx0_IOCFG2,  0x0B);
  halSpiWriteReg(CCxxx0_IOCFG0,  0x06);
}

void POWER_UP_RESET(void)
{
  P3OUT |= 0x01;
  TI_CC_Wait(60);
  P3OUT &= ~0x01;
  TI_CC_Wait(60);
  P3OUT |= 0x01;
  TI_CC_Wait(90);
  
  P3OUT &= ~0x01;
  while(P3IN & 0x04);
  UCB0TXBUF = CCxxx0_SRES;
  IFG2 &= ~UCB0RXIFG;
  while(!(IFG2 & UCB0RXIFG));
  while(P3IN & 0x04);
  P3OUT |= 0x01;
}
void TI_CC_Wait(unsigned int cycles)
{
  while(cycles>15)                          // 15 cycles consumed by overhead
    cycles = cycles - 6;                    // 6 cycles consumed each iteration
}
void SPIWriteBurstReg(char addr, char *buffer, char count)
{
  char i;
  P3OUT &= ~0x01;
  while(P3IN & 0x04);
  IFG2 &= ~UCB0RXIFG;
  UCB0TXBUF = addr | CCxxx0_WRITE_BURST;
  while(!(IFG2 & UCB0RXIFG));
  for(i = 0;i < count; i++)
  {
    IFG2 &= ~UCB0RXIFG;
    UCB0TXBUF = buffer[i];
    while(!(IFG2 & UCB0RXIFG));
  }
  P3OUT |= 0x01;
}

main( void )
{
  char paTable[] = {0xFF};
  char paTableLen = 1;
  unsigned int i,j,k;
  char txBuffer[40] = {0x27,0x55,0x7A,0x20,0x60,0xff,0xff,0xff,0xff,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x00,0x01,0x02,0x03,0x04,0x05,0xff,0xff,0xff,0xff,0xb0};
  char txBuffer1[4] = {0x03,0x01,0x01,0x00};
  WDTCTL = WDTPW + WDTHOLD;
  
  do
  {
     IFG1&=~OFIFG;
     for(i=250;i>0;i--);
  }
   while((IFG1&OFIFG)==OFIFG);   //when OSCFault＝1, waiting
  BCSCTL2 = SELM1 +SELM0 + SELS;
  
  //--------------------------
  Init_Uart();
  Setup_SPI();
  for(i = 100; i > 0;i--);
  POWER_UP_RESET();

  halRfWirteRfSettings_CC1100E();
  paTable[0] = 0xC2;  //CC1100E MAX OUTPUT POWER, 0xC2
  SPIWriteBurstReg(CCxxx0_PATABLE,paTable,paTableLen);

  halSpiStrobe(CCxxx0_SIDLE);
  halSpiStrobe(CCxxx0_STX);
}
