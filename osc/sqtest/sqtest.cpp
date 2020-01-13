#include "common.h"
#include "userosc.h"

struct Wave {
  struct State {
    float shape;
    float phase;
  };
  struct Params {
    float phase_offset = 0.0;
    uint8_t pitch_offset = 0;
    uint8_t fine_offset = 0;
  };

  float next(const user_osc_param_t* const params) {
    float shape = state.shape + q31_to_f32(params->shape_lfo);
    float duty = clipminmaxf(0.005, shape, 0.995);
    uint8_t note = p.pitch_offset + (params->pitch >> 8);
    uint8_t fine = p.fine_offset + (params->pitch & 0xFF);
    float w0 = osc_w0f_for_note(note, fine);
    state.phase += w0;
    state.phase -= (uint16_t)state.phase;
    return (state.phase < duty) ? -1 : 1;
    /*
    float adj_phase = state.phase = p.phase_offset;
    adj_phase -= (uint16_t)adj_phase;
    return (adj_phase < duty) ? -1 : 1;
    */
  }

  State state;
  Params p;
};

static Wave wave;
static Wave wave2;

void OSC_INIT(uint32_t platform, uint32_t api) {
  wave2.p.phase_offset = 0.1f;
}

// params: oscillator parameter
// yn: write address
// frames: requested frame count for write
void OSC_CYCLE(const user_osc_param_t * const params, int32_t *yn, const uint32_t frames) {
  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;
  for (; y != y_e; ) {
    const float sig = (wave.next(params)/2.0) + (wave2.next(params)/2.0);
    *(y++) = f32_to_q31(sig);
  }
}

void OSC_NOTEON(const user_osc_param_t * const params) { (void)params; }

void OSC_NOTEOFF(const user_osc_param_t * const params) { (void)params; }

void OSC_PARAM(uint16_t index, uint16_t value) {
  switch(index) {
    case 0:
      wave2.p.pitch_offset = value;
    case 6:  // k_user_osc_param_shape
      // 10bit parameter
      wave.state.shape = param_val_to_f32(value); 
      wave2.state.shape = param_val_to_f32(value); 
      //state.duty = clipminmaxf(0.005, param_val_to_f32(value), 0.995);
      break;
    case 7:
      wave2.p.fine_offset = value;
  }
}

