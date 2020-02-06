#include <array>
#include <vector>
#include "common.h"
#include "userosc.h"

static int8_t note_offset = 0;

struct Wave {
  struct State {
    float shape;
    float phase;
    bool on;
    int8_t detune;
  };
  struct WaveParams { 
  };

  void init(const WaveParams& wp, const user_osc_param_t* params) {
    state.on = true;
  }

  float next(const user_osc_param_t* const params) {
    if (!state.on) return 0.0;
    //state.cycles_left--;
    //if (state.cycles_left <= 0) state.on = false;
    float shape = state.shape + q31_to_f32(params->shape_lfo);
    float duty = clipminmaxf(0.01, shape / 2, 0.99);
    int16_t note = (params->pitch >> 8) + note_offset;
    // Bring note within range while keeping octave.
    while (note > 0xFF) note -= 12;
    while (note < 0) note += 12;

    int16_t fine = (params->pitch & 0xFF) + state.detune;
    if (fine > 0xFF) {
      note++;
    } else if (fine < 0) {
      note--;
    }
    fine = fine % 0x100;

    float w0 = osc_w0f_for_note(note, (uint8_t)fine);
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

static constexpr uint8_t kMaxVoices = 12;
// static Wave waves[kMaxVoices];
static std::array<Wave, kMaxVoices> waves;
static uint8_t num_voices = 1;
static uint8_t detune = 0;

void OSC_INIT(uint32_t platform, uint32_t api) {
}

// params: oscillator parameter
// yn: write address
// frames: requested frame count for write
void OSC_CYCLE(const user_osc_param_t * const params, int32_t *yn, const uint32_t frames) {
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
    *(y++) = f32_to_q31(sig_acc / num_voices);
  }
}

void OSC_NOTEON(const user_osc_param_t * const params) { 
  // Reset phase.
  for (Wave& wave : waves) wave.state.phase = 0;
}

void OSC_NOTEOFF(const user_osc_param_t * const params) { 
}

void AdjustNumVoices(uint8_t new_num_voices, uint8_t new_detune) {
  num_voices = new_num_voices;
  detune = new_detune;
  uint8_t detune_step = new_detune / (new_num_voices - 1) / 2;
  int8_t curr = detune;
  for (int i = 1; i < kMaxVoices; i++) {
    waves[i].state.on = (i < num_voices);
    if (i < num_voices) {
      waves[i].state.detune = curr;
      curr = -curr;
      if (curr > 0) {
        curr -= detune_step;
      }
    }
  }
}

void OSC_PARAM(uint16_t index, uint16_t value) {
  switch(index) {
    case 0:  // k_user_osc_param_id1:  // Voices, [1, 12]
      AdjustNumVoices((uint8_t)value+1, detune);
      break;
    case 1:  // k_user_osc_param_id2:  // Shape, [1, 3]
      break;
    case 2:  // k_user_osc_param_id3:  // Detune, [0, 100]
      AdjustNumVoices(num_voices, (uint8_t)value);
      break;
    case 3:  // k_user_osc_param_id4:  // Offset, [0, 72], subtract 36 to get offset value.
      note_offset = (int8_t)(value - 36);
      break;
    case 6:  // shape
      float shape = param_val_to_f32(value);
      for (Wave& wave : waves) wave.state.shape = shape;
      break;
  }
}

