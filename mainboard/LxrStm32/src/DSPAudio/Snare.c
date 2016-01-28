/*
 * Snare.c
 *
 *  Created on: 17.04.2012
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



#include "Snare.h"
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

//instance of the snare voice
INCCMZ SnareVoice snareVoice;

//---------------------------------------------------
void Snare_setPan(const uint8_t pan)
{
	//snareVoice.panL = squareRootLut[127-pan];
	//snareVoice.panR = squareRootLut[pan];
	snareVoice.pan = pan;
}
//---------------------------------------------------
void Snare_init()
{
	SnapEg_init(&snareVoice.snapEg);
	Snare_setPan(0);
	snareVoice.vol = 0.8f;

	//snareVoice.panModifier = 1.f;

	snareVoice.noiseOsc.freq = 440;
	snareVoice.noiseOsc.waveform = 1;
	snareVoice.noiseOsc.fmMod = 0;
	snareVoice.noiseOsc.midiFreq = 70<<8;
	snareVoice.noiseOsc.pitchMod = 1.0f;
	snareVoice.noiseOsc.modNodeValue = 1;

	snareVoice.osc.freq = 440;
	snareVoice.osc.waveform = 1;
	snareVoice.osc.fmMod = 0;
	snareVoice.osc.midiFreq = 70<<8;
	snareVoice.osc.modNodeValue = 1;

	setDistortionShape(&snareVoice.distortion, 2.f);

	snareVoice.volumeMod = 1;

	transient_init(&snareVoice.transGen);

	DecayEg_init(&snareVoice.oscPitchEg);
	snareVoice.egPitchModAmount = 0.5f;

	slopeEg2_init(&snareVoice.oscVolEg);

	setDistortionShape(&snareVoice.distortion, 2.f);

	SVF_init(&snareVoice.filter);

	lfo_init(&snareVoice.lfo);
}
//---------------------------------------------------
void Snare_trigger(const uint8_t vel, const uint8_t note)
{
	lfo_retrigger(3);
	//update velocity modulation
	modNode_updateValue(&velocityModulators[3],vel/127.f);

	float offset = 1;
	if(snareVoice.transGen.waveform==1) //offset mode
	{
		offset -= snareVoice.transGen.volume;
	}
	if(snareVoice.osc.waveform == SINE)
		snareVoice.osc.phase = (0x3ff<<20)*offset;//voiceArray[voiceNr].osc.startPhase ;
	else if(snareVoice.osc.waveform > SINE && snareVoice.osc.waveform <= REC)
		snareVoice.osc.phase = (0xff<<20)*offset;
	else
		snareVoice.osc.phase = 0;

	DecayEg_trigger(&snareVoice.oscPitchEg);
	slopeEg2_trigger(&snareVoice.oscVolEg);
	snareVoice.velo = vel/127.f;

	osc_setBaseNote(&snareVoice.osc,note);
	//TODO noise muss mit transponiert werden

	transient_trigger(&snareVoice.transGen);

	SnapEg_trigger(&snareVoice.snapEg);
}
//---------------------------------------------------
void Snare_calcAsync()
{
	//add modulation eg to osc freq (1 = no change. a+eg = original freq + modulation
	const float egPitchVal = DecayEg_calc(&snareVoice.oscPitchEg);
	const float pitchEgValue = egPitchVal*snareVoice.egPitchModAmount;
	snareVoice.osc.pitchMod = 1+pitchEgValue;

	//calc the osc  vol eg
	snareVoice.egValueOscVol = slopeEg2_calc(&snareVoice.oscVolEg);

	//turn off trigger signal if trigger gate mode is on and volume == 0
	if(trigger_isGateModeOn())
	{
		if(!snareVoice.egValueOscVol) {
			trigger_triggerVoice(TRIGGER_4, TRIGGER_OFF);
			voiceControl_noteOff(TRIGGER_4);
		}
	}

	//calc snap EG if transient sample 0 is activated
	if(snareVoice.transGen.waveform == 0)
	{
		const float snapVal = SnapEg_calc(&snareVoice.snapEg, snareVoice.transGen.pitch);
		snareVoice.osc.pitchMod += snapVal*snareVoice.transGen.volume;;
	}

	osc_setFreq(&snareVoice.osc);
	osc_setFreq(&snareVoice.noiseOsc);
}
//---------------------------------------------------
void Snare_calcSyncBlock(int16_t* buf, const uint8_t size)
{
	int16_t transBuf[size];

	calcNoiseBlock(&snareVoice.noiseOsc,buf,size,0.9f);
	SVF_calcBlockZDF(&snareVoice.filter,snareVoice.filterType,buf,size);

	//calc transient sample
	transient_calcBlock(&snareVoice.transGen,transBuf,size);
	bufferTool_addBuffersSaturating(buf,transBuf,size);

	//calc next osc sample
	calcNextOscSampleBlock(&snareVoice.osc,transBuf,size,(1.f-snareVoice.mix));
	//--AS apply filter to synthesized sound as well here if desired, or combine code for more efficiency
	//SVF_calcBlockZDF(&snareVoice.filter,snareVoice.filterType,transBuf,size);

	// --------------------------------------------------
	//rstephane: My effects ;-)
 		if (fxMaskType>0)
  			calcFxBlock(fxMaskType,buf, size,fx1,fx2,fx3); // TO DO add more PARAM !

			//	calcParamEqBlock(fxMaskType,buf, size,0.4,0.6,0.7,0.8);


 	uint8_t j;
	if(snareVoice.volumeMod)
	{
		for(j=0;j<size;j++)
		{
			//add filter to buffer
			buf[j] *= snareVoice.mix;
			buf[j] = (__QADD16(buf[j],transBuf[j]));
			buf[j] *=  snareVoice.velo * snareVoice.vol * snareVoice.egValueOscVol;
		}
	}
	else
	{
		for(j=0;j<size;j++)
		{
			//add filter to buffer
			buf[j] *= snareVoice.mix;
			buf[j] = (__QADD16(buf[j],transBuf[j]));
			buf[j] *=  snareVoice.vol * snareVoice.egValueOscVol;
		}
	}

	calcDistBlock(&snareVoice.distortion,buf,size);
}
//------------------------------------------------------------------------

//---------------------------------------------------
// rstephane : my functions
//random all the parameters for voice SNARE
//---------------------------------------------------

void randomSnareVoice(uint8_t randomType)
{
	switch(randomType)
	{
		case 1 :
			randomSnareVoiceOSC();
 			break;
		case 2 :
			randomSnareVoiceADSR();
			break;
		case 3 :
			randomSnareVoiceCLICK();
			break;
		case 4 :
			randomSnareVoiceFILTER();
			break;
		case 5 :
			randomSnareVoiceOSC();
			randomSnareVoiceADSR();
			break;
		case 6 :
			randomSnareVoiceOSC();
			randomSnareVoiceFILTER();
			break;
		case 7 :
			randomSnareVoiceOSC();
			randomSnareVoiceCLICK();
			break;
		case 8 :
			randomSnareVoiceOSC();
			randomSnareVoiceFILTER();
			break;
		case 9 :
			randomSnareVoiceCLICK();
			randomSnareVoiceADSR();
			break;
		case 10 :
			randomSnareVoiceCLICK();
			randomSnareVoiceFILTER();
		 	break;
		case 11 :
			randomSnareVoiceFILTER();
			randomSnareVoiceADSR();
			break;
		case 12 :
			randomSnareVoiceADSR();
			randomSnareVoiceOSC();
			randomSnareVoiceCLICK();
			break;
		case 13 :
			randomSnareVoiceOSC();
			randomSnareVoiceFILTER();
			randomSnareVoiceCLICK();
			break;
		case 14 :
			randomSnareVoiceCLICK();
			randomSnareVoiceFILTER();
			randomSnareVoiceADSR();
			break;
		case 15 :
			randomSnareVoiceOSC();
			randomSnareVoiceFILTER();
			randomSnareVoiceADSR();
 			break;
		case 16 :
			randomSnareVoiceOSC();
			randomSnareVoiceCLICK();
			randomSnareVoiceFILTER();
			randomSnareVoiceADSR();
 			break;

		default:
			break;
		break;
	}

}


//---------------------------------------------------
void randomSnareVoiceOSC(void)
{
		uint8_t rndData;
// SNARE_NOISE_F:
		rndData = GetRndValue127();
		snareVoice.noiseOsc.freq = rndData/127.f*22000;

// SNARE_DISTORTION:
		rndData = GetRndValue127();
		setDistortionShape(&snareVoice.distortion,rndData);

// SNARE_MIX:
		rndData = GetRndValue127();
		snareVoice.mix = rndData/127.f;

// OSC_WAVE_SNARE:
		rndData = GetRndValue6();
		snareVoice.osc.waveform = rndData;

// F_OSC4_COARSE:
		rndData = GetRndValue127();
		//rndDataTemp=calcRange(rndData, -63 ,+63,0,127);

		//clear upper nibble
		snareVoice.osc.midiFreq &= 0x00ff;
		//set upper nibble
		snareVoice.osc.midiFreq |= rndData << 8;
		osc_recalcFreq(&snareVoice.osc);

// PITCH_SLOPE4:
		rndData = GetRndValue127();
		DecayEg_setSlope(&snareVoice.oscPitchEg,rndData);


}
//---------------------------------------------------
void randomSnareVoiceCLICK(void)
{
		uint8_t rndData;

// CC2_TRANS4_WAVE:
		// 0 - 14
		do {
	        	rndData = GetRngValue();
	        	rndData = rndData & 0x0000001F;
	        } while ((rndData == 16) || (rndData == 15));
		snareVoice.transGen.waveform = rndData;

// CC2_TRANS4_VOL:
		rndData = GetRndValue127();
		snareVoice.transGen.volume = rndData/127.f;

// CC2_TRANS4_FREQ:
		rndData = GetRndValue127();
		snareVoice.transGen.pitch = 1.f + ((rndData/33.9f)-0.75f) ;

}

//---------------------------------------------------
void randomSnareVoiceADSR(void)
{
	uint8_t rndData;

// VELOA4:
	rndData = GetRndValue127();
	slopeEg2_setAttack(&snareVoice.oscVolEg,rndData,false);
// DECAY
	rndData = GetRndValue127();
	slopeEg2_setDecay(&snareVoice.oscVolEg,rndData,false);

// REPEAT1:
	rndData = GetRndValue127();
	snareVoice.oscVolEg.repeat = rndData;

// EG_SNARE1_SLOPE:
	rndData = GetRndValue127();
	slopeEg2_setSlope(&snareVoice.oscVolEg,rndData);


}

//---------------------------------------------------
void randomSnareVoiceFILTER(void)
{
		uint8_t rndData;

// SNARE_FILTER_F:
		rndData = GetRndValue127();
#if USE_PEAK
		peak_setFreq(&snareVoice.filter, rndData/127.f*20000.f);
#else
		const float f = rndData/127.f;
		//exponential full range freq
		SVF_directSetFilterValue(&snareVoice.filter,valueShaperF2F(f,FILTER_SHAPER) );
#endif

// SNARE_RESO:
		rndData = GetRndValue127();
#if USE_PEAK
		peak_setGain(&snareVoice.filter, rndData/127.f);
#else
		SVF_setReso(&snareVoice.filter, rndData/127.f);
#endif

// CC2_FILTER_TYPE_4:
		rndData = GetRndValue7();
		snareVoice.filterType = rndData + 1; // +1 because 0 is filter off which results in silence

// CC2_FILTER_DRIVE_4:
		rndData = GetRndValue127();
#if UNIT_GAIN_DRIVE
		snareVoice.filter.drive = (rndData/127.f);
#else
		SVF_setDrive(&snareVoice.filter, rndData);
#endif



}
