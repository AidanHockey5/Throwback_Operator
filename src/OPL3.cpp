#include "OPL3.h"

union U_32
{
    uint32_t fullWord;
    uint8_t bytes[4]; //0-1 LOW+HIGH, 2-3 EMPTY
} port;

void Write(uint8_t data)
{
    port.fullWord = GPIOB->regs->ODR; //Grab current state of the port
    port.bytes[1] = data;       //Stuff data byte into section where the 8-bit GPIO data bus is
    GPIOB->regs->ODR = port.fullWord; //Send it back to the port
}

void OPL3::Send(uint8_t addr, uint8_t data, bool setA1)
{
    GPIOC->regs->BSRR = (1U << 14) << (16 * !setA1);    //A1 setA1
    GPIOC->regs->ODR &= ~(0x2000);                      //A0 LOW
    Write(addr);
    GPIOB->regs->ODR &= ~(0x40);                        //WR LOW
    GPIOA->regs->ODR |= 0x200;                          //CS LOW (OPEN_DRAIN : HIGH)
    delayMicroseconds(2);
    GPIOB->regs->ODR |= 0x40;                           //WR HIGH
    GPIOA->regs->ODR &= ~(0x200);                       //CS HIGH (OPEN_DRAIN : LOW)
    GPIOC->regs->ODR |= 0x2000;                         //A0 HIGH
    Write(data);
    GPIOB->regs->ODR &= ~(0x40);                        //WR LOW
    GPIOA->regs->ODR |= 0x200;                          //CS LOW (OPEN_DRAIN : HIGH)
    delayMicroseconds(2);
    GPIOB->regs->ODR |= 0x40;                           //WR HIGH
    GPIOA->regs->ODR &= ~(0x200);                       //CS HIGH (OPEN_DRAIN : LOW)
}

 void OPL3::SetOPLMode(bool isOPL3)
 {
    Reset();
    Send(0x05, 1, isOPL3); //Set the OPL mode. Write 1 to this address for OPL3, 0 for OPL2. TODO: Detect chip from VGM clock
    delayMicroseconds(5);
 }

void OPL3::Reset()
{
    digitalWrite(IC, LOW);
    delayMicroseconds(25);
    digitalWrite(IC, HIGH);
    delayMicroseconds(25);
}

OPL3::OPL3()
{
    pinMode(IC, OUTPUT_OPEN_DRAIN);
    pinMode(CS, OUTPUT_OPEN_DRAIN);
    pinMode(A0, OUTPUT);
    pinMode(A1, OUTPUT);
    pinMode(WR, OUTPUT);
    pinMode(RD, OUTPUT);

    digitalWrite(IC, HIGH);
    digitalWrite(CS, HIGH);
    digitalWrite(A0, HIGH);
    digitalWrite(A1, HIGH);
    digitalWrite(WR, HIGH);
    digitalWrite(RD, HIGH);

    pinMode(PB8, OUTPUT);
    pinMode(PB9, OUTPUT);
    pinMode(PB10, OUTPUT);
    pinMode(PB11, OUTPUT);
    pinMode(PB12, OUTPUT);
    pinMode(PB13, OUTPUT);
    pinMode(PB14, OUTPUT);
    pinMode(PB15, OUTPUT);
}

OPL3::~OPL3()
{
}

/*
PB8   -   D0
PB9   -   D1
PB10 -   D2
PB11 -   D3
PB12 -   D4
PB13 -   D5
PB14 -   D6
PB15 -   D7

PC13   -   A0
PC14   -   A1

PB6   -  WR
PB7   -  RD

PA8   -   IC
PA9 -   CS
*/