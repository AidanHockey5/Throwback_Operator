#include "OPL3.h"

union U_32
{
    uint32_t fullWord;
    uint8_t bytes[4]; //0-1 EMPTY, 2 HIGH, 3 LOW
} port;

void Write(uint8_t data)
{
    // port.fullWord = GPIOB->regs->ODR; //Grab current state of the port
    // port.bytes[2] = data;       //Stuff data byte into section where GPIO bus is
    // GPIOB->regs->ODR = port.fullWord; //Send it back to the port
    //data = ~data;
    GPIOB->regs->BSRR = (1U << 8) << (16 * ((data >> 0)&1));
    GPIOB->regs->BSRR = (1U << 9) << (16 * ((data >> 1)&1));
    GPIOB->regs->BSRR = (1U << 10) << (16 * ((data >> 2)&1));
    GPIOB->regs->BSRR = (1U << 11) << (16 * ((data >> 3)&1));
    GPIOB->regs->BSRR = (1U << 12) << (16 * ((data >> 4)&1));
    GPIOB->regs->BSRR = (1U << 13) << (16 * ((data >> 5)&1));
    GPIOB->regs->BSRR = (1U << 14) << (16 * ((data >> 6)&1));
    GPIOB->regs->BSRR = (1U << 15) << (16 * ((data >> 7)&1));
}

void OPL3::Send(uint8_t addr, uint8_t data, bool setA1)
{
    // GPIOA->regs->BSRR = (1U << 9) << (16 * HIGH);   //CS HIGH
    // GPIOB->regs->BSRR = (1U << 6) << (16 * LOW);    //WR LOW
    // GPIOC->regs->BSRR = (1U << 13) << (16 * LOW);    //A0 LOW
    // GPIOB->regs->BSRR = (1U << 14) << (16 * setA1); //A1 setA1
    // Write(addr);
    // GPIOA->regs->BSRR = (1U << 9) << (16 * LOW);   //CS LOW
    // delayMicroseconds(4); 
    // GPIOA->regs->BSRR = (1U << 9) << (16 * HIGH);   //CS HIGH
    // GPIOC->regs->BSRR = (1U << 13) << (16 * HIGH);    //A0 HIGH
    // Write(data);
    // GPIOA->regs->BSRR = (1U << 9) << (16 * LOW);   //CS LOW
    // delayMicroseconds(4); 
    // GPIOA->regs->BSRR = (1U << 9) << (16 * HIGH);   //CS HIGH
    // GPIOB->regs->BSRR = (1U << 6) << (16 * HIGH);    //WR HIGH
    // GPIOC->regs->BSRR = (1U << 13) << (16 * HIGH);    //A0 HIGH

    Write(addr);
    GPIOB->regs->BSRR = (1U << 6) << (16 * LOW);    //WR LOW
    GPIOC->regs->BSRR = (1U << 14) << (16 * setA1); //A1 setA1
    GPIOC->regs->BSRR = (1U << 13) << (16 * LOW);    //A0 LOW
    GPIOA->regs->BSRR = (1U << 9) << (16 * LOW);   //CS LOW
    delayMicroseconds(4);                           //Usually needs about 2.2uS, but we'll use 4uS to stay on the safe side
    GPIOA->regs->BSRR = (1U << 9) << (16 * HIGH);
    //Possible WR reset here to high then back to low after write?
    Write(data);
    GPIOC->regs->BSRR = (1U << 13) << (16 * HIGH);    //A0 HIGH
    GPIOA->regs->BSRR = (1U << 9) << (16 * LOW);   //CS LOW
    delayMicroseconds(4); 
    GPIOA->regs->BSRR = (1U << 9) << (16 * HIGH);   //CS HIGH
    GPIOB->regs->BSRR = (1U << 6) << (16 * HIGH);    //WR HIGH
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
    pinMode(IC, OUTPUT);
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