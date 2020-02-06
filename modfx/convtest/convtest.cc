#include <array>
#include "fixed_math.h"
#include "usermodfx.h"

template <uint32_t B>
struct Buffer {
    float[B] buf;
    uint32_t pos = 0;
    void Write(const float value) {
        buf[pos] = value;
        pos = (pos + 1) % B;
    }
    float Read(const uint32_t offset) {
        return buf[(pos - offset) % B];
    }
};

template <uint32_t K, typename BufType>
struct ConvKernel {
    std::array<std::pair<uint32_t, float>, K> points;
    void Convolve(const BufType& buf, float* output, uint32_t frames) {
        float * __restrict y = output;
        const float * y_e = y + frames;
        for (uint32_t buf_offset = frames - 1; buf_offset >= 0; buf_offset--) {
            for (const auto& point : points) {
                (*y) += 0; //buf.Read(point.first + buf_offset) * point.second;
            }
        }
    }
};

static Buffer<2048> buf;
static ConvKernel<2, Buffer<2048>> one_tap_delay ({{0, 1.0}, {1024, 0.5}});

void MODFX_INIT(uint32_t platform, uint32_t api) {}

void MODFX_PROCESS(const float *main_xn, float *main_yn, const float *sub_xn, float *sub_yn, uint32_t frames) {
    float * __restrict y = main_yn;
    const float * y_e = y + frames;
    for (uint16_t i = 0; i < frames ; i++){
        *(y + i) = *(main_xn + i);
    }
    /*

    for (uint32_t i = 0; i < frames; i++) {
        buf.Write(*(main_xn + i));
    }
    one_tap_delay.Convolve(buf, main_yn, frames);
    */
}

void MODFX_PARAM(uint8_t index, int32_t value) {}