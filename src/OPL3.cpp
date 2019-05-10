#include "OPL3.h"

union U_32
{
    uint32_t fullWord;
    uint8_t bytes[4]; //0-1 EMPTY, 2 HIGH, 3 LOW
} port;

void Write(uint8_t data)
{
    port.fullWord = GPIOB->regs->ODR; //Grab current state of the port
    port.bytes[2] = data;       //Stuff data byte into section where GPIO bus is
    GPIOB->regs->ODR = port.fullWord; //Send it back to the port
}

void OPL3::Send(uint8_t addr, uint8_t data, bool setA1)
{
    Write(addr);
    GPIOB->regs->BSRR = (1U << 6) << (16 * LOW);    //WR LOW
    GPIOB->regs->BSRR = (1U << 4) << (16 * !setA1); //A1 setA1
    GPIOB->regs->BSRR = (1U << 3) << (16 * LOW);    //A0 LOW
    GPIOA->regs->BSRR = (1U << 15) << (16 * LOW);   //CS LOW
    delayMicroseconds(4);                           //Usually needs about 2.2uS, but we'll use 4uS to stay on the safe side
    GPIOA->regs->BSRR = (1U << 15) << (16 * HIGH);
    //Possible WR reset here to high then back to low after write?
    Write(data);
    GPIOB->regs->BSRR = (1U << 3) << (16 * HIGH);    //A0 HIGH
    GPIOA->regs->BSRR = (1U << 15) << (16 * LOW);   //CS LOW
    delayMicroseconds(4); 
    GPIOA->regs->BSRR = (1U << 15) << (16 * HIGH);   //CS HIGH
    GPIOB->regs->BSRR = (1U << 6) << (16 * HIGH);    //WR HIGH
}

void OPL3::Reset()
{
    digitalWrite(IC, LOW);
    delayMicroseconds(25);
    digitalWrite(IC, HIGH);
}

OPL3::OPL3()
{
    pinMode(IC, OUTPUT);
    pinMode(CS, OUTPUT);
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

PB3   -   A0
PB4   -   A1

PB6   -  WR
PB7   -  RD

PA8   -   IC
PA15 -   CS
*/