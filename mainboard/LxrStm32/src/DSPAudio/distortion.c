/*
 * distortion.c
 *
 *  Created on: 14.04.2012
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

/*
* ADD-ON by Ribas Stephane (egnouf@gmail.com)
* DATE: 20.01.2015
* Note that this program has been modified to include new functions.
* I will make a better code and separate the FX from this file later on.
* Meanwhile, you will find below the authors of certain codes I have used and modified. The codes are all LXR compatible (GNU GPL Licence)
* I thank: MusicDSP.ORG, Sonic Potion Community, OWL Project hoxtonowl.com, and MAZBOX.
*
* The whole modified code is licenced under the same licence of the orginal code: GNU GPL V2
*
*/


#include "distortion.h"
#include "math.h"


// rstephane : declare function for bit wise manipulation
#include <complex.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h> /* memset */
#include <unistd.h> /* close */
#include <stdlib.h>

// rstephane BITS Manipulation
#define bit_get(p,m) ((p) & (m))
#define bit_set(p,m) ((p) |= (m))
#define bit_clear(p,m) ((p) &= ~(m))
#define bit_flip(p,m) ((p) ^= (m))
#define BIT(x) (0x01 << (x))
#define LONGBIT(x) ((unsigned long)0x00000001 << (x))

// for FXs
extern uint8_t fxMaskType; // 0-16 for effects 1
extern uint8_t fx1; // PAR1
extern uint8_t fx2; // PAR2
extern uint8_t fx3; // PAR3
extern uint8_t fx4; // ...
extern uint8_t fx5;
extern uint8_t fx6;
extern uint8_t fx7;

// MISC....
#define SAMPLE_RATE 44100

// for reverse reverb
#define REVERSE_REVERB_BUF_LENGTH 40960

// rstephane : simple compressor
#define QOMP_SENS_RATE 0.5f
#define QOMP_SENS_OFFSET 0.2f
#define QOMP_OFFSET_RATE 0.5f

// DECIMATE
float cnt=0;

// EQ function
// Biquad Parametric EQ filter class
#define   PEQ 1 // Parametric EQ
#define   HSH 2 // High Shelf
#define   LSH 3 // Low SHelf

#define Q_BUTTERWORTH   0.707

// for the EQ Parametric...
float a[3] ; // ai coefficients
float b[3] ; // bi coefficients
float eq_x1, eq_x2, eq_y1, eq_y2 ; // state variables to compute samples

//BiquadDF1 band1, band2, band3, band4; // filters
float eq_fn1, eq_fn2, eq_fn3, eq_fn4; // cutoffs frequencies, normalized


// Octave Down
#define SQRT2 1.414213562f
#define TWOPI 6.2831853071f
#define TWOPI_BY_SAMPLERATE 0.00014247585731f
#define ABS(X) (X>0?X:-X)
bool lastRect = 0;
bool oct1 = 0;
bool oct2 = 0;
bool lastOct1 = 0;
float phase = 0;


typedef struct LPFStruct
{
	float z;
	float x;
	float y;
	float r;
	float c;
}LPF;

// End rstephane



//--------------------------------------------------
__inline void setDistortionShape(Distortion *dist, uint8_t shape)
{
	dist->shape = 2*(shape/128.f)/(1-(shape/128.f));
}
//--------------------------------------------------
void calcDistBlock(const Distortion *dist, int16_t* buf, const uint8_t size)
{
	uint8_t i;
	for(i=0;i<size;i++)
	{
			float x = buf[i]/32767.f;
			x = (1+dist->shape)*x/(1+dist->shape*fabsf(x));
			buf[i] = (x*32767);
	}
}
//--------------------------------------------------

float distortion_calcSampleFloat(const Distortion *dist, float x)
{
	return (1+dist->shape)*x/(1+dist->shape*fabsf(x));
}


//--------------------------------------------------
// rstephane: decimate
// Code from musicdsp.org
// Decimator
// Type : Bit-reducer and sample&hold unit
// References : Posted by tobyear[AT]web[DOT]de
// http://musicdsp.org/showArchiveComment.php?ArchiveID=124

float decimate(float x, int bits, float rate)
{
	// y is the ouput and x the input (single)
	// bits; // bits from 1 to 32 ;-) useless 32 !!!
	// rate; // sample rate from 0 to 1 , 1 is the orignal rate sample.

	long int m=1<<(bits-1);
	float y=0;

	cnt=cnt+rate;
	if (cnt>=1)
	{
		cnt=cnt-1;
		y=(long int)(x*m)/(float)m;
	}

	return y;
}

float decimateAdv(float x, int bits, float rate)
{
	// y is the ouput and x the input (single)
	// bits; // bits from 1 to 32 ;-) useless 32 !!!
	// rate; // sample rate from 0 to 1 , 1 is the orignal rate sample.
	float quantum = powf( 2.0f, bits );
	float y=0;

	cnt=cnt+rate;
	if (cnt>=1)
	{
		cnt=cnt-1;
		y = floorf( x * quantum ) / quantum;

	}

	return y;
}


/*
Waveshaper

References : Posted by Partice Tarrabia and Bram de Jong

Notes :
amount should be in [-1..1[ Plot it and stand back in astonishment! ;)

http://musicdsp.org/archive.php?classid=4#153
*/

float waveshape_tarabia( float in, float amount ) {

float x, y, k;

x = in; // input in [-1..1]
k = 2*amount/(1-amount); // amount in [-1..1]

y = (1+k)*x/(1+k*abs(x));

return y;  // output
}



//--------------------------------------------------
// rstephane: Reverse
uint16_t reverseBits(uint16_t num)
{
    int  NO_OF_BITS = sizeof(num) * 8;
    uint16_t reverse_num = 0;
    int i;

    for (i = 0; i < NO_OF_BITS; i++)
    {
        if((num & (1 << i)))
           reverse_num |= 1 << ((NO_OF_BITS - 1) - i);
   }
    return reverse_num;
}

// rstephane : Range function
float calcRange(uint8_t valueAmount, float old_min ,float old_max,float new_min,float new_max )
{
	float knobValue;
	knobValue = (  ( (valueAmount - old_min) / (old_max - old_min) ) * (new_max - new_min) + new_min  );
	return knobValue;
}


/* The clip function works to sanitize the data if it's to big (could happen) it's cut down to size*/
// inspired by http://hoxtonowl.com/patches-2/ codes...

float clip(float in)
{
        if (in >= 1.0)
            return 1.0;
        else if (in <= -1.0)
            return -1.0;

        return in;
}

int16_t clipInt(int16_t in)
{
	if (in >= 32767)
		return 32767;
		else if (in < -32767)
		return -32768;

		return in;
}

float returnFloat(int16_t in)
{
	float out;
	out = in / 32767.f;
	out = clip(out);
	return out;
}

int16_t returnInt(float in)
{
	int16_t out;
	out = in * 32767.f;
	out = clipInt(out);
	return out;
}

//------------------------------
//
// Octave Down code adapted from
// https://github.com/mazbox/OwlPatches
// original author: mazbox
//------------------------------

// Octave Down
LPF initLPF() {
	LPF temp;
	temp.z = 0;
	temp.x = 0;
	temp.y = 0;
	temp.r = 0;
	temp.c = 0;
	return temp;
}

// Octave Down
LPF setCoeffs(LPF temp, float cutoff, float resonance) {
	float zzz;

	if(cutoff>11025) cutoff = 11025;
	temp.z=cos(TWOPI_BY_SAMPLERATE*cutoff);
	temp.c = 2 - 2*temp.z;
	zzz = temp.z-1;
	zzz = zzz*zzz*zzz;
	temp.r = (SQRT2*sqrt(-zzz)+resonance*(temp.z-1))/(resonance*(temp.z-1));

	return temp;
}

// cutoff in hz/2 (min 10Hz/2), resonance 1 to 10
float processOctDown(float input, LPF temp) {
	temp.x += (input - temp.y)*temp.c;
	temp.y += temp.x;
	temp.x *= temp.r;
	return temp.y;
}

//------------------------------
//
// Call the FXs ....
//
//
//------------------------------

// rstephane : 16 FXs
void calcFxBlock(uint8_t maskType, int16_t* buf,const uint8_t size, uint8_t fx1, uint8_t fx2, uint8_t fx3)
{
	uint8_t i;
	int16_t bufTemp[size];
  float x =0;

  // Fx Amount Dry/wet
  uint8_t fxAmount = fx3;

  // OTO FX
  uint8_t bitRotate;

  // Simple Compressor
  float sig, val, sensitivity, compression;
  float s, o, s2, a, b,compressedSignal;
  // init the compressor factors ...
  a = 0.0f;
  b = 1.0f;

  // SIMPLE FILTER ?
  float lambda, filterGain;
  float y1=0.0;

	// DECIMATE
	float deci_y=0;
	float deci_x=0;
  int reducedBits=0;
  float decimateRate = 0.1;

	LPF inLpf, out1Lpf, out2Lpf;
	// init Octave Down
	inLpf= initLPF();
	out1Lpf=initLPF();
	out2Lpf=initLPF();

	inLpf=setCoeffs(inLpf,100, 0.5);
	out1Lpf=setCoeffs(out1Lpf,200, 0.5);
	out2Lpf=setCoeffs(out2Lpf,200, 0.5);

	// Convertions
	// TO change the range from 0 to 127 to 0.0 to 1.0
	float old_min = 0;
	float old_max = 127;
	float new_min = 0.0;
	float new_max = 1.0;
	float knobValue, dry, wet;

	// dry and Wet
	float dryFloatTemp;
	float wetFloatTemp;
	// knob Value (0 to 127 <--> 0 to 1)
	// knobValue = (  ( (Amount - old_min) / (old_max - old_min) ) * (new_max - new_min) + new_min  );
	knobValue = calcRange(fxAmount, old_min , old_max, new_min, new_max );
	dry = (1-knobValue);
	wet = fabs((1-knobValue)-1);

	// WE copy the Sounds before manipulating it
	for(i=0;i<size;i++)
		bufTemp[i] = buf[i] ;

	// FX PROCESS LOOP
	switch(maskType)
	{

    // QOMPRESSOR
		// original author: blondinou
		// http://hoxtonowl.com/patches-2/
    case 1 :
    sensitivity = fx1/127.0f;
    compression = fx2/127.0f;

    if (sensitivity > 0.0f) {
      s = QOMP_SENS_OFFSET + sensitivity * QOMP_SENS_RATE;
      o = s * (1 + compression * QOMP_OFFSET_RATE);

      s2 = s*s;
      b = (o - s2) / (s - s2);
      a = 1.0f - b;
    } else {
      a = 0.0f;
      b = 1.0f;
    }

    for (i = 0; i < size; i++) {
      if (a != 0.0f && b != 1.0f) {
        // compression
        sig = 1.0f;
        val = returnFloat(bufTemp[i]);

        if (val < 0.0f) {
          sig = -1.0f;
        }

        compressedSignal=clip(sig * a * val * val + b * val);
        bufTemp[i] = returnInt(compressedSignal);
      }
    }
    break;

		// author: Rstephane
    // OTO biscuit FX (Value one is Compresseur alike)
		case 2 :
      bitRotate = fx2/8;

      if (fx1 >64)
      {
        for(i=0;i<size;i++)
          bufTemp[i] = bufTemp[i] >> bitRotate ;
      }
      else
      {
        for(i=0;i<size;i++)
          bufTemp[i] = bufTemp[i] << bitRotate ;
      }
			break;


		case 3 :
    // Strange Low Pass Filter (OWL team)
		// http://hoxtonowl.com/patches-2/
          lambda=fx1/127.0f; // 0.0 .. 1.0
          filterGain=fx2/127.0f; // 0.0 .. 1.0

          for(i = 0; i < size; i++)
          {
            x = returnFloat(bufTemp[i]);
            y1 = lambda*y1 + (1-lambda)*x;
            bufTemp[i] = returnInt(filterGain*y1);
          }
          break;

    // DECIMATOR xx bits / Half SR
		// MusicDSP.org.. see the main function to get the author name.
    case 4 :
		reducedBits=fx1 / 16;
		decimateRate = fx2 / 127.0;

		for(i =0; i < size; i++)
		{

			deci_x=returnFloat(buf[i]);
			deci_y=decimate(deci_x, reducedBits, 1.0-decimateRate);
			bufTemp[i] = returnInt(deci_y);
		}
		break;

    // BIT - REVERSED
		case 5 :
        bitRotate = fx1/32;

        switch  (bitRotate)
        {
            case 1 :
                for(i =0; i < size; i++)
                  bufTemp[i] = ((bufTemp[i] & 0x55555555) << 2) | ((bufTemp[i] & 0xaaaaaaaa) >> 2);
                break;
            case 2 :
                for(i =0; i < size; i++)
                  bufTemp[i] = ((bufTemp[i] & 0x55555555) << 4) | ((bufTemp[i] & 0xaaaaaaaa) >> 4);
                break;
            case 3 :
                for(i =0; i < size; i++)
                  bufTemp[i] = ((bufTemp[i] & 0x55555555) << 8) | ((bufTemp[i] & 0xaaaaaaaa) >> 8);
                break;
            case 4 :
                for(i =0; i < size; i++)
                  bufTemp[i] = ((bufTemp[i] & 0x55555555) << 1) | ((bufTemp[i] & 0xaaaaaaaa) >> 1);
                break;
            default :
                for(i =0; i < size; i++)
                  bufTemp[i] = ((bufTemp[i] & 0x0000ffff) << 2) | ((bufTemp[i] & 0xffff0000) >> 2);
                break;
        }
        break;

    case 6 :
        for(i =0; i < size; i++)
          bufTemp[i] = ((bufTemp[i] & 0x55555555) << 2) | ((bufTemp[i] & 0xaaaaaaaa) >> 2);
          break;
		case 7 :
		// DECIMATOR Advanced (New way of doing bit reduced... hey hey!)
		// MusicDSP.org..
			reducedBits=fx1 / 16;
			decimateRate = fx2 / 127.0;

			for(i =0; i < size; i++)
			{

				deci_x=returnFloat(buf[i]);
				deci_y=decimateAdv(deci_x, reducedBits, 1.0-decimateRate);
				bufTemp[i] = returnInt(deci_y);
			}
			break;

	case 8 :

		// Waveshaper

		for(i =0; i < size; i++)
		{
			x = returnFloat(bufTemp[i]);
			x=waveshape_tarabia(x, fx1/127.f);
			bufTemp[i] = returnInt(x) ;
		}
		break;

default:
			fxMaskType = 0; // we switch off the effect
			maskType = 0;
			break;
		break;
	}

// We merge the Dry and Wet Signal
if (maskType!=0)
	for(i=0;i<size;i++)
	{
		dryFloatTemp = (float) (buf[i]) * dry;
		wetFloatTemp = (float) (bufTemp[i]) * wet;
		buf[i] = wetFloatTemp + dryFloatTemp ;
	}

}



//------------------------------
//
// PARAMETRIC EQ
// NOT WORKING
// original author: OWL Team
// http://hoxtonowl.com/patches-2/
//------------------------------
void initStateVariables(){
  eq_x1=0.f;
  eq_x2=0.f;
  eq_y1=0.f;
  eq_y2=0.f;
}

// function used for PEQ, HSH, LSH
void setCoeffsEq(float normalizedFrequency, float Q, float dbGain, int fType){

  float alpha, c, omega, d, e, gamma, beta ;

  omega = 2*M_PI*normalizedFrequency ;
  c = cosf(omega) ;
  alpha = sinf(omega)/(2*Q);
  d = powf(10,dbGain/40.f);
  gamma = alpha*powf(10,fabsf(dbGain)/40.f);
  e = powf(10,fabsf(dbGain)/20.f);
  beta = 2*alpha*powf(e,0.5f);

switch (fType)
{
  case PEQ: // Parametric EQ
  a[0]=1+gamma/d;
  a[1]=-2*c/a[0];
  a[2]=(1-gamma/d)/a[0];
  b[0]=(1+gamma*d)/a[0];
  b[1]=a[1];
  b[2]=(1-gamma*d)/a[0];
  a[0]=1;
  break;

  case HSH: // High Shelf
  if (dbGain >0){
    a[0]=2*(1+alpha);
    a[1]=-4*c/a[0];
    a[2]=2*(1-alpha)/a[0];
    b[0]=((1+e)-(1-e)*c+beta)/a[0];
    b[1]=2*((1-e)-(1+e)*c)/a[0];
    b[2]=((1+e)-(1-e)*c-beta)/a[0];
    a[0]=1;
  }
  else {
    a[0]=(1+e)-(1-e)*c+beta;
    a[1]=2*((1-e)-(1+e)*c)/a[0];
    a[2]=((1+e)-(1-e)*c-beta)/a[0];
    b[0]=2*(1+alpha)/a[0];
    b[1]=-4*c/a[0];
    b[2]=2*(1-alpha)/a[0];
    a[0]=1;
  }
  break;

  case LSH: // Low Shelf
  if (dbGain >0){
    a[0]=2*(1+alpha);
    a[1]=-4*c/a[0];
    a[2]=2*(1-alpha)/a[0];
    b[0]=((1+e)+(1-e)*c+beta)/a[0];
    b[1]=-(2*((1-e)+(1+e)*c))/a[0];
    b[2]=((1+e)+(1-e)*c-beta)/a[0];
    a[0]=1;

  }
  else {
    a[0]=(1+e)+(1-e)*c+beta;
    a[1]=-2*((1-e)+(1+e)*c)/a[0];
    a[2]=((1+e)+(1-e)*c-beta)/a[0];
    b[0]=(2*(1+alpha))/a[0];
    b[1]=-4*c/a[0];
    b[2]=2*(1-alpha)/a[0];
    a[0]=1;
  }
  break;
}

}

void processEq (const uint8_t numSamples, float* buf){
  float out;
  int i;

  for (i=0;i<numSamples;i++){
    out = b[0]*buf[i]+b[1]*eq_x1+b[2]*eq_x2-a[1]*eq_y1-a[2]*eq_y2 ;
    eq_y2 = eq_y1;
    eq_y1 = out;
    eq_x2 = eq_x1;
    eq_x1 = buf[i];
    buf[i]=out;
  }
}

float getDbGain(float linGain){
  // linGain = 0    <-> -15 dB
  // linGain = 0.5  <-> 0dB
  // linGain = 1    <-> 15dB
  return (linGain-0.5)*30;
}

void calcParamEqBlock(int16_t* buf,const uint8_t size, uint8_t fx4, uint8_t fx5, uint8_t fx6,uint8_t fx7)
{
  uint8_t i;

	// init the EQ parametric
	float eqBuf[size];
	float g1 = fx4/127.0f;
	float g2 = fx5/127.0f;
	float g3 = fx6/127.0f;
	float g4 = fx7/127.0f;

  // WE copy the Sounds before manipulating it
  for(i=0;i<size;i++)
    eqBuf[i] = clip(buf[i]/32767.f);


    // We process the four bands...

    initStateVariables();
    eq_fn1=100/SAMPLE_RATE;
    setCoeffsEq(eq_fn1, Q_BUTTERWORTH, getDbGain(g1),LSH);
    processEq(size, eqBuf);

   initStateVariables();
    eq_fn2=250/SAMPLE_RATE;
    setCoeffsEq(eq_fn2, Q_BUTTERWORTH, getDbGain(g2),PEQ);
    processEq(size, eqBuf);

    initStateVariables();
    eq_fn3=1500/SAMPLE_RATE;
    setCoeffsEq(eq_fn3, Q_BUTTERWORTH, getDbGain(g3),PEQ);
    processEq(size, eqBuf);

    initStateVariables();
    eq_fn4=4000/SAMPLE_RATE;
    setCoeffsEq(eq_fn4, Q_BUTTERWORTH, getDbGain(g4),PEQ);
    processEq(size, eqBuf);


      for(i=0;i<size;i++)
				buf[i] = eqBuf[i]*32767.f;


}
