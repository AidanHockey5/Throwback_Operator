#include "VGMEngine.h"

VGMEngineClass::VGMEngineClass()
{
    MegaStream_Create(&stream, buf, VGM_BUF_SIZE);
}
VGMEngineClass::~VGMEngineClass(){}

bool VGMEngineClass::begin(File *f)
{
    ready = false;
    file = f;
    if(!header.read(file))
        return false;
    gd3.read(file, header.gd3Offset+0x14);
    

    if(header.vgmDataOffset == 0)
        file->seek(0x40);
    else
        file->seek(header.vgmDataOffset+0x34);
    
    if(header.gd3Offset != 0)
    {
        loopPos = header.gd3Offset+0x14-1;
    }
    else
    {
        loopPos = header.EoF+4-1;
    }

    chipSetup();

    #if ENABLE_SPIRAM
        ram.Init();
    #endif

    storePCM(true);
    pcmBufferPosition = 0;

    waitSamples = 0;
    loopCount = 0;
    MegaStream_Reset(&stream);
    load();
    ready = true;
    return true;
}

uint8_t empty = 0;
uint8_t VGMEngineClass::readBufOne()
{
    if(MegaStream_Used(&stream) < 1)
    {
        //digitalWrite(PA8, HIGH);
        load();
        empty = 1;
    }
    uint8_t b[1];
    MegaStream_Recv(&stream, b, 1);
    return b[0];
}

uint16_t VGMEngineClass::readBuf16()
{
    if(MegaStream_Used(&stream) < 2)
    {
        //digitalWrite(PA8, HIGH);
        load();
        empty = 16;
    }
    uint16_t d;
    uint8_t b[2];
    MegaStream_Recv(&stream, b, 2);
    d = uint16_t(b[0] + (b[1] << 8));
    return d;
}

uint32_t VGMEngineClass::readBuf32()
{
    if(MegaStream_Used(&stream) < 4)
    {
        //digitalWrite(PA8, HIGH);
        load();
        empty = 32;
    }
    uint32_t d;
    uint8_t b[4];
    MegaStream_Recv(&stream, b, 4);
    d = uint32_t(b[0] + (b[1] << 8) + (b[2] << 16) + (b[3] << 24));
    return d;
}

bool VGMEngineClass::topUp()
{
    if(MegaStream_Free(&stream) == 0)
        return true;
    byte b = file->read();
    MegaStream_Send(&stream, &b, 1);
    return false;
}

bool VGMEngineClass::storePCM(bool skip)
{
    bool isPCM = false;
    if(file->peek() == 0x67) //PCM Block
    {
        isPCM = true;
        //Serial.println("pcm");
        file->read(); //0x67
        file->read(); //0x66
        file->read(); //datatype
        uint32_t pcmSize = 0;
        file->read(&pcmSize, 4); //PCM chunk size
        if(skip)            //On loop, you'll want to just skip the PCM block since it's already loaded.
        {
            file->seekCur(pcmSize);
            return false;
        }
        if(pcmSize > MAX_PCM_BUFFER_SIZE)
        {
            return true; //todo: Error out here or go to next track. return is temporary
        }
        else
        {
            for(uint32_t i = 0; i<pcmSize; i++)
            {
                #if ENABLE_SPIRAM
                else
                    ram.WriteByte(i, file->read());
                #endif
            }
        }
    } 
    return isPCM;
}

bool VGMEngineClass::load(bool singleChunk)
{
    const uint16_t MAX_CHUNK_SIZE = 512;
    int32_t space = MegaStream_Free(&stream);
    if(space == 0)
        return true;
    bool didSingleChunk = false;
    uint8_t chunk[MAX_CHUNK_SIZE];
    while(MegaStream_Free(&stream) != 0 || didSingleChunk) //Fill up the entire buffer, or only grab a single chunk quickly
    {
        bool hitLoop = false;
        uint16_t chunkSize = min(space, MAX_CHUNK_SIZE); //Who's smaller, the space left in the buffer or the maximum chunk size?
        
        if(file->position() + chunkSize >= loopPos+1) //Loop code. A bit of math to see where the file pointer is. If it goes over the 0x66 position, we'll set a flag to move the file pointer to the loop point and adjust the chunk as to not grab data past the 0x66
        {
            chunkSize = loopPos+1 - file->position(); //+1 on loopPos is to make sure we include the 0x66 command in the buffer in order for loop-triggered events to work.
            hitLoop = true;
        }

        space -= chunkSize;                 //Reduce space by the chunkSize, then read from the SD card into the chunk. Send chunk to buffer.
        file->read(chunk, chunkSize);
        MegaStream_Send(&stream, chunk, chunkSize); 
        if(hitLoop)                         //Here is where we reset the file pointer back to the loop point
        {
            if(header.loopOffset !=0)
                file->seek(header.loopOffset+0x1C);
            else
            {
                if(header.vgmDataOffset == 0)
                    file->seek(0x40);
                else
                    file->seek(header.vgmDataOffset+0x34);
                storePCM(true);
            }
        }
        if(space <= 0)                      //No more space in the buffer? Just eject.
            return true;
        if(singleChunk)                     //Only want to grab a single chunk instead of filling the entire buffer? Set this flag.
            didSingleChunk = true;
    }
    //NOTE, Loop (0x66) will ALWAYS be (Gd3 offset - 1) OR (EoF offset - 1) if there is no Gd3 data
    return false;
}

void VGMEngineClass::chipSetup()
{
    #if ENABLE_SN76489
    //sn76489->setClock(header.sn76489Clock);
    ltcSN.SetFrequency(header.sn76489Clock);
    delay(10);
    sn76489->reset();
    #endif
    #if ENABLE_YM2612
    //ym2612->setClock(header.ym2612Clock);
    ltcOPN.SetFrequency(header.ym2612Clock);
    delay(10);
    ym2612->reset();
    #endif
    #if ENABLE_YM3526 || ENABLE_YM3812 || ENABLE_YMF262
    if(header.ym3526clock > 0)
    {
        setClock(header.ym3526clock);
        opl3->setOPLMode(0); //OPL1 and 2 mode
    }
    else if(header.ym3812clock > 0)
    {
        setClock(header.ym3812clock);
        opl3->setOPLMode(0); //OPL1 and 2 mode
    }
    else if(header.ymf262clock > 0)
    {
        setClock(header.ymf262clock);
        opl3->setOPLMode(1); //OPL3 mode
    }

    #endif
}

void VGMEngineClass::tick()
{
    if(!ready)
        return;
    waitSamples--;      
}

bool VGMEngineClass::play()
{
    load(); 
    while(waitSamples <= 0)
    {
        waitSamples += parseVGM();
    }
    if(loopCount == maxLoops)
        return true;
    return false;
}

uint16_t VGMEngineClass::getLoops()
{
    return loopCount;
}

uint16_t VGMEngineClass::parseVGM()
{
    uint8_t cmd;
    //ready = false;
    while(true)
    {
        cmd = readBufOne();
        switch(cmd)
        {
            case 0x4F:
            #if ENABLE_SN76489
                sn76489->write(0x06);
                sn76489->write(readBufOne());
            #endif
            break;
            case 0x50:
            #if ENABLE_SN76489
                sn76489->write(readBufOne());
            #endif
            break;
            case 0x52:
            #if ENABLE_YM2612
                ym2612->write(readBufOne(), readBufOne(), 0);
            #endif
            break;
            case 0x53:
            #if ENABLE_YM2612
                ym2612->write(readBufOne(), readBufOne(), 1);
            #endif
            break;
            case 0x5A:
            #if ENABLE_YM3812
                opl3->write(readBufOne(), readBufOne(), 0);
            #endif
            break;
            case 0x5B:
            #if ENABLE_YM3526
                opl3->write(readBufOne(), readBufOne(), 0);
            #endif
            break;
            case 0x5E:
            #if ENABLE_YMF262
                opl3->write(readBufOne(), readBufOne(), 0);
            #endif
            break;
            case 0x5F:
            #if ENABLE_YMF262
                opl3->write(readBufOne(), readBufOne(), 1);
            #endif
            break;
            case 0x61:
                return readBuf16();
            case 0x62:
                return 735;
            case 0x63:
                return 882;
            case 0x67:
            {
                //who puttin' 0x67's in MUH stream?
                Serial.println("0x67 PCM STORE command encountered mid stream");
                break;
            }
            case 0x70:
            case 0x71:
            case 0x72:
            case 0x73:
            case 0x74:
            case 0x75:
            case 0x76:
            case 0x77:
            case 0x78:
            case 0x79:
            case 0x7A:
            case 0x7B:
            case 0x7C:
            case 0x7D:
            case 0x7E:
            case 0x7F:
                return (cmd & 0x0F)+1;
            case 0x80:
            case 0x81:
            case 0x82:
            case 0x83:
            case 0x84:
            case 0x85:
            case 0x86:
            case 0x87:
            case 0x88:
            case 0x89:
            case 0x8A:
            case 0x8B:
            case 0x8C:
            case 0x8D:
            case 0x8E:
            case 0x8F:
            {
            #if ENABLE_YM2612
                ym2612->write(0x2A, ram.ReadByte(pcmBufferPosition++), 0);
            #endif

            uint8_t wait = (cmd & 0x0F);
            if(wait == 0)
                break;
            else
                return wait;
            }
            case 0xE0:
                pcmBufferPosition = readBuf32();
            break;
            case 0x66:
            {
                //Loop
                loopCount++;
                return 0;
            }

            break;
            
            //Things to ignore
            case 0xB5: //Ignore common secondary PCM chips
            case 0xB6:
            case 0xB7:
            case 0xB8:
            case 0xB9:
            case 0xBA:
            case 0xBB:
            case 0xBC:
            case 0xBD:
            case 0xBE:
            case 0xBF:
            readBuf16();
            break;
            case 0xC0: //Ignore SegaPCM:
            case 0xC1:
            case 0xC2:
            case 0xC3:
            readBufOne();readBufOne();readBufOne();
            break;
            case 0xD4:
            readBufOne();readBufOne();readBufOne();
            break;


            default:
                Serial.print("E:"); Serial.println(cmd, HEX);
                return 0;
        }
    }
}

VGMEngineClass VGMEngine;