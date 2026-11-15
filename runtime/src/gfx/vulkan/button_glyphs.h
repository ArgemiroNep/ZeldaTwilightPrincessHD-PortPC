// Button prompts of the game (A / B / X / Y icons): they show the NAME of the controller button that does the
// action, with the Wii U Pro Controller's positions kept (Wii U A is the right face button: Xbox "B", PlayStation
// Circle). The icons are tiny textures (res/Layout/*.pack: tt_zelda_button_{a,b,x,y}_text); they are found by the
// hash of their guest data when uploaded and their pixels replaced by drawn letters or shapes (no game data is
// changed, no artwork used). A change of style or controls uploads them again (generation()).
#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace btnglyph {
enum Option : int { kAuto, kOriginal, kXbox, kPlayStation };
void set_option(int opt);   // Option
int option();
int resolved_style();       // kOriginal / kXbox / kPlayStation (Auto follows the connected controller)
uint32_t generation();      // changes with the style, the controls and (Auto) the connected controller
// level 0 of a small texture is about to be uploaded: `packed` (w*h*bytesPerTexel) may be replaced
void maybe_replace(uint32_t w, uint32_t h, uint32_t gx2Format, const uint8_t* guest, size_t guestSize,
                   std::vector<uint8_t>& packed);
// byte offsets of every texel of level 0 of a guest texture (row-major, w*h entries) in its image data: the GX2
// tiling of the surface (format, tile mode, swizzle). False when the format is not handled. (surfaces.cpp)
bool texel_offsets(uint32_t gx2Format, uint32_t w, uint32_t h, uint32_t tileMode, uint32_t swizzle, uint32_t bytesPerTexel,
                   std::vector<uint32_t>& offsets);
}  // namespace btnglyph
