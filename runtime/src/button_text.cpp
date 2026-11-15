// HUD button letters (TPHD Windows): the HD HUD's A / B / X / Y / R cluster (zelda_game_image_button_info.blo) prints
// its letters as text (J2DTextBox, the rodan font), not with the tt_zelda_button_*_text textures. While such a text
// pane draws (J2DPane::draw hook, aspect.cpp), its string pointer (+0x11C, the GC layout's mStringPtr less the 8
// bytes HD moved its vtable by) is pointed at the NAME of the controller button that does that Wii U action, with the
// Wii U Pro Controller's positions kept (Xbox: Wii U A -> "B", B -> "A", X -> "Y", Y -> "X", R -> "RB"); put back
// after the draw. PlayStation's shapes are not in the font: there, only the shoulders change (R1, L2, ...).
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "gfx/vulkan/button_glyphs.h"
#include "input_map.h"
#include "runtime.h"

namespace btntext {
// diagnostics of the button prompts (tphd.log): WWHD_LOG_GLYPHS=1
static const bool kLogGlyphs = getenv("WWHD_LOG_GLYPHS") != nullptr;
std::atomic<int> g_drawing{0};  // a HUD letter pane is drawing (1..8): surfaces.cpp logs the textures it samples
void done() { g_drawing.store(0, std::memory_order_relaxed); }
namespace {
const char* face_name(int pad) {
    switch (pad) {
    case input_map::kPadA: return "A";
    case input_map::kPadB: return "B";
    case input_map::kPadX: return "X";
    case input_map::kPadY: return "Y";
    default: return nullptr;
    }
}
const char* shoulder_name(int pad, bool ps) {
    switch (pad) {
    case input_map::kPadLB: return ps ? "L1" : "LB";
    case input_map::kPadRB: return ps ? "R1" : "RB";
    case input_map::kPadLT: return ps ? "L2" : "LT";
    case input_map::kPadRT: return ps ? "R2" : "RT";
    default: return nullptr;
    }
}
}  // namespace

// The HUD's letters are pictures (zelda_game_image_button_info.blo: panes a_btn_t, b_btn_t, x_btn_t, y_btn_t, and
// a_btn_t1 / b_btn_t1 of the second set; their textures tt_zelda_button_{a,b,x,y}_text). They are J2DPictureEx (blo2), whose
// texture is in its material (+0x148). While a letter draws, it takes the material of the letter that names the controller's button (Xbox, Pro Controller positions: the A
// picture shows the B texture, ...), with its box narrowed or widened to that letter's shape about its centre.
namespace {
struct Letter { uint32_t tex = 0; uint64_t seen = 0; uint32_t pane = 0; };
Letter g_letters[32];      // A, B, X, Y: the game's own texture of each, as last drawn
uint64_t g_pic_draws = 0;
const float kRatio[4] = {32.f / 46.f, 24.f / 40.f, 32.f / 45.f, 32.f / 40.f};  // the textures' width / height
// 0..3: A, B, X, Y of the button-info screen (a_btn_t, b_btn_t1, ...); 4..7: of the main HUD (zelda_game_image.blo in
// main2D: b_text_a, b_text_b, b_text_x, b_text_y, read from the game's layout); -1: another pane
int letter_of_pane(uint32_t pane) {
    char n[9] = {};
    for (int i = 0; i < 8; i++) n[i] = (char)ld8(pane + 8 + i);
    const char* p = n;
    while (*p == 0 && p < n + 8) p++;  // names are right-aligned in the 8-byte tag
    const std::string s(p, strnlen(p, n + 8 - p));
    char c = 0;
    int set = 0;
    // the HD HUD's visible buttons have their letters baked in (seen in the game's own screenshots): the whole
    // button pictures swap materials
    // (seen in the game's screenshots: itmBtn_1 is the X button, itmBtn_4 the R button, both with their letter)
    // doBtn_cp / atkBtnCP are the visible A and B buttons (tphd.log's pane list: the only visible ones there)
    if (s == "doBtn_cp") c = 'a', set = 12;
    else if (s == "atkBtnCP") c = 'b', set = 12;
    // the file-select screen's "Confirmar (A) / Atrás (B)" letters
    // the file-select screen's "Confirmar (A) / Atrás (B)" icons (20x20, letter baked in; seen in its screenshots)
    else if (s == "a_btn_cp") c = 'a', set = 16;
    else if (s == "b_btn_cp") c = 'b', set = 16;
    // letters that are panes of their own in the file-select / name screens, and the 30x30 button icons of texts
    else if (s == "abtn_t") c = 'a', set = 20;
    else if (s == "bbtn_t") c = 'b', set = 20;
    else if (s == "wa_btn_t") c = 'a', set = 24;
    else if (s == "w_b_btnt") c = 'b', set = 24;
    // (the visible "(A) Hablar" icon is b_text_2; the rest are the same screen's other A / B icons, 28..35 wide)
    else if (s == "a_txt" || s == "b_text_2" || s == "a_btn_t2" || s == "a_btn_t5" ||
             (s == "a_btn_t1" && ldf32(pane + 0x20) - ldf32(pane + 0x18) > 20)) c = 'a', set = 28;
    else if (s == "b_txt" || s == "b_text_1" || (s == "b_text_b" && ldf32(pane + 0x20) - ldf32(pane + 0x18) > 20)) c = 'b', set = 28;
    else if (s == "x_txt") c = 'x', set = 28;
    else if (s == "y_txt") c = 'y', set = 28;
    else if (s == "itmBtn_1") c = 'x', set = 12;
    else if (s == "itmBtn_2") c = 'y', set = 12;
    else if (s == "b_text_s") c = 'a', set = 8;        // the HD HUD's shadows of the A and B letters
    else if (s == "b_texb_s") c = 'b', set = 8;
    else if (s.size() == 8 && s.compare(0, 7, "b_text_") == 0) c = s[7], set = 4;
    else if (s.size() >= 7 && s.compare(1, 6, "_btn_t") == 0 && (s.size() == 7 || s.substr(7) == "1")) c = s[0];
    switch (c) {
    case 'a': return set + 0;
    case 'b': return set + 1;
    case 'x': return set + 2;
    case 'y': return set + 3;
    default: return -1;
    }
}
int letter_of_pad(int pad) {
    switch (pad) {
    case input_map::kPadA: return 0;
    case input_map::kPadB: return 1;
    case input_map::kPadX: return 2;
    case input_map::kPadY: return 3;
    default: return -1;
    }
}
// The texture header (ResTIMG, 32 bytes: format, alpha, width u16, height u16, ..., image offset 0x20 at +0x1C) of a
// picture's material: material -> TEV block -> JUTTexture -> ResTIMG. HD's offsets in those are not known, so the
// pointers are followed by search (once per pane and material; results are cached).
bool heap_ptr(uint32_t p) { return p >= 0x10290000 && p < 0x4FFFF000 && !(p & 3); }
bool timg_at(uint32_t p) {
    if (!heap_ptr(p) || ld32(p + 0x1C) != 0x20) return false;
    const uint32_t w = ld16(p + 2), h = ld16(p + 4);
    return ld8(p) <= 14 && ld8(p + 1) <= 2 && w >= 1 && w <= 2048 && h >= 1 && h <= 2048;
}
uint32_t find_timg(uint32_t mat) {
    for (uint32_t a = 0; a < 0x90; a += 4) {
        const uint32_t p1 = ld32(mat + a);
        if (!heap_ptr(p1)) continue;
        for (uint32_t b = 0; b < 0x120; b += 4) {
            const uint32_t p2 = ld32(p1 + b);
            if (!heap_ptr(p2)) continue;
            if (timg_at(p2)) return p2;
            for (uint32_t d = 0; d < 0x50; d += 4)
                if (const uint32_t p3 = ld32(p2 + d); timg_at(p3)) return p3;
        }
    }
    return 0;
}
struct PaneTex { uint32_t mat = 0, timg = 0; };
std::unordered_map<uint32_t, PaneTex> g_pane_tex;
std::string pane_name(uint32_t pane) {
    char n[9] = {};
    for (int i = 0; i < 8; i++) n[i] = (char)ld8(pane + 8 + i);
    const char* p = n;
    while (*p == 0 && p < n + 8) p++;
    return std::string(p, strnlen(p, n + 8 - p));
}
uint32_t g_b_mat = 0;       // a material with the B letter's texture (tt_zelda_button_b_text), as last drawn
uint64_t g_b_seen = 0;
uint32_t parent_of(uint32_t pane) {
    const uint32_t list = pane ? ld32(pane + 0xE4) : 0;
    return list ? ld32(list + 0xC) : 0;
}
uint32_t root_of(uint32_t pane) {
    for (int i = 0; i < 32; i++) {
        const uint32_t p = parent_of(pane);
        if (!p) break;
        pane = p;
    }
    return pane;
}
// a text pane of each screen (J2DTextBox, the game's text font), as last drawn: the B of "RB" / "LB" is written with it
struct TextPane { uint32_t pane = 0; uint64_t seen = 0; };
std::unordered_map<uint32_t, TextPane> g_text;
bool g_in_extra = false;
struct Extra { uint32_t pane = 0; float gx = 0, gy = 0, cap = 0; uint8_t alpha = 0; } g_extra;
struct Second { uint32_t pane = 0, mat = 0; float x0 = 0, x1 = 0, y0 = 0, y1 = 0; } g_second;  // the second letter of "RB" / "LB"

// R / L letters (im__zelda_button_r_text 24x44; tt_zelda_button_{l,r}_text 32x59): with Xbox prompts and the action
// on a bumper, the letter is drawn narrower with a B after it (the pane is drawn twice, see second()).
// Names from the game's own HD HUD (tphd.log's picture list): b_texr_r is the R letter beside the item button (a
// picture of its own: drawn narrower, with the game's B letter after it); L_btn is the map button with its L baked
// in (the B is drawn over its right half). z_btn_t / r_btn_r: the R of the older layouts.
// The image a material shows, as a key: its TEV block's JUTTexture (+0x6C, +0x64), whose words +0x04 and +0x20 are
// the same for every texture object of one image (seen in tphd.log's dumps of those objects: the L button's two
// objects share them, the R's differ). 0: not found.
uint64_t image_key(uint32_t mat) {
    const uint32_t tev = heap_ptr(mat) ? ld32(mat + 0x6C) : 0;
    const uint32_t jut = heap_ptr(tev) ? ld32(tev + 0x64) : 0;
    if (!heap_ptr(jut)) return 0;
    return (uint64_t(ld32(jut + 4)) << 32) | ld32(jut + 0x20);
}

bool apply_shoulder(uint32_t pane, const std::string& n, std::vector<std::pair<uint32_t, uint32_t>>& saved) {
    int action = -1;
    bool baked = false;
    if (n == "itmBtn_4") action = input_map::kR, baked = true;
    else if (n == "L_btn") action = input_map::kL, baked = true;
    if (action < 0) return false;
    if (baked) return false;  // LB / RB are written in the button's texture when it is uploaded (button_glyphs.cpp)
    if (btnglyph::resolved_style() != btnglyph::kXbox) return false;
    const int pad = input_map::current().pad[action];
    if (pad != input_map::kPadLB && pad != input_map::kPadRB) return false;  // LT / RT: no T among the game's letters
    const uint32_t bmat = g_letters[5].tex ? g_letters[5].tex : g_letters[1].tex;  // the B letter's material
    if (!bmat) return false;
    static bool logged[2];
    if (!logged[action == input_map::kL]) {
        logged[action == input_map::kL] = true;
        LOG("[glyph] HUD letter %s shows %s (pane %s)", action == input_map::kL ? "L" : "R", action == input_map::kL ? "LB" : "RB", n.c_str());
    }
    const float x0 = (float)ldf32(pane + 0x18), y0 = (float)ldf32(pane + 0x1C), x1 = (float)ldf32(pane + 0x20), y1 = (float)ldf32(pane + 0x24);
    const float mid = (x0 + x1) / 2, w = x1 - x0, h = y1 - y0, cy = (y0 + y1) / 2;
    for (uint32_t a : {0x18u, 0x1Cu, 0x20u, 0x24u}) saved.push_back({pane + a, ld32(pane + a)});
    if (baked) {
        if (const auto it = g_text.find(root_of(pane)); it != g_text.end() && g_pic_draws - it->second.seen < 20000) {
            // the B written with the game's text font, right of the baked letter (extra_text): the point is the top
            // left of the letter in screen units, from the picture's global matrix (+0x78)
            const float bh = h * 0.27f, lx = mid + w * 0.045f, ly = cy - bh * 0.60f;
            const uint32_t g = pane + 0x78;
            g_extra = {pane, float(ldf32(g) * lx + ldf32(g + 4) * ly + ldf32(g + 12)),
                       float(ldf32(g + 16) * lx + ldf32(g + 20) * ly + ldf32(g + 28)), float(bh * std::fabs(ldf32(g + 20))), ld8(pane + 0xAA)};
            g_second = {};
            return true;
        }
        // (no text pane of this screen seen yet) the picture as it is, then the B button's letter right of its L
        // the B's box: its height from the picture's (the baked letter is about a third of it), 0.6 as wide
        // measured on the game's screenshots: the baked L / R is about a fifth of the picture's height
        const float bh = h * 0.265f, bw = bh * 0.62f;
        const float bx = mid + w * 0.05f;
        g_second = {pane, bmat, bx, bx + bw, cy - bh * 0.62f, cy + bh * 0.38f};
    } else {
        const float gw = w * 0.62f;
        stf32(pane + 0x18, mid - gw);
        stf32(pane + 0x20, mid);
        g_second = {pane, bmat, mid, mid + gw, y0, y1};
    }
    return true;
}

// Where a material keeps its first texture: J2DMaterial's TEV block is at +0x6C (GC's 0x70 less the vtable word HD
// moved; read all over the recompiled J2DPictureEx code), and in the block the JUTTexture* is the first word past
// its colours that is a heap pointer in both blocks and differs (GC: +0x50 with one stage, +0x64 with two).
// Swapping only that word leaves each button its own material: the game dims and lights buttons through the
// material's colours (seen: the dimmed "back" icon stayed dim whichever letter it showed), so the glow of the
// action button stays at its place while the letters trade.
uint32_t tex_slot(uint32_t matA, uint32_t matB, uint32_t& slotA, uint32_t& slotB) {
    const uint32_t ta = ld32(matA + 0x6C), tb = ld32(matB + 0x6C);
    if (!heap_ptr(ta) || !heap_ptr(tb) || ta == tb) return 0;
    for (uint32_t off = 0x40; off < 0xA0; off += 4) {
        const uint32_t a = ld32(ta + off), b = ld32(tb + off);
        if (heap_ptr(a) && heap_ptr(b) && a != b) {
            slotA = ta + off, slotB = tb + off;
            return off;
        }
    }
    return 0;
}


// Buttons that stand alone (no pair in their screen): the floating "(A) Hablar" prompt of the HUD (a_btn_c1 / a_btn1,
// about 40 wide) takes the texture of the HUD button that names the controller's button.
struct Lone { const char* name; int letter; float minw, maxw; };
const Lone kLone[] = {{"a_btn_c1", 0, 38.5f, 60}, {"a_btn1", 0, 34, 60}};
bool apply_lone(uint32_t pane, const std::string& n, std::vector<std::pair<uint32_t, uint32_t>>& saved) {
    for (const Lone& k : kLone) {
        if (n != k.name) continue;
        const float w = (float)(ldf32(pane + 0x20) - ldf32(pane + 0x18));
        if (w < k.minw || w > k.maxw || btnglyph::resolved_style() != btnglyph::kXbox) return false;
        static const int kAction[4] = {input_map::kA, input_map::kB, input_map::kX, input_map::kY};
        const int d = letter_of_pad(input_map::current().pad[kAction[k.letter]]);
        const uint32_t mat = ld32(pane + 0x148);
        if (d < 0 || d == k.letter || !heap_ptr(mat)) return false;
        const Letter& src = g_letters[12 + d];
        if (!src.tex || g_pic_draws - src.seen > 20000) return false;
        uint32_t mine = 0, theirs = 0;
        if (!tex_slot(mat, src.tex, mine, theirs)) return false;
        static bool told = false;
        if (!told) {
            told = true;
            LOG("[glyph] prompt %s shows %c", k.name, "ABXY"[d]);
        }
        saved.push_back({mine, ld32(mine)});
        st32(mine, ld32(theirs));
        return true;
    }
    return false;
}

bool apply_picture(uint32_t pane, std::vector<std::pair<uint32_t, uint32_t>>& saved) {
    ++g_pic_draws;
    {
        const std::string n = pane_name(pane);
        // diagnostics: each picture pane on its 300th draw, with its place on the screen (its global matrix's
        // translation, +0x84 / +0x94), box, visibility and alpha: tells which pane is which button
        static std::unordered_map<uint32_t, uint32_t> draws;
        static int lines = 0;
        if (draws.size() > 30000) draws.clear();
        if (kLogGlyphs && ++draws[pane] == 300 && lines < 1500) {
            ++lines;
            LOG("[glyph] pane %s at %.0f,%.0f box %.0fx%.0f vis %u alpha %u mat %08X", n.c_str(), ldf32(pane + 0x84), ldf32(pane + 0x94),
                ldf32(pane + 0x20) - ldf32(pane + 0x18), ldf32(pane + 0x24) - ldf32(pane + 0x1C), ld8(pane + 0xA8), ld8(pane + 0xAA),
                ld32(pane + 0x148));
        }
        if (apply_shoulder(pane, n, saved)) return true;
        if (apply_lone(pane, n, saved)) return false;
    }
    const int pl = letter_of_pane(pane);
    if (pl < 0) return false;
    const int set = pl & ~3;
    int l = pl & 3;
    // The prompt screen's icons are set by the game as it needs them ("(A) Hablar", "(+) Saltar", ...): which letter a
    // pane shows is told by its picture, compared with the fixed icons a_txt / b_txt / x_txt / y_txt of that screen.
    // Anything else (+, -, sticks...) is left as it is.
    bool fixed = true;
    if (set == 28) {
        const std::string nm = pane_name(pane);
        fixed = nm == "a_txt" || nm == "b_txt" || nm == "x_txt" || nm == "y_txt";
        if (!fixed) {
            const uint64_t mine = image_key(ld32(pane + 0x148));
            if (!mine) return false;
            l = -1;
            for (int k = 0; k < 4; k++)
                if (g_letters[28 + k].tex && image_key(g_letters[28 + k].tex) == mine) l = k;
            if (l < 0) return false;
        }
    }
    // blo2 screens make J2DPictureEx: its texture is in its J2DMaterial (+0x148; the constructor 02B3AD60 stores
    // it there), not in J2DPicture's own texture slots
    const uint32_t tex = ld32(pane + 0x148);
    static bool seen[32];
    if (kLogGlyphs && !seen[pl]) {
        seen[pl] = true;
        LOG("[glyph] HUD letter pane %c (%s) at %08X, material %08X", "ABXY"[l], set >= 12 ? "button" : set == 8 ? "shadow" : set ? "HUD" : "info", pane, tex);
    }
    if (tex < 0x10000000 || tex >= 0x50000000) return false;
    if (fixed) g_letters[pl] = {tex, g_pic_draws, pane};
    g_drawing.store(pl + 1, std::memory_order_relaxed);
    if (btnglyph::resolved_style() != btnglyph::kXbox) return false;  // PlayStation's shapes are not among the game's letters
    static const int kAction[4] = {input_map::kA, input_map::kB, input_map::kX, input_map::kY};
    const int d = letter_of_pad(input_map::current().pad[kAction[l]]);
    if (d < 0) return false;
    // Xbox colours: the letter is tinted with its button's colour (A green, B red, X blue, Y yellow) through the
    // picture's corner colours (+0x130..+0x13F, the quad's vertex colours; alpha kept). WWHD_NO_BUTTON_COLORS=1: off
    static const bool colours = !getenv("WWHD_NO_BUTTON_COLORS");
    if (colours && set == 4) {
        static const uint32_t kRgb[4] = {0x7EE05A00, 0xFF5A5000, 0x55A8FF00, 0xFFD84000};
        for (int i = 0; i < 4; i++) {
            const uint32_t a = pane + 0x130 + 4 * i, v = ld32(a);
            saved.push_back({a, v});
            st32(a, kRgb[d] | (v & 0xFF));
        }
    }
    if (d == l) return false;
    const Letter& src = g_letters[set + d];
    if (!src.tex || g_pic_draws - src.seen > 20000) return false;  // not drawn lately: the screen may be gone
    static bool logged[32][4];
    if (!logged[pl][d]) {
        logged[pl][d] = true;
        LOG("[glyph] HUD letter %c shows %c (the controller's button for that action)", "ABXY"[l], "ABXY"[d]);
    }
    if (set == 16) {  // only the 20x20 pair of the file-select screens (other screens have a taller b_btn_cp)
        const float w = (float)(ldf32(pane + 0x20) - ldf32(pane + 0x18)), sw = (float)(ldf32(src.pane + 0x20) - ldf32(src.pane + 0x18));
        if (w < 15 || w > 25 || sw < 15 || sw > 25) return false;
    }
    uint32_t mine = 0, theirs = 0;
    if (const uint32_t off = tex_slot(tex, src.tex, mine, theirs)) {  // the texture alone: the material stays
        static bool told = false;
        if (!told) {
            told = true;
            LOG("[glyph] button textures trade at TEV block +0x%X", off);
        }
        saved.push_back({mine, ld32(mine)});
        st32(mine, ld32(theirs));
    } else {
        saved.push_back({pane + 0x148, tex});
        st32(pane + 0x148, src.tex);
    }
    if (set == 12 || set == 16 || set == 28) return false;  // whole buttons keep their box
    // the box (+0x18 x0, +0x20 x1) to the shown letter's shape, same height and centre
    const float x0 = (float)ldf32(pane + 0x18), x1 = (float)ldf32(pane + 0x20);
    const float mid = (x0 + x1) / 2, half = (x1 - x0) / 2 * kRatio[d] / kRatio[l];
    saved.push_back({pane + 0x18, ld32(pane + 0x18)});
    saved.push_back({pane + 0x20, ld32(pane + 0x20)});
    stf32(pane + 0x18, mid - half);
    stf32(pane + 0x20, mid + half);
    return false;
}
}  // namespace

// the second letter of a two-letter name: the pane takes the B's material and the right half of the box
bool second(uint32_t pane, std::vector<std::pair<uint32_t, uint32_t>>& saved) {
    if (g_second.pane != pane || !g_second.mat) return false;
    saved.push_back({pane + 0x148, ld32(pane + 0x148)});
    st32(pane + 0x148, g_second.mat);
    stf32(pane + 0x18, g_second.x0);  // the box's own words are already in `saved`
    stf32(pane + 0x20, g_second.x1);
    stf32(pane + 0x1C, g_second.y0);
    stf32(pane + 0x24, g_second.y1);
    for (int i = 0; i < 4; i++) {  // the added B in the baked letter's dark grey (corner colours, alpha kept)
        const uint32_t a = pane + 0x130 + 4 * i, v = ld32(a);
        saved.push_back({a, v});
        st32(a, 0x2C2C2C00u | (v & 0xFF));
    }
    g_second = {};
    return true;
}

// After a shoulder button's picture: the text pane to draw once more as its "B" (0: none). It is moved to the letter's
// place, sized to the baked letter, coloured its dark grey and given the string "B"; all put back by the caller.
uint32_t extra_text(uint32_t pane, std::vector<std::pair<uint32_t, uint32_t>>& saved) {
    if (g_extra.pane != pane) return 0;
    const Extra e = g_extra;
    g_extra = {};
    const auto it = g_text.find(root_of(pane));
    if (it == g_text.end()) return 0;
    const uint32_t tp = it->second.pane, par = parent_of(tp);
    if (!par || (ld32(tp + 4) >> 8) != 0x544258 || ld32(tp + 0xF4) != 0x100AB64C) return 0;
    const float p00 = (float)ldf32(par + 0x78), p03 = (float)ldf32(par + 0x84), p11 = (float)ldf32(par + 0x8C), p13 = (float)ldf32(par + 0x94);
    const float m00 = (float)ldf32(tp + 0x48), m11 = (float)ldf32(tp + 0x5C);
    if (std::fabs(p00) < 1e-3f || std::fabs(p11) < 1e-3f || std::fabs(m00) < 1e-3f || std::fabs(m11) < 1e-3f) return 0;
    const float fsx = (float)ldf32(tp + 0x114), fsy = (float)ldf32(tp + 0x118);
    if (!(fsy > 1 && fsy < 200)) return 0;
    const float fs = e.cap / 0.70f / std::fabs(p11 * m11);  // a capital is about 0.7 of the font's size
    auto set = [&](uint32_t a, uint32_t v) { saved.push_back({a, ld32(a)}); st32(a, v); };
    auto setf = [&](uint32_t a, float v) { saved.push_back({a, ld32(a)}); stf32(a, v); };
    setf(tp + 0x114, fs * (fsx > 1 ? fsx / fsy : 1.0f));
    setf(tp + 0x118, fs);
    setf(tp + 0x54, (e.gx - p03) / p00 - m00 * (float)ldf32(tp + 0x18));
    setf(tp + 0x64, (e.gy - p13) / p11 - m11 * (float)ldf32(tp + 0x1C));
    const uint32_t buf = mem::fixed_slot(mem::kFixButtonText) + 0xF0;
    st8(buf, 'B');
    st8(buf + 1, 0);
    set(tp + 0x11C, buf);
    set(tp + 0xFC, 0x2C2C2CFF);   // character and gradient colours
    set(tp + 0x100, 0x2C2C2CFF);
    saved.push_back({tp + 0xA8, ld32(tp + 0xA8)});
    st8(tp + 0xA8, 1);
    st8(tp + 0xAA, e.alpha);
    static bool told = false;
    if (!told) {
        told = true;
        LOG("[glyph] shoulder B written with text pane %s (font %.1f -> %.1f) at %.0f,%.0f", pane_name(tp).c_str(), fsy, fs, e.gx, e.gy);
    }
    g_in_extra = true;
    return tp;
}
void extra_done() { g_in_extra = false; }

bool apply(uint32_t pane, std::vector<std::pair<uint32_t, uint32_t>>& saved) {
    if (g_in_extra) return false;
    if ((ld32(pane + 4) >> 8) == 0x504943 && ld32(pane + 0xF4) != 0x100AB64C) return apply_picture(pane, saved);  // 'PIC'
    if (g_in_extra) return false;
    if ((ld32(pane + 4) >> 8) != 0x544258 || ld32(pane + 0xF4) != 0x100AB64C) return false;  // 'TBX', J2DTextBox
    const uint32_t str = ld32(pane + 0x11C);
    if (str < 0x10000000 || str >= 0x50000000) return false;
    if (ld8(str) && ld8(str + 1) && ld8(str + 2) && ld8(pane + 0xA8)) {  // a text pane of this screen, for extra_text
        const float fsy = (float)ldf32(pane + 0x118);
        if (fsy > 6 && fsy < 60) {
            if (g_text.size() > 512) g_text.clear();
            TextPane& t = g_text[root_of(pane)];
            if (!t.pane || t.pane == pane || g_pic_draws - t.seen > 5000) t = {pane, g_pic_draws};
        }
    }
    char t[4] = {};
    for (int i = 0; i < 3; i++) t[i] = (char)ld8(str + i);
    if (t[0] == 0 || (t[1] && t[2])) return false;  // one or two characters only
    static std::unordered_set<std::string> logged;
    if (kLogGlyphs && logged.size() < 40 && logged.insert(t).second) LOG("[glyph] HUD text \"%s\" (pane %08X)", t, pane);
    const int style = btnglyph::resolved_style();
    if (style != btnglyph::kXbox && style != btnglyph::kPlayStation) return false;
    const bool ps = style == btnglyph::kPlayStation;
    const input_map::Mapping m = input_map::current();
    const char* name = nullptr;
    const std::string s = t;
    if (s == "A") name = ps ? nullptr : face_name(m.pad[input_map::kA]);
    else if (s == "B") name = ps ? nullptr : face_name(m.pad[input_map::kB]);
    else if (s == "X") name = ps ? nullptr : face_name(m.pad[input_map::kX]);
    else if (s == "Y") name = ps ? nullptr : face_name(m.pad[input_map::kY]);
    else if (s == "R") name = shoulder_name(m.pad[input_map::kR], ps);
    else if (s == "L") name = shoulder_name(m.pad[input_map::kL], ps);
    else if (s == "ZR") name = shoulder_name(m.pad[input_map::kZR], ps);
    else if (s == "ZL") name = shoulder_name(m.pad[input_map::kZL], ps);
    if (!name || s == name) return false;
    // a buffer of our own (the game's string has no room for "RB"): 16 bytes per pane, round robin
    static uint32_t next = 0;
    const uint32_t buf = mem::fixed_slot(mem::kFixButtonText) + 16 * (next++ & 7);
    for (int i = 0; i < 8; i++) st8(buf + i, i < (int)strlen(name) ? (uint8_t)name[i] : 0);
    saved.push_back({pane + 0x11C, str});
    st32(pane + 0x11C, buf);
    return false;
}
}  // namespace btntext
