// Host audio output: 48 kHz stereo s16 through CoreAudio, fed by the AX frame thread.
#pragma once
#include <cstdint>

namespace audio {

constexpr int kRate = 48000;

void init();                                    // opens the default output device (safe to call twice)
void push(const int16_t* stereo, int frames);   // interleaved L/R
int buffered_frames();                          // frames queued for the device
int target_frames();                            // latency the producer should aim for
void stats(uint64_t& underrun, uint64_t& dropped);  // frames of silence inserted / frames discarded
void flush();                                   // drop what is queued (a save state was loaded)

// TPHD Windows: master volume 0..1 (settings > Audio; saved by the overlay as "masterVolume"; WWHD_AUDIO_VOLUME wins)
float master_volume();
void set_master_volume(float v);
bool master_volume_from_env();

uint32_t dump_frames();                         // frames written to WWHD_AUDIO_DUMP so far

}  // namespace audio
