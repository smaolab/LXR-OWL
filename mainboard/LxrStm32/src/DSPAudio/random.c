/*
 * rnd.c
 *
 *  Created on: 07.04.2012
 * ------------------------------------------------------------------------------------------------------------------------
 *  Copyright 2013 Julian Schmidt
 *  Julian@sonic-potions.com
 * ------------------------------------------------------------------------------------------------------------------------
 *  This file is part of the Sonic Potions LXR drumsynth firmware.
 * ------------------------------------------------------------------------------------------------------------------------
 *  Redistribution and use of the LXR code or any derivative works are permitted
 *  provided that the following conditions are met:
 *
 *       - The code may not be sold, nor may it be used in a commercial product or activity.
 *
 *       - Redistributions that are modified from the original source must include the complete
 *         source code, including the source code for all components used by a binary built
 *         from the modified sources. However, as a special exception, the source code distributed
 *         need not include anything that is normally distributed (in either source or binary form)
 *         with the major components (compiler, kernel, and so on) of the operating system on which
 *         the executable runs, unless that component itself accompanies the executable.
 *
 *       - Redistributions must reproduce the above copyright notice, this list of conditions and the
 *         following disclaimer in the documentation and/or other materials provided with the distribution.
 * ------------------------------------------------------------------------------------------------------------------------
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 *   USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ------------------------------------------------------------------------------------------------------------------------
 */


#include "random.h"
#include "stm32f4xx.h"
//-------------------------------------------------------------
void initRng()
{
	/* Enable RNG clock source */
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);

	/* RNG Peripheral enable */
	RNG_Cmd(ENABLE);
}
//-------------------------------------------------------------
__inline uint32_t GetRngValue()
{
	return RNG_GetRandomNumber();
}
//-------------------------------------------------------------


//-------------------------------------------------------------
// rstephane : get a random number between pre defined two markers
uint8_t GetRndValue7()
{
	uint8_t rndData;
	initRng();
	while(RNG_GetFlagStatus(RNG_FLAG_DRDY)== RESET)
      	{
      	}
      	
      	/* Get a random number between 0 and 7 */
   	do {
        	rndData = GetRngValue();

        	/* mask off the bottom 3 bits */
       		rndData = rndData & 0x00000007;
    	} while (rndData == 7);

      return rndData;
}

// rstephane : get a random number between pre defined two markers
uint8_t GetRndValue6()
{
	uint8_t rndData;
	initRng();
	while(RNG_GetFlagStatus(RNG_FLAG_DRDY)== RESET)
      	{
      	}

      	/* Get a random number between 0 and 6 */
   	do {
        	rndData = GetRngValue();

        	/* mask off the bottom 3 bits */
       		rndData = rndData & 0x00000007;
    	} while (rndData == 6 || rndData == 7);

      return rndData;
}

// rstephane : get a random number between pre defined two markers
uint8_t GetRndValue2()
{
	uint8_t rndData;
	initRng();
	while(RNG_GetFlagStatus(RNG_FLAG_DRDY)== RESET)
      	{
      	}

      	/* Get a random number between 0 and 2 */
   	do {
        	rndData = GetRngValue();

        	/* mask off the bottom 3 bits */
       		rndData = rndData & 0x00000007;
    	} while (rndData == 3 || rndData == 4 || rndData == 5 || rndData == 6 || rndData == 7);

      return rndData;
}

// rstephane : get a random number between pre defined two markers
uint8_t GetRndValue1()
{
	uint8_t rndData;
	initRng();
	while(RNG_GetFlagStatus(RNG_FLAG_DRDY)== RESET)
      	{
      	}

      	/* Get a random number between 0 and 1 */
   	do {
        	rndData = GetRngValue();

        	/* mask off the bottom 3 bits */
       		rndData = rndData & 0x00000007;
    	} while (rndData == 2 || rndData == 3 || rndData == 4 || rndData == 5 || rndData == 6 || rndData == 7);

      return rndData;
}

// rstephane : get a random number between pre defined two markers
uint8_t GetRndValue16()
{
	uint8_t rndData;
	initRng();
	while(RNG_GetFlagStatus(RNG_FLAG_DRDY)== RESET)
      	{
      	}

      	/* Get a random number between 0 and 15 */
   	do {
        	rndData = GetRngValue();

        	/* mask off the bottom 3 bits 1 to 6 */
       		//rndData = rndData & 0x00000007;
         	/* mask off the bottom 5 bits 0 to 15 */
        	rndData = rndData & 0x0000001F;
    	} while (rndData == 16);

      return rndData;
}

// rstephane : get a random number between pre defined two markers
uint8_t GetRndValue127()
{
	uint8_t rndData;
	initRng();
	while(RNG_GetFlagStatus(RNG_FLAG_DRDY)== RESET)
      	{
      	}

      	/* Get a random number between 0 and 15 */
   	do {
        	rndData = GetRngValue();

         	/* mask off the bottom 7 bits 0 to 126 */
        	rndData = rndData & 0x0000007F;
    	} while (rndData == 127);

      return rndData;
}
