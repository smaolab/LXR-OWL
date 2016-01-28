/*
 * HiHat.c
 *
 *  Created on: 18.04.2012
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



#include "HiHat.h"
#include "squareRootLut.h"
#include "modulationNode.h"
#include "TriggerOut.h"



// rstephane  ---------
#include "valueShaper.h"

extern uint8_t fxMaskType; // 0-16 for effects 1
extern uint8_t fx1; // PAR1
extern uint8_t fx2; // PAR2
extern uint8_t fx3; // PAR3
extern uint8_t fx4; // ...
extern uint8_t fx5;
extern uint8_t fx6;
extern uint8_t fx7;

INCCMZ HiHatVoice hatVoice;

//---------------------------------------------------
void HiHat_setPan(const uint8_t pan)
{
	//hatVoice.panL = squareRootLut[127-pan];
	//hatVoice.panR = squareRootLut[pan];
	hatVoice.pan = pan;
}
//---------------------------------------------------

void HiHat_init()
{
	SnapEg_init(&hatVoice.snapEg);
	HiHat_setPan(0);
	hatVoice.vol = 0.8f;

	//hatVoice.panModifier = 1.f;

	transient_init(&hatVoice.transGen);

	hatVoice.fmModAmount1 = 0.5f;
	hatVoice.fmModAmount2 = 0.5f;

	setDistortionShape(&hatVoice.distortion, 2.f);

	hatVoice.modOsc.freq = 440;
	hatVoice.modOsc.waveform = SINE;
	hatVoice.modOsc.fmMod = 0;
	hatVoice.modOsc.midiFreq = 70<<8;
	hatVoice.modOsc.pitchMod = 1.0f;
	hatVoice.modOsc.modNodeValue = 1;

	hatVoice.modOsc2.freq = 440;
	hatVoice.modOsc2.waveform = NOISE;//SINE;
	hatVoice.modOsc2.fmMod = 0;
	hatVoice.modOsc2.midiFreq = 70<<8;
	hatVoice.modOsc2.pitchMod = 1.0f;
	hatVoice.modOsc2.modNodeValue = 1;

	hatVoice.osc.freq = 440;
	hatVoice.osc.waveform = 1;
	hatVoice.osc.fmMod = 1;
	hatVoice.osc.midiFreq = 70<<8;
	hatVoice.osc.pitchMod = 1.0f;
	hatVoice.osc.modNodeValue = 1;

	hatVoice.volumeMod = 1;

	slopeEg2_init(&hatVoice.oscVolEg);

	SVF_init(&hatVoice.filter);

	lfo_init(&hatVoice.lfo);
}
//---------------------------------------------------
void HiHat_trigger( uint8_t vel, uint8_t isOpen, const uint8_t note)
{
	lfo_retrigger(5);

	//update velocity modulation
	modNode_updateValue(&velocityModulators[5],vel/127.f);

	float offset = 1;
	if(hatVoice.transGen.waveform==1) //offset mode
	{
		offset -= hatVoice.transGen.volume;
	}
	if(hatVoice.osc.waveform == SINE)
		hatVoice.osc.phase = (0x3ff<<20)*offset;//voiceArray[voiceNr].osc.startPhase ;
	else if(hatVoice.osc.waveform > SINE && hatVoice.osc.waveform <= REC)
		hatVoice.osc.phase = (0xff<<20)*offset;
	else
		hatVoice.osc.phase = 0;

	osc_setBaseNote(&hatVoice.osc,note);
	osc_setBaseNote(&hatVoice.modOsc,note);
	osc_setBaseNote(&hatVoice.modOsc2,note);

	hatVoice.isOpen = isOpen;
	hatVoice.oscVolEg.decay = isOpen?hatVoice.decayOpen:hatVoice.decayClosed;

	slopeEg2_trigger(&hatVoice.oscVolEg);
	hatVoice.velo = vel/127.f;
	transient_trigger(&hatVoice.transGen);

	SnapEg_trigger(&hatVoice.snapEg);
}
//---------------------------------------------------
void HiHat_calcAsync( )
{
	//calc the osc  vol eg
	hatVoice.egValueOscVol = slopeEg2_calc(&hatVoice.oscVolEg);

	//turn off trigger signal if trigger gate mode is on and volume == 0
	if(trigger_isGateModeOn())
	{
		if(!hatVoice.egValueOscVol)
		{
			if(hatVoice.isOpen)
			{
				trigger_triggerVoice(TRIGGER_7, TRIGGER_OFF);
				voiceControl_noteOff(TRIGGER_7);
			} else {
				trigger_triggerVoice(TRIGGER_6, TRIGGER_OFF);
				voiceControl_noteOff(TRIGGER_6);
			}
		}
	}

	//calc snap EG if transient sample 0 is activated
	if(hatVoice.transGen.waveform == 0)
	{
		const float snapVal = SnapEg_calc(&hatVoice.snapEg, hatVoice.transGen.pitch);
		hatVoice.osc.pitchMod = 1 + snapVal*hatVoice.transGen.volume;
	}

	osc_setFreq(&hatVoice.osc);
	osc_setFreq(&hatVoice.modOsc);
	osc_setFreq(&hatVoice.modOsc2);
}
//---------------------------------------------------
void HiHat_calcSyncBlock(int16_t* buf, const uint8_t size)
{
	//2 buffers for the mod oscs
	int16_t mod1[size],mod2[size];
	//calc next mod osc samples, scaled with mod amount
	calcNextOscSampleBlock(&hatVoice.modOsc,mod1,size, hatVoice.fmModAmount1);
	calcNextOscSampleBlock(&hatVoice.modOsc2,mod2,size,  hatVoice.fmModAmount2);

	//combine both mod oscs to 1 modulation signal
	bufferTool_addBuffersSaturating(mod1,mod2,size);

	calcNextOscSampleFmBlock(&hatVoice.osc,mod1,buf,size,0.5f) ;

	SVF_calcBlockZDF(&hatVoice.filter,hatVoice.filterType,buf,size);

	//calc transient sample
	transient_calcBlock(&hatVoice.transGen,mod1,size);

	// --------------------------------------------------
	//rstephane: My effects ;-)
	if (fxMaskType>0)
		calcFxBlock(fxMaskType,buf, size,fx1,fx2,fx3); // TO DO add more PARAM !

		//calcParamEqBlock(fxMaskType,buf, size,0.4,0.6,0.7,0.8);


	uint8_t j;
	if(hatVoice.volumeMod)
	{
		for(j=0;j<size;j++)
		{
			//add filter to buffer
			buf[j] = __QADD16(buf[j],mod1[j]);
			buf[j] *= hatVoice.velo * hatVoice.vol * hatVoice.egValueOscVol;
		}
	}
	else
	{
		for(j=0;j<size;j++)
		{
			//add filter to buffer
			buf[j] = __QADD16(buf[j],mod1[j]);
			buf[j] *= hatVoice.vol * hatVoice.egValueOscVol;
		}
	}

	calcDistBlock(&hatVoice.distortion,buf,size);
}
//---------------------------------------------------



//---------------------------------------------------
// rstephane : my functions
//random all the parameters for voice HitHAt
//---------------------------------------------------

void randomHHVoice(uint8_t randomType)
{
	switch(randomType)
	{
		case 1 :
			randomHHVoiceOSC();
 			break;
		case 2 :
			randomHHVoiceADSR();
			break;
		case 3 :
			randomHHVoiceDIST();
			break;
		case 4 :
			randomHHVoiceFILTER();
			break;
		case 5 :
			randomHHVoiceOSC();
			randomHHVoiceADSR();
			break;
		case 6 :
			randomHHVoiceOSC();
			randomHHVoiceFILTER();
			break;
		case 7 :
			randomHHVoiceOSC();
			randomHHVoiceDIST();
			break;
		case 8 :
			randomHHVoiceOSC();
			randomHHVoiceFILTER();
			break;
		case 9 :
			randomHHVoiceDIST();
			randomHHVoiceADSR();
			break;
		case 10 :
			randomHHVoiceDIST();
			randomHHVoiceFILTER();
		 	break;
		case 11 :
			randomHHVoiceFILTER();
			randomHHVoiceADSR();
			break;
		case 12 :
			randomHHVoiceADSR();
			randomHHVoiceOSC();
			randomHHVoiceDIST();
			break;
		case 13 :
			randomHHVoiceOSC();
			randomHHVoiceFILTER();
			randomHHVoiceDIST();
			break;
		case 14 :
			randomHHVoiceDIST();
			randomHHVoiceFILTER();
			randomHHVoiceADSR();
			break;
		case 15 :
			randomHHVoiceOSC();
			randomHHVoiceFILTER();
			randomHHVoiceADSR();
 			break;
		case 16 :
			randomHHVoiceOSC();
			randomHHVoiceDIST();
			randomHHVoiceFILTER();
			randomHHVoiceADSR();
 			break;

		default:
			break;
		break;
	}

}


//---------------------------------------------------
void randomHHVoiceOSC(void)
{
	uint8_t rndData;
// F_OSC6_COARSE:
	rndData = GetRndValue127();
	//clear upper nibble
	hatVoice.osc.midiFreq &= 0x00ff;
	//set upper nibble
	hatVoice.osc.midiFreq |= rndData << 8;
	osc_recalcFreq(&hatVoice.osc);
// WAVE1_HH:
	rndData = GetRndValue127();
	hatVoice.osc.waveform = rndData;

}

//---------------------------------------------------
void randomHHVoiceDIST(void)
{
		uint8_t rndData;

// HAT_DISTORTION:
	rndData = GetRndValue127();
	setDistortionShape(&hatVoice.distortion,rndData);

}

//---------------------------------------------------
void randomHHVoiceADSR(void)
{
	uint8_t rndData;

// VELOD6:
	rndData = GetRndValue127();
	hatVoice.decayClosed = slopeEg2_calcDecay(rndData);

// VELOD6_OPEN:
	rndData = GetRndValue127();
	hatVoice.decayOpen = slopeEg2_calcDecay(rndData);


}

//---------------------------------------------------
void randomHHVoiceFILTER(void)
{
	uint8_t rndData;
// HAT_FILTER_F:
	rndData = GetRndValue127();
	const float f = rndData/127.f;
	//exponential full range freq
	SVF_directSetFilterValue(&hatVoice.filter,valueShaperF2F(f,FILTER_SHAPER) );

// HAT_RESO:
	rndData = GetRndValue127();
	SVF_setReso(&hatVoice.filter, rndData/127.f);

// CC2_FILTER_TYPE_6:
//Hihat filter
	rndData = GetRndValue6();
	hatVoice.filterType = rndData+1;


// CC2_FILTER_DRIVE_6:
	rndData = GetRndValue127();
#if UNIT_GAIN_DRIVE
	hatVoice.filter.drive = (rndData/127.f);
#else
	SVF_setDrive(&hatVoice.filter, rndData);
#endif


}
