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
    // Write(addr);
    // GPIOB->regs->BSRR = (1U << 6) << (16 * LOW);    //WR LOW
    // GPIOC->regs->BSRR = (1U << 14) << (16 * setA1); //A1 setA1
    // GPIOC->regs->BSRR = (1U << 13) << (16 * LOW);    //A0 LOW
    // GPIOA->regs->BSRR = (1U << 9) << (16 * LOW);   //CS LOW
    // delayMicroseconds(4);                           //Usually needs about 2.2uS, but we'll use 4uS to stay on the safe side
    // GPIOA->regs->BSRR = (1U << 9) << (16 * HIGH); //CS HIGH
    // //Possible WR reset here to high then back to low after write?
    // Write(data);
    // GPIOC->regs->BSRR = (1U << 13) << (16 * HIGH);    //A0 HIGH
    // GPIOA->regs->BSRR = (1U << 9) << (16 * LOW);   //CS LOW
    // delayMicroseconds(4); 
    // GPIOA->regs->BSRR = (1U << 9) << (16 * HIGH);   //CS HIGH
    // GPIOB->regs->BSRR = (1U << 6) << (16 * HIGH);    //WR HIGH


    
    digitalWrite(A1, setA1); //A1 setA1
    digitalWrite(A0, LOW);   //A0 LOW
    Write(addr);
    digitalWrite(WR, LOW);    //WR LOW
    digitalWrite(CS, LOW);   //CS LOW
    delayMicroseconds(3);
    digitalWrite(WR, HIGH);
    digitalWrite(CS, HIGH);   //CS HIGH
    digitalWrite(A0, HIGH);   //A0 HIGH
    Write(data);
    digitalWrite(WR, LOW);    //WR LOW
    digitalWrite(CS, LOW);   //CS LOW
    delayMicroseconds(3);
    digitalWrite(WR, HIGH);    //WR HIGH
    digitalWrite(CS, HIGH);   //CS LOW


}

void OPL3::Reset()
{
    digitalWrite(IC, LOW);
    delayMicroseconds(25);
    digitalWrite(IC, HIGH);
    delayMicroseconds(25);
    Send(0x05, 1, 1); //Set the OPL mode. Write 1 to this address for OPL3, 0 for OPL2. TODO: Detect chip from VGM clock
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