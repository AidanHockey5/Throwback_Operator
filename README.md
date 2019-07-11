# THROWBACK OPERATOR
OPL3 VGM player powered by STM32

This project is a hardware video game music (VGM) player that uses a genuine YMF262 (OPL3) sound chip and its accompanying YAC512 DAC.
There is no emulation here. This is the real chip.

Included in this repository is the complete schematic, firmware source, and KiCAD board files. Feel free to make your own board, but please don't resell them :)

If you would like to purchase an official complete Throwback Operator board, you can order one from me here: https://www.aidanlawrence.com

# Information About the Main Sound Chips and VGM

The YMF262, also known as the Operator Type L3 (OPL3), is an 18-channel FM sound chip that was commonly found inside of personal computers in the mid to late 1990's.
Most notably, the OPL3 was found on the Sound Blaster Pro 2 and the Sound Blaster 16.
The OPL3 is backwards compatible with the OPL2 (YM3812). The OPL2 is backwards compatible with the original OPL (YM3526).
This means that the Throwback Operator is capable of playing back OPL3, OPL2, and OPL tracks. The OPL2 was found on the original Adlib and Sound Blaster cards.
The OPL was usually found within arcade machines.

The clock signal for the YMF262 is dynamically generated from the microcontroller. The timers on this particular board only have 16-bit resolution, so VGMs with exotic frequencies may not sound perfectly in tune. Standard clock OPL2 and OPL3 VGMs will often sound just fine though.

VGM stands for Video Game Music, and it is a 44.1KHz logging format that stores real sound chip register information. My player will parse these files and send the data to the appropriate chips. You can learn more about the VGM file format here: http://www.smspower.org/Music/VGMFileFormat

http://www.smspower.org/uploads/Music/vgmspec170.txt?sid=58da937e68300c059412b536d4db2ca0

# SD Card Information

The Throwback Operator accepts MicroSD cards formatted as FAT32. Your SD card must only contain uncompressed .vgm files. VGZ FILES WILL NOT WORK! You may download .vgz files and use 7zip to extract the uncompressed file out of them. Vgm files on the SD card do not need to have the .vgm extension. As long as they contain valid, uncompressed vgm data, they will be read by the program regardless of their name. You can find VGM files by Googling "myGameName VGM," or by checking out sites like http://vgmrips.net/packs/

# Control Over Serial
You can use a serial connection to control playback features. The commands are as follows:

Command | Result
------------ | -------------
\+ | Next Track
\- | Previous Track
\* | Random Track
\/ | Toggle Shuffle Mode
\. | Toggle Song Looping
r: | Request song
? | What song is playing?
\! | Toggle the OLED display

A song request is formatted as follows: ```r:mySongFile.vgm```
Once a song request is sent through the serial console, an attempt will be made to open that song file. The file must exist on the SD card, and spelling/capitalization must be correct.
Need an easy-to-use serial console? [I've made one here.](https://github.com/AidanHockey5/OpenArduinoSerialConsole)

# Programming the Microcontroller

The on-board microcontroller is a STM32F103C8T6, the same microcontroller found on the popular "blue pill" development boards. 
Therefore, programming the Throwback Operator boards is fairly similar.
The firmware was built with Visual Studio Code with the PlatformIO extension installed.

Programming over Serial:
* Connect your Throwback Operator board to your computers USB port.
* Ensure that you have the CH340 driver installed. Windows 10 should have this automatically, otherwise, [grab the driver here](https://sparks.gogo.co.nz/ch340.html)
* Ensure nothing else is currently connected to your board over serial.
* On the top of the board, you will see two black jumpers. By default, these jumpers are both in the "Down" position (middle pin and bottom pin jumped).
* Remove the left jumper and reattach it to the "Up" position (middle pin and top pin jumped).
* Press the RESET button on the Throwback Operator board
* In VS Code (or whatever your IDE of choice is), navigate to the `platformio.ini` and ensure the upload protocol is set to `serial`
* Hit the upload button (Arrow icon in the bottom bar of VS code)
* Once your code is uploaded, replace the left jumper to its original "Down" position 
