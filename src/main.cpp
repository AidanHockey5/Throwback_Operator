#include <Arduino.h>
#include <libmaple/iwdg.h>
#include <SPI.h>
#include "SdFat.h"
#include <U8g2lib.h>
#include "TrackStructs.h"
#include "SerialUtils.h"
#include "VGMEngine.h"

//Debug variables
//REMOVE IN PCB VERSION
#define ENABLE_DEBUG_PORT_PIN PB3 //If this pin is HIGH, enable the debug port pins, otherwise, disable the pins. Keep pin low by default. Disables STlink when low.
//REMOVE IN PCB VERSION
#define DEBUG false //Set this to true for a detailed printout of the header data & any errored command bytes
#define DEBUG_LED PC15

//Prototypes
void setup();
void loop();
//void setClock(uint32_t frq);
void handleSerialIn();
void tick();
void removeMeta();
void handleButtons();
void startISR();
void pauseISR();
void drawOLEDTrackInfo();
bool startTrack(FileStrategy fileStrategy, String request = "");

//Sound Chip
OPL3 opl3; //Pins are already self-contained in this driver class.

//SD & File Streaming
SdFat SD;
File file;
#define MAX_FILE_NAME_SIZE 128
char fileName[MAX_FILE_NAME_SIZE];
uint32_t numberOfFiles = 0;
uint32_t currentFileNumber = 0;

//Counters
uint32_t bufferPos = 0;
uint32_t cmdPos = 0;
uint16_t waitSamples = 0;

//VGM Variables
uint16_t loopCount = 0;
const uint8_t maxLoops = 3;
bool fetching = false;
bool trackError = false;
//PlayMode playMode = PlayMode::IN_ORDER;

//IO
const uint8_t next_btn = PB3;
const uint8_t prev_btn = PB0;
const uint8_t rand_btn = PB1;
const uint8_t option_btn = PB4;

//OLED
U8G2_SSD1306_128X32_UNIVISION_F_SW_I2C u8g2(U8G2_R0, PB6, PB7, U8X8_PIN_NONE); //SW I2C for now, HW seems to have a bug that crashes the unit
bool isOledOn = true;

void setup()
{
  //init watchdog timer to reset on crash
  //iwdg_init(IWDG_PRE_32, INT16_MAX);

  //For breadboard prototypes only.
  //Use ENABLE_DEBUG_PORT_PIN as a jumper to enable and disable the SWD pins
  //pinMode(ENABLE_DEBUG_PORT_PIN, INPUT);
  //digitalRead(ENABLE_DEBUG_PORT_PIN) ? enableDebugPorts() : disableDebugPorts();


  //DISABLE DEBUG BY DEFAULT IN PCB VERSION!!!
  disableDebugPorts();

  //Output OPL3 clock on PA1
  /****configure clocks for PORTA & TIMER2********/
  RCC_BASE->APB2ENR |= RCC_APB2ENR_IOPAEN; // enable port A clock
  RCC_BASE->APB1ENR |= RCC_APB1ENR_TIM2EN; // enable clock for timer 2

  /* Porta Pin 1 &2 as alternate function output Push pull */
  GPIOA->regs->CRL = 0x000000BB;  
  /* Output Compare Mode, ENABLE Preload,PWM  mode:2*/
  TIMER2_BASE->CCMR1 = 0x00007800;
  TIMER2_BASE->EGR |= 0x00000001;     // ENABLE update generation
  /*CC2E : channel 2 enabled; polarity : active high*/      
  TIMER2_BASE->CCER = 0x00000010; 
  TIMER2_BASE->CR1 |= 0x00000080;     // Auto preload ENABLE
  TIMER2_BASE->CR1 |= 0x00000001;     // ENABLE Timer counter  
  TIMER2_BASE->CCR2 = 2;              // SET duty cycle to 50%

  setClock(OPL_DEFAULT_CLOCK);
  
  //Set Chips
  VGMEngine.opl3 = &opl3;

  playMode = PlayMode::SHUFFLE;

  delay(100);

  u8g2.begin();
  u8g2.setFont(u8g2_font_fub11_tf);
  u8g2.clearBuffer();
  u8g2.drawStr(0,16,"Aidan Lawrence");
  u8g2.drawStr(0,32,"OPL3, 2019");
  u8g2.sendBuffer();

  delay(250);

  //DEBUG
  pinMode(DEBUG_LED, OUTPUT);
  digitalWrite(DEBUG_LED, LOW);

  //COM
  Serial.begin(9600);

  //IO
  pinMode(next_btn, INPUT_PULLUP);
  pinMode(prev_btn, INPUT_PULLUP);
  pinMode(rand_btn, INPUT_PULLUP);
  pinMode(option_btn, INPUT_PULLUP);

  //SD
  while(!SD.begin(PA4, SPI_HALF_SPEED)) 
  {
    SD.begin(PA4, SPI_HALF_SPEED);
    delay(100);
    u8g2.clearBuffer();
    u8g2.drawStr(0,16,"SD Mount");
    u8g2.drawStr(0,32,"failed!");
    u8g2.sendBuffer();
    Serial.println("SD MOUNT FAILED");
  }

  //Prepare files
  removeMeta();

  File countFile;
  while ( countFile.openNext( SD.vwd(), O_READ ))
  {
    countFile.close();
    countFile.getName(fileName, MAX_FILE_NAME_SIZE);
    
    numberOfFiles++;
  }
  countFile.close();
  SD.vwd()->rewind();

  //Begin
  startTrack(FIRST_START);
}

void pauseISR()
{
  Timer4.pause();
  Timer4.refresh();
}

void setISR()
{
  Timer4.pause();
  Timer4.setPrescaleFactor(1);
  Timer4.setOverflow(1633);
  Timer4.setChannel1Mode(TIMER_OUTPUT_COMPARE);
  Timer4.attachCompare1Interrupt(tick);
  Timer4.refresh();
  Timer4.resume();  
}

void drawOLEDTrackInfo()
{
  if(isOledOn)
  {
    u8g2.setPowerSave(0);
    u8g2.setFont(u8g2_font_helvR08_tf);
    u8g2.clearBuffer();
    u8g2.drawStr(0,9, widetochar(VGMEngine.gd3.enTrackName));
    u8g2.drawStr(0,22, widetochar(VGMEngine.gd3.enGameName));

    u8g2.setFont(u8g2_font_micro_tr);
    if(playMode == PlayMode::LOOP)
      u8g2.drawStr(0,32, "LOOP");
    else if(playMode == PlayMode::SHUFFLE)
      u8g2.drawStr(0,32, "SHUFFLE");
    else
      u8g2.drawStr(0,32, "IN ORDER");
  }
  else
  {
    u8g2.clearDisplay();
    u8g2.setPowerSave(1);
  }
  u8g2.sendBuffer();
}

//Mount file and prepare for playback. Returns true if file is found.
bool startTrack(FileStrategy fileStrategy, String request)
{
  pauseISR();
  File nextFile;
  memset(fileName, 0x00, MAX_FILE_NAME_SIZE);

  switch(fileStrategy)
  {
    case FIRST_START:
    {
      nextFile.openNext(SD.vwd(), O_READ);
      nextFile.getName(fileName, MAX_FILE_NAME_SIZE);
      nextFile.close();
      currentFileNumber = 0;
    }
    break;
    case NEXT:
    {
      if(currentFileNumber+1 >= numberOfFiles)
      {
          SD.vwd()->rewind();
          currentFileNumber = 0;
      }
      else
          currentFileNumber++;
      nextFile.openNext(SD.vwd(), O_READ);
      nextFile.getName(fileName, MAX_FILE_NAME_SIZE);
      nextFile.close();
    }
    break;
    case PREV:
    {
      if(currentFileNumber != 0)
      {
        currentFileNumber--;
        SD.vwd()->rewind();
        for(uint32_t i = 0; i<=currentFileNumber; i++)
        {
          nextFile.close();
          nextFile.openNext(SD.vwd(), O_READ);
        }
        nextFile.getName(fileName, MAX_FILE_NAME_SIZE);
        nextFile.close();
      }
      else
      {
        currentFileNumber = numberOfFiles-1;
        SD.vwd()->rewind();
        for(uint32_t i = 0; i<=currentFileNumber; i++)
        {
          nextFile.close();
          nextFile.openNext(SD.vwd(), O_READ);
        }
        nextFile.getName(fileName, MAX_FILE_NAME_SIZE);
        nextFile.close();
      }
    }
    break;
    case RND:
    {
      randomSeed(micros());
      uint32_t randomFile = currentFileNumber;
      if(numberOfFiles > 1)
      {
        while(randomFile == currentFileNumber)
          randomFile = random(numberOfFiles-1);
      }
      currentFileNumber = randomFile;
      SD.vwd()->rewind();
      nextFile.openNext(SD.vwd(), O_READ);
      {
        for(uint32_t i = 0; i<randomFile; i++)
        {
          nextFile.close();
          nextFile.openNext(SD.vwd(), O_READ);
        }
      }
      nextFile.getName(fileName, MAX_FILE_NAME_SIZE);
      nextFile.close();
    }
    break;
    case REQUEST:
    {
      request.trim();
      char *cstr = &request[0u]; //Convert to C-string
      if(SD.exists(cstr))
      {
        file.close();
        strcpy(fileName, cstr);
        Serial.println("File found!");
      }
      else
      {
        Serial.println("ERROR: File not found! Continuing with current song.");
        goto fail;
      }
    }
    break;
  }

  if(SD.exists(fileName))
    file.close();
  file = SD.open(fileName, FILE_READ);
  if(!file)
  {
    Serial.println("Failed to read file");
    goto fail;
  }
  else
  {
    delay(100);
    if(VGMEngine.begin(&file))
    {
      printlnw(VGMEngine.gd3.enGameName);
      printlnw(VGMEngine.gd3.enTrackName);
      printlnw(VGMEngine.gd3.enSystemName);
      printlnw(VGMEngine.gd3.releaseDate);
      drawOLEDTrackInfo();
      setISR();
      return true;
    }
    else
    {
      Serial.println("Header Verify Fail");
      goto fail;
    }
  }

  fail:
  trackError = true;
  setISR();
  return false;
}

void removeMeta() //Sometimes, Windows likes to place invisible files in our SD card without asking... GTFO!
{
  File tmpFile;
  while ( tmpFile.openNext( SD.vwd(), O_READ ))
  {
    memset(fileName, 0x00, MAX_FILE_NAME_SIZE);
    tmpFile.getName(fileName, MAX_FILE_NAME_SIZE);
    if(fileName[0]=='.')
    {
      if(!SD.remove(fileName))
      if(!tmpFile.rmRfStar())
      {
        Serial.print("FAILED TO DELETE META FILE"); Serial.println(fileName);
      }
    }
    if(String(fileName) == "System Volume Information")
    {
      if(!tmpFile.rmRfStar())
        Serial.println("FAILED TO REMOVE SVI");
    }
    tmpFile.close();
  }
  tmpFile.close();
  SD.vwd()->rewind();
}

//Count at 44.1KHz
void tick()
{
  VGMEngine.tick();
}

//Poll the serial port
void handleSerialIn()
{
  while(Serial.available())
  {
    pauseISR();
    char serialCmd = Serial.read();
    switch(serialCmd)
    {
      case '+':
        startTrack(NEXT);
      break;
      case '-':
        startTrack(PREV);
      break;
      case '*':
        startTrack(RND);
      break;
      case '/':
        playMode = SHUFFLE;
        drawOLEDTrackInfo();
      break;
      case '.':
        playMode = LOOP;
        drawOLEDTrackInfo();
      break;
      case '?':
        printlnw(VGMEngine.gd3.enGameName);
        printlnw(VGMEngine.gd3.enTrackName);
        printlnw(VGMEngine.gd3.enSystemName);
        printlnw(VGMEngine.gd3.releaseDate);
      break;
      case '!':
        isOledOn = !isOledOn;
        drawOLEDTrackInfo();
      break;
      case 'r':
      {
        String req = Serial.readString();
        req.remove(0, 1); //Remove colon character
        startTrack(REQUEST, req);
      }
      break;
      default:
        continue;
    }
  }
  Serial.flush();
  setISR();
}

//Check for button input
bool buttonLock = false;
void handleButtons()
{
  bool togglePlaymode = false;
  uint32_t count = 0;
  
  if(!digitalRead(next_btn))
    startTrack(NEXT);
  if(!digitalRead(prev_btn))
    startTrack(PREV);
  if(!digitalRead(rand_btn))
    startTrack(RND);
  if(!digitalRead(option_btn))
    togglePlaymode = true;
  else
    buttonLock = false;
  while(!digitalRead(option_btn))
  {
    pauseISR();
    if(count >= 100) 
    {
      //toggle OLED after one second of holding OPTION button
      isOledOn = !isOledOn;
      drawOLEDTrackInfo();
      togglePlaymode = false;
      buttonLock = true;
      break;
    } 
    delay(10);
    count++;
  }
  if(buttonLock)
  {
    togglePlaymode = false;
    setISR();
  }

  if(togglePlaymode)
  {
    togglePlaymode = false;
    if(playMode == PlayMode::SHUFFLE)
      playMode = PlayMode::LOOP;
    else if(playMode == PlayMode::LOOP)
      playMode = PlayMode::IN_ORDER;
    else if(playMode == PlayMode::IN_ORDER)
      playMode = PlayMode::SHUFFLE;
    drawOLEDTrackInfo();
    setISR();
  }
}

void loop()
{   
  if(trackError)
  {
    startTrack(NEXT);
    trackError = false;
  }
  while(!VGMEngine.play()) //needs to account for LOOP playmode
  {
    if(Serial.available() > 0)
      handleSerialIn();
    handleButtons();
    if(trackError)
    {
      startTrack(NEXT);
      trackError = false;
    }
  }
  //Hit max loops and/or VGM exited
  if(playMode == PlayMode::SHUFFLE)
    startTrack(RND);
  if(playMode == PlayMode::IN_ORDER)
    startTrack(NEXT);
  //iwdg_feed();
}
