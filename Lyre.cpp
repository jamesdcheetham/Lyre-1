#include "userosc.h"
#include "math.h"
//#include "usermodfx.h"
#include "simplelfo.hpp"

#include "Lyre.hpp"

static dsp::SimpleLFO s_lfo;
static dsp::SimpleLFO s_lfoB;

static Osc carrierOscillator;
static Osc modulatorOscillator;
float lfo_out, lfo2_out;
float lfoShape;



//lfo
static uint8_t s_lfo_wave;
static float s_param_z, s_param, s_paramB;
static const float s_fs_recip = 1.f / 48000.f;
//lfo

void OSC_INIT(uint32_t platform, uint32_t api) {
	carrierOscillator.waveFold = 0.f;
	carrierOscillator.phase = 0.f;
	carrierOscillator.octave = 0;
	carrierOscillator.semitone = 0;
	carrierOscillator.cent = 0;
	carrierOscillator.fmDepth = 0.f;
  
	modulatorOscillator.phase = 0.f;
//	modulatorOscillator.feedback = 0.f;
	modulatorOscillator.semitone = 0;
	modulatorOscillator.cent = 0;
	
	//lfo

	s_lfo.reset();
		s_lfoB.reset();

	//lfo
}

float hyperLFO() {
	float hyperLFO_out;
	if (lfo_out >= 0.f && lfo2_out >= 0.f) {
	hyperLFO_out = 3.f;}
	else {
	hyperLFO_out = 0.f; }
	return 1.f + hyperLFO_out * carrierOscillator.lfoDepth;
}
	

void OSC_CYCLE(const user_osc_param_t * const params, int32_t *yn, const uint32_t frames) { 

	s_lfo.setF0(s_param/10.f,s_fs_recip);
		s_lfoB.setF0(s_paramB/10.f,s_fs_recip);

  float freqOsc1;
  float modOsc1;
  float fmOsc;

    float lfos;
	float lfoz=lfos;


  
  //get frequency of main oscillator
  const float oscNote = ((params->pitch)>>8) + carrierOscillator.semitone - 24;
  const float oscMod = (params->pitch + carrierOscillator.cent) & 0xFF;
  
  //compute phase increments for main oscillator (from freq)
  const float f0 = osc_notehzf(oscNote);
  const float f1 = osc_notehzf(oscNote+1);
  const float f = clipmaxf(linintf(oscMod * k_note_mod_fscale, f0, f1), k_note_max_hz);
  
  //compute phase increments for FM oscillator (from note)
//  const float w1 = osc_w0f_for_note(oscNote + modulatorOscillator.semitone, oscMod);  

   //get LFO values

	lfo2_out = s_lfo.sine_bi();
		lfo_out = s_lfoB.sine_bi();
  	float lfo = q31_to_f32(params->shape_lfo); 
  
  
  //setting buffer
  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;
  
	for (; y != y_e; ) {

    //p_z = linintf(0.002f, p_z, p);
   
    s_lfo.cycle();
	    s_lfoB.cycle();
	  const float lfo_inc = (lfo - lfoz) / frames;
		lfoShape = lfoz;
		float mainOsc;

		// Main Oscillator
		mainOsc = 0.5f * osc_sinf(carrierOscillator.phase) * (1.f + carrierOscillator.waveFold);

		//FM oscillator
		float fmOsc = osc_sinf(modulatorOscillator.phase);
		const float w1 = (f * (1.f+(modulatorOscillator.semitone/100.f)) * hyperLFO()) * k_samplerate_recipf;
		//fm oscillator set phase		
		modulatorOscillator.phase += w1;
		modulatorOscillator.phase -= (uint32_t)modulatorOscillator.phase;

		
		// wavefolder to main out
		const float audioOut = (mainOsc < -0.5f) ? -1.f - mainOsc : (mainOsc >  0.5f) ? 1.f - mainOsc: mainOsc;		

		
		//set feedback of fm oscillator, then set phase	
//		const float w0 = (f * + (carrierOscillator.lfoDepth * hyperLFO() + fmOsc * carrierOscillator.fmDepth)) * k_samplerate_recipf; 
		const float w0 = (f * hyperLFO() + (fmOsc * (carrierOscillator.fmDepth * (1 + lfoShape))/2.f)) * k_samplerate_recipf;
		carrierOscillator.phase += w0;
		carrierOscillator.phase -= (uint32_t)carrierOscillator.phase;
		
		//fill buffer
		*(y++) = f32_to_q31(audioOut);
		
		 lfoz += lfo_inc;
		  
  }
  lfos=lfoz;
}

void OSC_NOTEON(const user_osc_param_t * const params) {
//		s_lfo.reset();
}

void OSC_NOTEOFF(const user_osc_param_t * const params) {
  (void)params;
}

void OSC_PARAM(uint16_t index, uint16_t value) {
	  const float valf = param_val_to_f32(value);
	  
  switch (index) {
  case k_user_osc_param_id1:
    s_param = value;
	break;  
  case k_user_osc_param_id2:
    s_paramB = value;
	break;  	
  case k_user_osc_param_id3:
    carrierOscillator.waveFold = valf * 10.f;
	break;
  case k_user_osc_param_id4:
	modulatorOscillator.semitone = value;
	break;
//  case k_user_osc_param_id4:
//	carrierOscillator.octave = value * 12;
//	break;
  case k_user_osc_param_id5:
	carrierOscillator.semitone = value;
	break;
  case k_user_osc_param_id6:
	carrierOscillator.cent = value;
	break;	
   case k_user_osc_param_shape:
	carrierOscillator.fmDepth = value * 10;
    break;
  case k_user_osc_param_shiftshape:
    carrierOscillator.lfoDepth = valf;
	break;
  default:
    break;
  }
  
}