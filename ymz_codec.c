/*
	Encode and decode algorithms for
	YMZ280B ADPCM
	
	2019 by superctr.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

inline int16_t ymz_step(uint8_t step, int16_t* history, int16_t* step_size)
{
	static const int ct_table[8] = {
		230, 230, 230, 230, 307, 409, 512, 614
	};

	int sign = step & 8;
	int delta = step & 7;
	int diff = ((1+(delta<<1)) * *step_size) >> 3;
	int newval = *history * 254 / 256;
	int nstep = (ct_table[delta] * *step_size) >> 8;
	if (sign > 0)
		newval -= diff;
	else
		newval += diff;
	//*step_size = CLAMP(nstep, 511, 32767);
	*step_size = CLAMP(nstep, 127, 24576);
	*history = newval = CLAMP(newval, -32768, 32767);
	return newval;
}

void ymz_encode(int16_t *buffer,uint8_t *outbuffer,long len)
{
	long i;
	int16_t step_size = 127;
	int16_t history = 0;
	uint8_t buf_sample = 0, nibble = 0;
	uint8_t adpcm_sample;

	for(i=0;i<len;i++)
	{
		// we remove a few bits of accuracy to reduce some noise.
		int16_t step = ((*buffer++) & 0xfff8) - history;
		adpcm_sample = (abs(step<<16)) / (step_size<<14);
		adpcm_sample = CLAMP(adpcm_sample, 0, 7);
		if(step < 0)
			adpcm_sample |= 8;
		if(nibble)
			*outbuffer++ = buf_sample | (adpcm_sample&15);
		else
			buf_sample = (adpcm_sample&15)<<4;
		nibble^=1;
		ymz_step(adpcm_sample, &history, &step_size);
	}
}

void ymz_decode(uint8_t *buffer,int16_t *outbuffer,long len)
{
	long i;
	
	int16_t step_size = 127;
	int16_t history = 0;
	uint8_t nibble = 0;
	
	for(i=0;i<len;i++)
	{
		int8_t step = (*(int8_t*)buffer)<<nibble;
		step >>= 4;
		if(nibble)
			buffer++;
		nibble^=4;
		*outbuffer++ = ymz_step(step, &history, &step_size);
	}
}