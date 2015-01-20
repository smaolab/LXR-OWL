LXR-marimba Drumsynth Firmware
====================================
This version is a fork from PLD/RUDEOG that contains all original code and many new features (I called this version OWL as some of the plug ins added are coming from the nice project OWL http://hoxtonowl.com/patches-2/ ):

### 1. You can select among 72 pre filled patterns for each voice

Select a track (1-7), then click on the button VOICE, then click on the OSC menu to access the extra features :-)
### 2. You can randomize steps/patterns for each voice, creating surprising drum lines !

Select a track (1-7), then click on the button VOICE, then click on the OSC menu to access the extra features :-)
### 3. You can make a LOOP/DIVIDE effect like on the Arturia Spark machine

Select the PERF button, then click again the PERF button to access the sub menu:  You will see a menu called LEN, to arm the looping effects, turn the knob until you reach 127 and then turn down little by little the knob (until ZERO) to listen to the effect!

### 4. Sounds effect (Compressor, Bit crusher, Bit reduce, OTO biscuit FX alike, Bit reverse, etc)

Select the PERF button, then click again the PERF button to access the sub menu: 
# You will see a menu FX VL1 VL2 VL3. 
# FX : Select the FX you want to apply : 
- 1: Simple compressor (VL1 = sensitivity, VL2 = Compression level, VL3 = Dry/wet) 
- - 2: Bit rotate -
- 3: not working 
- 4: 
- 5: 
- 6: 
- 7: 
- 8: 
- 9-16: not working

Enjoy!


Sonic Potions LXR Drumsynth
====================================
The LXR is a digital drum synthesizer based on the 32-bit Cortex-M4 processor and an Atmega644 8-bit CPU. Developed by Julian Schmidt.

    The 'front' folder contains the AVR code

    The 'mainboard' folder contains the STM32F4 code

    The 'tools' folder contains the firmware image builder tool, to combine AVR and Cortex code into a single file, usable by the bootloader.

Please note that there are libraries from ST and ARM used in the mainboard code of this project. They are all located in the Libraries subfolder of the project. Those come with their own license. The libraries are:

    ARM CMSIS library
    ST STM32_USB_Device_Library
    ST STM32_USB_OTG_Driver
    ST STM32F4xx_StdPeriph_Driver


Here are the instructions for MAC OS Build (MAC OS 10.8 and above):
-------------------------------------------------------------------

Hello,
First of all, I would like to thanks @Rudeog, @Pld, @TB323, @Julian and @spfrc for their great help with me :-) I must have forgot some names who helped me, sorry in advance :-)

Tutorial to build the firmware for MAC OS 10.8 and above:

Note 1 - The installation steps are rather simple as I don't use anymore ECLIPSE. Indeed, configuring Eclipse on MAC to compile the code is rather tricky for a beginner like me.

Note 2 - The LXR code is divided into two mains code parts that we will compile "separately" using two different tools :-)

    the AVR Atmel CODE (LCD, front panel control, USB, SD card)
    the ARM CODE (main LXR code for the voices / sounds)


General steps:
====================================

We need "specific compilers and librairies" for each CODE. We need to change the PATH in your .bash_profile, and finally we need a MAKEFILE to build the firmware:

    AVR AtmelCODE  -> command/libs avr-gcc
    GNU ARM CODE --> command/libs arm-eabi-none-gcc
    Change PATH on your MAC -> I will provide an exemple later.
    LXR Code  -> We need the LXR code that we will download later.
    MACOS specific MAKEFILE  -> We need special makefile that @PLD has provided. I will give you the link later
    Compiling and build the firmware.bin file-> You just have to launch the command "make firmware"/"make clean" from a MAC terminal window.


Let's start!
====================================


1- AVR COMPILER & LIBS
====================================

- Download of Atmel AVR 8-bit toolchain from http://www.obdev.at/products/crosspack/download.html
- You will get a .dmg file that you should extract and install (CrossPack-AVR-20131216.dmg is the most recent as of 2015/05/02)
- The compiler and libs are installed under /usr/local/CrossPack-AVR-20131216/
- Note that you will also find a folder like /usr/local/CrossPack-AVR/ (this folder is an alias to the previous folder - symlink).

2- GNU ARM COMPILER AND LIBS
====================================

- Download of GNU bare metal ARM toolchain from https://launchpad.net/gcc-arm-embedded (v4.8)
- Take the MAC version (something like : gcc-arm-none-eabi-4_8-2014q1-20140314-mac.tar.bz2 )
- Extract the compressed/tar file.
- Copy the extracted folder under /usr/local/
- Nothing else to do !

3- Change your PATH
====================================

We need to add in our MAC session a PATH to both compilers (and libs), here is mine:
	
	export PATH=$PATH:/usr/local/CrossPack-AVR-20131216/bin:/usr/local/CrossPack-AVR-20131216/:/usr/include:/usr/local/CrossPack-AVR-20131216/avr/include:/usr/local/gcc-arm-none-eabi-4_8-2014q1/bin
	export ARM_TOOLKIT_ROOT=/usr/local/gcc-arm-none-eabi-4_8-2014q1
	export AVR_TOOLKIT_ROOT=/usr/local/CrossPack-AVR-20131216

In a terminal window, execute "source ./.bash_profile" to update the PATH to your system. For the moment, don't try to understand what are ARM_TOOLKIT / AVR_TOLLKIT variables, I will explain why we need this later.

4- Grab the LXR Code
====================================

I advice you to grab the code either from :

- Julian repository on GITHUB https://github.com/SonicPotions/LXR -> but you may have issues as this code contains wrong backslashes in certain area of the code. Should be soon fixed.

- PLD repository https://github.com/patrickdowling/LXR -> but you may also have issues with this code containing wrong backslashes in certain area of the code.
    
- rudeog repository https://github.com/rudeog/LXR -> this one is OKAY with many extra features compared to Julian code. 

- rstephane repository https://github.com/rstephane/LXR  -> this one is OKAY, it is equivalent to Julian CODE without the wrong backslashes. 

Of course once downloaded, you should extract the code and copy it to any folder you like.
On my side I did copied it under: /Users/music/Documents/workspace/LXR/

5- MACOS specific makefile 
====================================


I advice you to grab the makefile either from :

- PLD repository https://github.com/patrickdowling/LXR
- rstephane (@egnouf) repository https://github.com/rstephane/LXR

You will find on the root path of both repository a file called MAKEFILE.
Download it!

Once downloaded, copy it at the root PATH of the code you dowloaded in STEP 4.
In my case, I copied the makefile of PLD into /Users/music/Documents/workspace/LXR/

If you have taken the whole LXR code from @PLD or rstephane (@egnouf) you don't need to download once more the Makefile, it is provided within ! 

The Makefile of rstephane is a copy of the one provided by @PLD

Now, let's go back to the .bash_profile file:

	export PATH=$PATH:/usr/local/CrossPack-AVR-20131216/bin:/usr/local/CrossPack-AVR-20131216/:/usr/include:/usr/local/CrossPack-AVR-20131216/avr/include:/usr/local/gcc-arm-none-eabi-4_8-2014q1/bin
	export ARM_TOOLKIT_ROOT=/usr/local/gcc-arm-none-eabi-4_8-2014q1
	export AVR_TOOLKIT_ROOT=/usr/local/CrossPack-AVR-20131216

You can notice the two variables ARM_TOOLKIT_ROOT and AVR_TOOLKIT_ROOT.
They are used by the MAKEFILE that has created @PLD (and also used by rstephane).

Don't forget to change their values appropriately, according to your system path, where you installed the libs... etc.  

6- Compile!!!!
====================================

You are nearly done:

- You just have to launch the command "make firmware" from a MAC terminal window. To clean the code you can execute "make clean".
- You will get a FIRMWARE.BIN image, copy this file on your SD CARD. Put the SD CARD into the LXR drummachine, hold the main rotary encoder and switch on the drum machine.... the system will upload and upgrade the machine with your code.

If you have downloaded @rstephane code, similar to @Julian and @PLD, you will noticed that when you switch on the machine, it displays "LXR-Drums-V" instead of "LXR Drums V". If so, you have successfully loaded (and compiled) LXR CODE!!!!

Well done and happy hacking!
:-)
