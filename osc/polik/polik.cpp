#include <array>
#include <vector>
#include "common.h"
#include "userosc.h"

struct Wave {
  struct State {
    float shape;
    float phase;
    uint32_t cycles_left;
    bool on;
    uint8_t note;
  };
  struct WaveParams { 
    uint32_t lifetime_cycles;
  };

  void init(const WaveParams& wp, const user_osc_param_t* params) {
    state.cycles_left = wp.lifetime_cycles;
    state.note = params->pitch >> 8;
    state.on = true;
  }

  float next(const user_osc_param_t* const params) {
    //state.cycles_left--;
    //if (state.cycles_left <= 0) state.on = false;
    float shape = state.shape + q31_to_f32(params->shape_lfo);
    float duty = 0.5f;  // clipminmaxf(0.005, shape, 0.995);
    uint8_t note = state.note;  // params->pitch >> 8;
    uint8_t fine = params->pitch & 0xFF;
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
};

static constexpr uint8_t kMaxVoices = 8;
// static Wave waves[kMaxVoices];
static std::array<Wave, kMaxVoices> waves;
static uint8_t next = 0;

void InitNext(const user_osc_param_t* const params, Wave* reuse = nullptr) {
  if (reuse != nullptr) {
    reuse->init({100000}, params);
    return;
  }
  waves[next].init({100000}, params);
  next = (next + 1) % kMaxVoices;
}
Wave* FindNote(uint8_t note, uint8_t default_idx = -1) {
  for (Wave& wave : waves) {
    if (wave.state.on && wave.state.note == note) return &wave;
  }
  if (default_idx == -1) return nullptr;
  return &waves[default_idx];
}

void OSC_INIT(uint32_t platform, uint32_t api) {
}

// params: oscillator parameter
// yn: write address
// frames: requested frame count for write
void OSC_CYCLE(const user_osc_param_t * const params, int32_t *yn, const uint32_t frames) {
  uint8_t note = params->pitch >> 8;
  bool note_found = false;
  for (Wave& wave : waves) {
    if (wave.state.note == note) {
      note_found = true;
      break;
    }
  }
  if (!note_found) {
    InitNext(params);
  }
  q31_t * __restrict y = (q31_t *)yn;
  const q31_t * y_e = y + frames;
  /*
  std::vector<Wave*> active_waves;
  for (int i = 0; i < 8; i++) {
    if (waves[i].state.on) active_waves.push_back(&waves[i]);
  }
  */
  for (; y != y_e; ) {
    uint8_t active_voices = 0;
    float sig_acc = 0.0f;
    for (Wave& wave : waves) {
      if (!wave.state.on) continue;
      sig_acc += wave.next(params);
      active_voices++;
    }
    *(y++) = f32_to_q31(sig_acc / kMaxVoices);
  }
}

void OSC_NOTEON(const user_osc_param_t * const params) { 
  uint8_t note = params->pitch >> 8;
  InitNext(params, FindNote(note));
}

void OSC_NOTEOFF(const user_osc_param_t * const params) { 
  for (Wave& wave : waves) wave.state.on = false;
  /*
  uint8_t note = params->pitch >> 8;
  Wave* this_wave = FindNote(note);
  if (this_wave != nullptr) {
    this_wave->state.on = false;
  }
  */
}

void OSC_PARAM(uint16_t index, uint16_t value) {
  /*
  switch(index) {
    case 6:  // k_user_osc_param_shape
      // 10bit parameter
      wave.state.shape = param_val_to_f32(value); 
      wave2.state.shape = param_val_to_f32(value); 
      //state.duty = clipminmaxf(0.005, param_val_to_f32(value), 0.995);
      break;
  }
  */
}

