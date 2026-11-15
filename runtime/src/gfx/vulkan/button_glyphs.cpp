#include "button_glyphs.h"
#include <filesystem>

#include <SDL3/SDL.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_set>

#include "input_map.h"
#include "runtime.h"

namespace btnglyph {
namespace {
std::atomic<int> g_option{kAuto};

uint64_t fnv(const uint8_t* p, size_t n) {
    uint64_t h = 0xcbf29ce484222325ull;
    for (size_t i = 0; i < n; i++) h = (h ^ p[i]) * 0x100000001b3ull;
    return h;
}

// glyphs: letters (Xbox) or shapes (PlayStation), by the host button's position name (Xbox A, B, X, Y)
enum Glyph { kNone, kLetA, kLetB, kLetX, kLetY, kCross, kCircle, kSquare, kTriangle, kLetL, kLetR, kLetT, kLet1, kLet2 };
struct Known { uint64_t hash; uint32_t w, h; int action; const char* name; };
// FNV-1a of the first 4096 bytes of each texture's guest data (computed from the GTX files of button.pack)
const Known kKnown[] = {
    {0x69bbd79f6047eb5bull, 32, 46, input_map::kA, "A"},
    {0xa2cdeba96374eb81ull, 24, 40, input_map::kB, "B"},
    {0x0bb7da621d2847feull, 32, 45, input_map::kX, "X"},
    {0x0e1d0ca2a3f7e163ull, 32, 40, input_map::kY, "Y"},
    {0xd09b2183feaaf281ull, 32, 46, input_map::kA, "A"},   // the Title2D / Revo variant
    {0xf138661e040353d5ull, 24, 44, input_map::kR, "R"},   // the shoulder: its name on the controller (RB / R1, ...)
};

struct P { double x, y; };
double seg_dist(double px, double py, double ax, double ay, double bx, double by) {
    double dx = bx - ax, dy = by - ay, l = dx * dx + dy * dy;
    double t = l > 0 ? ((px - ax) * dx + (py - ay) * dy) / l : 0;
    t = std::clamp(t, 0.0, 1.0);
    return std::hypot(px - (ax + t * dx), py - (ay + t * dy));
}
double poly_dist(double px, double py, const std::vector<P>& v) {
    double d = 1e9;
    for (size_t i = 0; i + 1 < v.size(); i++) d = std::min(d, seg_dist(px, py, v[i].x, v[i].y, v[i + 1].x, v[i + 1].y));
    return d;
}
void arc(std::vector<P>& v, double cx, double cy, double rx, double ry, double a0, double a1) {
    const int n = std::max(12, int(std::fabs(a1 - a0) / 6));  // a round circle, not a dodecagon
    for (int i = 0; i <= n; i++) {
        double a = (a0 + (a1 - a0) * i / double(n)) * 3.14159265358979 / 180.0;
        v.push_back({cx + rx * std::cos(a), cy + ry * std::sin(a)});
    }
}
// strokes of a glyph in a unit box x,y in [-0.5, 0.5] (y down); shapes are drawn square, letters 0.72 wide
std::vector<std::vector<P>> strokes(Glyph g) {
    std::vector<std::vector<P>> s;
    switch (g) {
    case kLetA: s = {{{-0.5, 0.5}, {0, -0.5}, {0.5, 0.5}}, {{-0.27, 0.14}, {0.27, 0.14}}}; break;
    case kLetX: s = {{{-0.5, -0.5}, {0.5, 0.5}}, {{0.5, -0.5}, {-0.5, 0.5}}}; break;
    case kLetY: s = {{{-0.5, -0.5}, {0, 0.02}, {0.5, -0.5}}, {{0, 0.02}, {0, 0.5}}}; break;
    case kLetB: {
        std::vector<P> top = {{-0.42, -0.5}, {0.02, -0.5}}, bot = {{-0.42, 0.0}, {0.08, 0.0}};
        arc(top, 0.02, -0.25, 0.42, 0.25, -90, 90);
        top.push_back({-0.42, 0.0});
        arc(bot, 0.08, 0.25, 0.42, 0.25, -90, 90);
        bot.push_back({-0.42, 0.5});
        s = {{{-0.42, -0.5}, {-0.42, 0.5}}, top, bot};
        break;
    }
    case kLetL: s = {{{-0.35, -0.5}, {-0.35, 0.5}, {0.4, 0.5}}}; break;
    case kLetT: s = {{{-0.5, -0.5}, {0.5, -0.5}}, {{0, -0.5}, {0, 0.5}}}; break;
    case kLet1: s = {{{-0.3, -0.3}, {0.1, -0.5}, {0.1, 0.5}}, {{-0.3, 0.5}, {0.4, 0.5}}}; break;
    case kLet2: {
        std::vector<P> top;
        arc(top, 0, -0.25, 0.4, 0.25, 180, 400);
        top.push_back({-0.45, 0.5});
        top.push_back({0.45, 0.5});
        s = {top};
        break;
    }
    case kLetR: {
        std::vector<P> bowl = {{-0.4, -0.5}, {0.0, -0.5}};
        arc(bowl, 0.0, -0.22, 0.4, 0.28, -90, 90);
        bowl.push_back({-0.4, 0.06});
        s = {{{-0.4, -0.5}, {-0.4, 0.5}}, bowl, {{0.0, 0.06}, {0.42, 0.5}}};
        break;
    }
    case kCross: s = {{{-0.42, -0.42}, {0.42, 0.42}}, {{0.42, -0.42}, {-0.42, 0.42}}}; break;
    case kCircle: { std::vector<P> c; arc(c, 0, 0, 0.45, 0.45, 0, 360); s = {c}; break; }
    case kSquare: s = {{{-0.4, -0.4}, {0.4, -0.4}, {0.4, 0.4}, {-0.4, 0.4}, {-0.4, -0.4}}}; break;
    case kTriangle: s = {{{0, -0.47}, {0.47, 0.37}, {-0.47, 0.37}, {0, -0.47}}}; break;
    default: break;
    }
    return s;
}

// coverage 0..255 of one or more glyphs side by side, centred in a w x h box, 4x4 supersampling
void draw(const std::vector<Glyph>& gs, uint32_t w, uint32_t h, std::vector<uint8_t>& out) {
    const int n = (int)gs.size();
    out.assign((size_t)w * h, 0);
    if (!n) return;
    const double cw = double(w) / n;  // each glyph's cell
    struct Cell { std::vector<std::vector<P>> st; double lw, lh, t; double cx; };
    std::vector<Cell> cells;
    for (int k = 0; k < n; k++) {
        const bool letter = gs[k] >= kLetA && gs[k] <= kLetY ? true : gs[k] >= kLetL;
        Cell c;
        c.lh = std::min<double>(h * (n > 1 ? 0.62 : 0.80), cw * 0.9 / (letter ? 0.72 : 1.0));
        c.lw = letter ? c.lh * 0.72 : c.lh;
        c.t = std::max(1.4, c.lh * (letter ? 0.17 : 0.14));
        c.cx = cw * (k + 0.5) - w / 2.0;
        for (const auto& poly : strokes(gs[k])) {
            std::vector<P> sc;
            for (const P& q : poly) sc.push_back({q.x * c.lw + c.cx, q.y * c.lh});
            c.st.push_back(std::move(sc));
        }
        cells.push_back(std::move(c));
    }
    for (uint32_t y = 0; y < h; y++)
        for (uint32_t x = 0; x < w; x++) {
            int hit = 0;
            for (int j = 0; j < 4; j++)
                for (int i = 0; i < 4; i++) {
                    const double px = x + (i + 0.5) / 4 - w / 2.0, py = y + (j + 0.5) / 4 - h / 2.0;
                    bool on = false;
                    for (const Cell& c : cells) {
                        if (std::fabs(px - c.cx) > cw) continue;
                        for (const auto& poly : c.st)
                            if (poly_dist(px, py, poly) <= c.t / 2) { on = true; break; }
                        if (on) break;
                    }
                    if (on) hit++;
                }
            out[(size_t)y * w + x] = (uint8_t)(hit * 255 / 16);
        }
}
void draw(Glyph g, uint32_t w, uint32_t h, std::vector<uint8_t>& out) { draw(std::vector<Glyph>{g}, w, h, out); }

// the shoulder/trigger button's name on the style's controller (Xbox LB / RB / LT / RT, PlayStation L1 / R1 / L2 / R2)
std::vector<Glyph> shoulder_for(int style, int pad) {
    const bool ps = style == kPlayStation;
    switch (pad) {
    case input_map::kPadLB: return ps ? std::vector<Glyph>{kLetL, kLet1} : std::vector<Glyph>{kLetL, kLetB};
    case input_map::kPadRB: return ps ? std::vector<Glyph>{kLetR, kLet1} : std::vector<Glyph>{kLetR, kLetB};
    case input_map::kPadLT: return ps ? std::vector<Glyph>{kLetL, kLet2} : std::vector<Glyph>{kLetL, kLetT};
    case input_map::kPadRT: return ps ? std::vector<Glyph>{kLetR, kLet2} : std::vector<Glyph>{kLetR, kLetT};
    default: return {};
    }
}

// the name of the host button bound to a Wii U action, as a glyph of the style
Glyph glyph_for(int style, int pad) {
    switch (pad) {
    case input_map::kPadA: return style == kPlayStation ? kCross : kLetA;
    case input_map::kPadB: return style == kPlayStation ? kCircle : kLetB;
    case input_map::kPadX: return style == kPlayStation ? kSquare : kLetX;
    case input_map::kPadY: return style == kPlayStation ? kTriangle : kLetY;
    default: return kNone;
    }
}
std::atomic<uint32_t> g_gen{1};
}  // namespace

void set_option(int opt) { g_option = std::clamp(opt, 0, 3); g_gen++; }
int option() { return g_option.load(); }

int resolved_style() {
    int o = g_option.load();
    if (o != kAuto) return o;
    // Auto: the connected controller's family, looked at once a second (generation() asks for every small texture):
    // PlayStation pads -> PlayStation; Nintendo pads (Wii U / Switch Pro, Joy-Con, GameCube adapters: SDL's Nintendo
    // types or "Nintendo" / "Wii U" in the name) and no pad at all (keyboard) -> the game's own Wii U letters; any other
    // pad (Xbox, generic XInput) -> Xbox.
    static std::atomic<int> cached{kOriginal};
    static std::atomic<uint64_t> checked{0};
    const uint64_t now = SDL_GetTicks();
    if (checked.load() == 0 || now - checked.load() > 1000) {
        checked = now ? now : 1;
        int n = 0, style = kOriginal;
        if (SDL_JoystickID* ids = SDL_GetGamepads(&n)) {
            for (int i = 0; i < n && style == kOriginal; i++) {
                const SDL_GamepadType t = SDL_GetGamepadTypeForID(ids[i]);
                const char* nm = SDL_GetGamepadNameForID(ids[i]);
                const std::string name = nm ? nm : "";
                const bool nintendo = t == SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO || t == SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT ||
                                      t == SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT || t == SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR ||
                                      name.find("Nintendo") != std::string::npos || name.find("Wii U") != std::string::npos ||
                                      name.find("WiiU") != std::string::npos || name.find("GameCube") != std::string::npos;
                if (t == SDL_GAMEPAD_TYPE_PS3 || t == SDL_GAMEPAD_TYPE_PS4 || t == SDL_GAMEPAD_TYPE_PS5) style = kPlayStation;
                else if (!nintendo) style = kXbox;
            }
            SDL_free(ids);
        }
        if (style != cached.load()) LOG("[glyph] button prompts follow the controller: %s", style == kXbox ? "Xbox" : style == kPlayStation ? "PlayStation" : "Wii U (original)");
        cached = style;
    }
    return cached.load();
}

namespace {
// ---------------------------------------------------------------- button captions in the game's own textures
// The HD screens draw their buttons with R8G8 textures (byte 0 intensity, byte 1 alpha; main2DR / ringresR .pack)
// that carry the Wii U name baked in. When one of them is uploaded with Xbox prompts, its caption is rebuilt from the
// game's own pixels into the name of the controller button that does that action: LB / RB / LT / RT for the
// shoulders and triggers, and the face letters (X <-> Y of the item screen, A -> B). Each letter's shading is lifted
// from a source button (the L, R and B buttons of the HUD; the T is built from the L's stem and bar), measured as
// pixel over the button's body (a Coons patch over the box's edges), and laid over the caption's place after the old
// caption was filled with the body: same height, centred. Textures are known by the hash of their whole data and the
// boxes are the captions' places, both measured on the game's files. Undone (not applied) for other prompt styles.
constexpr uint64_t kHashL83 = 0x1720bfa2969bbe1cull, kHashR83 = 0xfeda96baaf674588ull, kHashB88 = 0x315ddcce3c63c38full;
std::vector<float> g_src_l, g_src_r, g_src_b;            // the source buttons' intensities, once seen
std::vector<uint8_t> g_ring_x, g_ring_y;                  // the item screen's X and Y buttons (packed), once seen
struct Box { int x0, y0, x1, y1; };
struct Map {  // a letter's shading: w x h ratios
    int w = 0, h = 0;
    std::vector<float> v;
    float at(int x, int y) const { return v[size_t(y) * w + x]; }
};
// the body under a box: Coons patch from the two texels around each edge
std::vector<float> body(const std::vector<float>& I, int W, const Box& b) {
    const int w = b.x1 - b.x0, h = b.y1 - b.y0;
    auto px = [&](int x, int y) { return I[size_t(y) * W + x]; };
    std::vector<float> out(size_t(w) * h);
    const float c00 = px(b.x0 - 1, b.y0 - 1), c10 = px(b.x1, b.y0 - 1), c01 = px(b.x0 - 1, b.y1), c11 = px(b.x1, b.y1);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            const float u = (x + 0.5f) / w, v = (y + 0.5f) / h;
            const float l = (px(b.x0 - 2, b.y0 + y) + px(b.x0 - 1, b.y0 + y)) / 2, r = (px(b.x1, b.y0 + y) + px(b.x1 + 1, b.y0 + y)) / 2;
            const float t = (px(b.x0 + x, b.y0 - 2) + px(b.x0 + x, b.y0 - 1)) / 2, d = (px(b.x0 + x, b.y1) + px(b.x0 + x, b.y1 + 1)) / 2;
            const float hz = l * (1 - u) + r * u, vt = t * (1 - v) + d * v;
            const float bil = c00 * (1 - u) * (1 - v) + c10 * u * (1 - v) + c01 * (1 - u) * v + c11 * u * v;
            out[size_t(y) * w + x] = std::max(1.0f, hz + vt - bil);
        }
    return out;
}
// a letter's shading over its box; faint deviations of the body (<8%) snap to 1, the rest is stretched to keep its depth
Map letter(const std::vector<float>& I, int W, const Box& b) {
    Map m{b.x1 - b.x0, b.y1 - b.y0, {}};
    const std::vector<float> bd = body(I, W, b);
    m.v.resize(size_t(m.w) * m.h);
    float lo = 1;
    for (int y = 0; y < m.h; y++)
        for (int x = 0; x < m.w; x++) {
            const float r = std::min(1.0f, I[size_t(b.y0 + y) * W + b.x0 + x] / bd[size_t(y) * m.w + x]);
            m.v[size_t(y) * m.w + x] = r;
            lo = std::min(lo, r);
        }
    const float knee = 0.92f;
    if (lo < knee)
        for (float& r : m.v) r = r >= knee ? 1.0f : lo + (r - lo) * (1 - lo) / (knee - lo);
    return m;
}
Map resample(const Map& m, int nw, int nh) {
    Map o{nw, nh, std::vector<float>(size_t(nw) * nh, 1.0f)};
    for (int y = 0; y < nh; y++)
        for (int x = 0; x < nw; x++) {
            const float sx0 = float(x) * m.w / nw, sx1 = float(x + 1) * m.w / nw, sy0 = float(y) * m.h / nh, sy1 = float(y + 1) * m.h / nh;
            float sum = 0, area = 0;
            for (int yy = int(sy0); yy < m.h && yy < sy1; yy++)
                for (int xx = int(sx0); xx < m.w && xx < sx1; xx++) {
                    const float a = (std::min(sx1, xx + 1.0f) - std::max(sx0, float(xx))) * (std::min(sy1, yy + 1.0f) - std::max(sy0, float(yy)));
                    if (a > 0) sum += m.at(xx, yy) * a, area += a;
                }
            o.v[size_t(y) * nw + x] = area > 0 ? sum / area : 1.0f;
        }
    return o;
}
// a T from an L: the L's bottom bar on top, its stem centred under it
Map make_t(const Map& l) {
    Map t{l.w, l.h, std::vector<float>(l.v.size(), 1.0f)};
    std::vector<int> stem, bar;
    int gx0 = l.w, gx1 = 0, gy0 = l.h, gy1 = 0;
    for (int x = 0; x < l.w; x++) {
        int n = 0;
        for (int y = 0; y < l.h; y++) n += l.at(x, y) < 0.8f;
        if (n * 2 > l.h) stem.push_back(x);
    }
    for (int y = 0; y < l.h; y++) {
        int n = 0;
        for (int x = 0; x < l.w; x++)
            if (l.at(x, y) < 0.8f) n++, gx0 = std::min(gx0, x), gx1 = std::max(gx1, x + 1), gy0 = std::min(gy0, y), gy1 = std::max(gy1, y + 1);
        if (n * 2 > l.w) bar.push_back(y);
    }
    if (stem.empty() || bar.empty() || gx1 <= gx0) return t;
    const int s0 = stem.front(), sw = stem.back() + 1 - s0, b0 = bar.front(), bh = bar.back() + 1 - b0;
    for (int y = 0; y < bh; y++)
        for (int x = 0; x < l.w; x++) t.v[size_t(gy0 + y) * l.w + x] = l.at(x, b0 + y);
    const int c0 = (gx0 + gx1) / 2 - sw / 2;
    for (int y = gy0; y < gy1; y++)
        for (int x = 0; x < sw; x++) {
            float& d = t.v[size_t(y) * l.w + c0 + x];
            d = std::min(d, l.at(s0 + x, y));
        }
    return t;
}
// the caption in a box replaced by `word` (I is rewritten)
void write_word(std::vector<float>& I, int W, const Box& b, const std::vector<const Map*>& word, int gap = -1) {
    const std::vector<float> bd = body(I, W, b);
    const int bw = b.x1 - b.x0, th = b.y1 - b.y0;
    for (int y = 0; y < th; y++)
        for (int x = 0; x < bw; x++) I[size_t(b.y0 + y) * W + b.x0 + x] = bd[size_t(y) * bw + x];
    std::vector<Map> rs;
    int total = 0;
    for (const Map* g : word) {
        rs.push_back(resample(*g, std::max(1, int(std::lround(float(g->w) * th / g->h))), th));
        total += rs.back().w;
    }
    total += gap * (int(rs.size()) - 1);
    int nx = int(std::lround((b.x0 + b.x1) / 2.0f - total / 2.0f));
    for (const Map& r : rs) {
        for (int y = 0; y < th; y++)
            for (int x = 0; x < r.w; x++)
                if (nx + x >= 0 && nx + x < W) I[size_t(b.y0 + y) * W + nx + x] *= r.at(x, y);
        nx += r.w + gap;
    }
}
// "LB" / "RB": the method of v0.8.23 (the user's pick), generalised to any target box. Body interpolated across each
// row only, the letters' shading taken as it is (no snapping), the B scaled so that its letter is as tall as the L / R
// (glyph heights 16 of 29), centred on them, 2 texels of overlap; all scaled from the HUD's 83x83 L / R to the target.
float body_h(const std::vector<float>& I, int W, const Box& b, int x, int y) {
    auto px = [&](int xx, int yy) { return I[size_t(yy) * W + xx]; };
    const float l = (px(b.x0 - 2, y) + px(b.x0 - 1, y)) / 2, r = (px(b.x1, y) + px(b.x1 + 1, y)) / 2;
    const float t = (x - b.x0 + 0.5f) / (b.x1 - b.x0);
    return std::max(1.0f, l * (1 - t) + r * t);
}
Map letter_h(const std::vector<float>& I, int W, const Box& b) {
    Map m{b.x1 - b.x0, b.y1 - b.y0, {}};
    m.v.resize(size_t(m.w) * m.h);
    for (int y = 0; y < m.h; y++)
        for (int x = 0; x < m.w; x++) m.v[size_t(y) * m.w + x] = I[size_t(b.y0 + y) * W + b.x0 + x] / body_h(I, W, b, b.x0 + x, b.y0 + y);
    return m;
}
void write_xb(std::vector<float>& I, int W, const Box& tb, const std::vector<float>& src, const Box& sb) {
    const Box bb = {30, 22, 59, 55};  // the B in the B button (88x88), 2 texels around
    const Map ml = letter_h(src, 83, sb), mb = letter_h(g_src_b, 88, bb);
    const float k = float(sb.y1 - sb.y0 - 4) / float(bb.y1 - bb.y0 - 4), sc = float(tb.y1 - tb.y0) / float(sb.y1 - sb.y0);
    const int lh = tb.y1 - tb.y0, lw = std::max(1, int(std::lround(ml.w * sc)));
    const int bh = std::max(1, int(std::lround((bb.y1 - bb.y0) * k * sc))), bw = std::max(1, int(std::lround((bb.x1 - bb.x0) * k * sc)));
    const int by = tb.y0 + (lh - bh) / 2, gap = int(std::lround(-2 * sc));
    const Map rl = resample(ml, lw, lh), rb = resample(mb, bw, bh);
    std::vector<float> out = I;
    for (int y = tb.y0; y < tb.y1; y++)
        for (int x = tb.x0; x < tb.x1; x++) out[size_t(y) * W + x] = body_h(I, W, tb, x, y);
    const int total = lw + gap + bw, nx = int(std::lround((tb.x0 + tb.x1) / 2.0f - total / 2.0f));
    for (int y = 0; y < lh; y++)
        for (int x = 0; x < lw; x++)
            if (nx + x >= 0 && nx + x < W) out[size_t(tb.y0 + y) * W + nx + x] *= rl.at(x, y);
    for (int y = 0; y < bh; y++)
        for (int x = 0; x < bw; x++)
            if (nx + lw + gap + x >= 0 && nx + lw + gap + x < W) out[size_t(by + y) * W + nx + lw + gap + x] *= rb.at(x, y);
    I = std::move(out);
}
enum Cap { kCapShoulder, kCapFaceSwap, kCapFaceB, kCapFace };
struct Target { uint64_t hash; int w, h, action; Cap cap; Box box; const char* name; };
// boxes: the caption's glyphs, 1 texel around them (2 right and below, past the letters' embossed edge)
const Target kTargets[] = {
    {0x1720bfa2969bbe1cull, 83, 83, input_map::kL, kCapShoulder, {33, 33, 46, 53}, "L (HUD)"},
    {0xfeda96baaf674588ull, 83, 83, input_map::kR, kCapShoulder, {33, 32, 48, 53}, "R (HUD)"},
    {0x48c5b1ce700a5612ull, 140, 140, input_map::kL, kCapShoulder, {63, 61, 83, 93}, "L 140"},
    {0xb14837dc42d1d5c0ull, 140, 140, input_map::kR, kCapShoulder, {55, 61, 80, 93}, "R 140"},
    {0x9393656bda033db7ull, 84, 84, input_map::kZL, kCapShoulder, {28, 30, 60, 54}, "ZL 84"},
    {0xbad81080414fdd79ull, 84, 84, input_map::kZR, kCapShoulder, {28, 30, 61, 54}, "ZR 84"},
    {0x758509eaa317a053ull, 140, 140, input_map::kZL, kCapShoulder, {46, 60, 95, 92}, "ZL 140"},
    {0x635fd7a9332acc39ull, 140, 140, input_map::kZR, kCapShoulder, {46, 60, 98, 92}, "ZR 140"},
    {0x8ed2f1635f850cb3ull, 125, 58, input_map::kR, kCapShoulder, {51, 14, 74, 44}, "R (items)"},
    {0x1c52e2806ca3da3eull, 34, 32, input_map::kZL, kCapShoulder, {10, 9, 25, 21}, "ZL (items)"},
    {0x2cbb3e98708a7327ull, 41, 44, input_map::kX, kCapFaceSwap, {12, 10, 30, 29}, "X (items)"},
    {0x97a310f9206c2308ull, 41, 44, input_map::kY, kCapFaceSwap, {12, 11, 30, 30}, "Y (items)"},
    {0x30a8f069ef83563cull, 26, 28, input_map::kA, kCapFaceB, {7, 6, 20, 19}, "A (items)"},
    // the face buttons (88x88, light and dark): only for PlayStation (Xbox trades them between the HUD's panes)
    {0x8ff37d6ee3e858afull, 88, 88, input_map::kB, kCapFace, {31, 29, 60, 61}, "B (dark)"},
    {0x315ddcce3c63c38full, 88, 88, input_map::kB, kCapFace, {31, 23, 60, 55}, "B"},
    {0xd0e08276c016e3eaull, 88, 88, input_map::kA, kCapFace, {27, 28, 61, 62}, "A (dark)"},
    {0x4ae173bc8c40c8d9ull, 88, 88, input_map::kX, kCapFace, {29, 23, 61, 55}, "X"},
    {0xaff31753179fb93cull, 88, 88, input_map::kX, kCapFace, {29, 29, 61, 61}, "X (dark)"},
    {0x49fe12c3e2182d9full, 88, 88, input_map::kY, kCapFace, {28, 24, 61, 56}, "Y"},
    {0x150ea81c5a3b72e6ull, 88, 88, input_map::kY, kCapFace, {28, 30, 61, 62}, "Y (dark)"},
};
// PlayStation: a shape or a digit drawn as a letter of the game would be: strokes of `stroke` (fraction of the glyph's
// height) with the letters' depth `core` (their darkest shading), anti-aliased (4x4), in a w x h map whose glyph fills
// the rectangle gx0..gx0+gw, gy0..gy0+gh (unit glyph coordinates as strokes(): x and y in [-0.5, 0.5])
Map stroke_map(Glyph g, int w, int h, float gx0, float gy0, float gw, float gh, float stroke, float core) {
    Map m{w, h, std::vector<float>(size_t(w) * h, 1.0f)};
    std::vector<std::vector<P>> st;
    for (const auto& poly : strokes(g)) {
        std::vector<P> sc;
        for (const P& q : poly) sc.push_back({gx0 + (q.x + 0.5) * gw, gy0 + (q.y + 0.5) * gh});
        st.push_back(std::move(sc));
    }
    const double half = stroke * gh / 2;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int hit = 0;
            for (int j = 0; j < 4; j++)
                for (int i = 0; i < 4; i++) {
                    const double px = x + (i + 0.5) / 4, py = y + (j + 0.5) / 4;
                    for (const auto& poly : st)
                        if (poly_dist(px, py, poly) <= half) { hit++; break; }
                }
            m.v[size_t(y) * w + x] = 1.0f - (hit / 16.0f) * (1.0f - core);
        }
    return m;
}
float darkest(const Map& m) {
    float lo = 1;
    for (float v : m.v) lo = std::min(lo, v);
    return lo;
}
Glyph ps_shape(int pad) {  // by position: south Cross, east Circle, west Square, north Triangle
    switch (pad) {
    case input_map::kPadA: return kCross;
    case input_map::kPadB: return kCircle;
    case input_map::kPadX: return kSquare;
    case input_map::kPadY: return kTriangle;
    default: return kNone;
    }
}
// the caption in a box replaced by a shape filling it (square, centred); I is one channel
void write_shape(std::vector<float>& I, int W, const Box& b, Glyph g, float core) {
    const std::vector<float> bd = body(I, W, b);
    const int bw = b.x1 - b.x0, bh = b.y1 - b.y0;
    for (int y = 0; y < bh; y++)
        for (int x = 0; x < bw; x++) I[size_t(b.y0 + y) * W + b.x0 + x] = bd[size_t(y) * bw + x];
    const int side = std::min(bw, bh) + 2, ox = b.x0 + (bw - side) / 2, oy = b.y0 + (bh - side) / 2;
    const float m = side * 0.10f;
    const Map sh = stroke_map(g, side, side, m, m, side - 2 * m, side - 2 * m, 0.145f, core);
    for (int y = 0; y < side; y++)
        for (int x = 0; x < side; x++)
            if (ox + x >= 0 && ox + x < W && oy + y >= 0 && size_t((oy + y) * W + ox + x) < I.size()) I[size_t(oy + y) * W + ox + x] *= sh.at(x, y);
}
// PlayStation captions: shapes for the face buttons, L1 / R1 / L2 / R2 for the shoulders (the L / R of the game, the
// digit drawn as the game's letters are); channels: 2 (R8G8: intensity) or 4 (RGBA: each colour channel)
bool caption_ps(const Target& t, std::vector<uint8_t>& packed, int bpt, const Map& ml, const Map& mr, const Map& mb) {
    const int W = t.w, pad = input_map::current().pad[t.action];
    const float core = darkest(mb);
    std::vector<std::vector<float>> ch(bpt == 4 ? 3 : 1, std::vector<float>(size_t(W) * t.h));
    for (size_t c = 0; c < ch.size(); c++)
        for (size_t k = 0; k < ch[c].size(); k++) ch[c][k] = packed[bpt * k + c];
    if (t.cap == kCapShoulder) {
        const bool left = pad == input_map::kPadLB || pad == input_map::kPadLT, trig = pad == input_map::kPadLT || pad == input_map::kPadRT;
        if (!left && pad != input_map::kPadRB && pad != input_map::kPadRT) return false;
        const Map& lm = left ? ml : mr;
        // the digit in the letter's own frame: glyph rows 1..h-2 of the letter's map, 0.62 as wide as tall
        // (as tall as the letter's glyph, strokes included: the round caps reach half a stroke past the path)
        const int dh = lm.h, dw = std::max(3, int(std::lround(dh * 0.70f)));
        const float gh = dh - 2.6f, st = 0.14f;
        const Map dg = stroke_map(trig ? kLet2 : kLet1, dw, dh, 0.4f + st * gh / 2, 0.8f + st * gh / 2, dw - 0.8f - st * gh, gh - st * gh, st, darkest(lm));
        for (auto& c : ch) write_word(c, W, t.box, {&lm, &dg});
    } else {
        const Glyph g = ps_shape(pad);
        if (g == kNone) return false;
        for (auto& c : ch) write_shape(c, W, t.box, g, core);
    }
    for (size_t c = 0; c < ch.size(); c++)
        for (size_t k = 0; k < ch[c].size(); k++) packed[bpt * k + c] = (uint8_t)std::clamp(std::lround(ch[c][k]), 0l, 255l);
    return true;
}

// returns true when packed was rewritten

bool caption(const Target& t, std::vector<uint8_t>& packed, int style, int bpt = 2) {
    const int W = t.w, pad = input_map::current().pad[t.action];
    if (style == kXbox && t.cap == kCapFace) return false;  // the HUD's panes trade these (button_text.cpp)
    if (style == kXbox && t.cap == kCapFaceSwap) {
        // Wii U X / Y -> the controller's name: Xbox's by position, top is Y and left is X (the other texture)
        const bool wantOther = (t.action == input_map::kX && pad == input_map::kPadY) || (t.action == input_map::kY && pad == input_map::kPadX);
        const std::vector<uint8_t>& other = t.action == input_map::kX ? g_ring_y : g_ring_x;
        if (!wantOther || other.size() != packed.size()) return false;
        packed = other;
        return true;
    }
    if (g_src_l.empty() || g_src_r.empty() || g_src_b.empty()) return false;
    static Map ml, mr, mb, mt;
    static bool built = false;
    if (!built) {
        built = true;
        ml = letter(g_src_l, 83, {33, 33, 46, 53});
        mr = letter(g_src_r, 83, {33, 32, 48, 53});
        mb = letter(g_src_b, 88, {31, 23, 60, 55});
        mt = make_t(ml);
    }
    if (style == kPlayStation) return caption_ps(t, packed, bpt, ml, mr, mb);
    if (style != kXbox || bpt != 2) return false;
    std::vector<const Map*> word;
    if (t.cap == kCapFaceB) {
        if (pad != input_map::kPadB) return false;
        word = {&mb};
    } else {
        switch (pad) {
        case input_map::kPadLB: word = {&ml, &mb}; break;
        case input_map::kPadRB: word = {&mr, &mb}; break;
        case input_map::kPadLT: word = {&ml, &mt}; break;
        case input_map::kPadRT: word = {&mr, &mt}; break;
        default: return false;
        }
    }
    std::vector<float> I(size_t(W) * t.h);
    for (size_t k = 0; k < I.size(); k++) I[k] = packed[2 * k];
    if (t.cap == kCapShoulder && (pad == input_map::kPadLB || pad == input_map::kPadRB)) {
        const bool l = pad == input_map::kPadLB;
        const Box sb = l ? Box{32, 32, 45, 52} : Box{32, 32, 48, 52};  // the HUD's L / R, 2 texels around
        // the HUD's own L / R buttons get exactly the boxes of v0.8.23; the others their caption's box
        Box tb = t.box;
        if (t.w == 83 && t.h == 83) tb = t.action == input_map::kL ? Box{32, 32, 45, 52} : Box{32, 32, 48, 52};
        write_xb(I, W, tb, l ? g_src_l : g_src_r, sb);
    } else {
        write_word(I, W, t.box, word);
    }
    for (size_t k = 0; k < I.size(); k++) packed[2 * k] = (uint8_t)std::clamp(std::lround(I[k]), 0l, 255l);
    return true;
}
}  // namespace

void maybe_replace(uint32_t w, uint32_t h, uint32_t fmt, const uint8_t* guest, size_t guestSize, std::vector<uint8_t>& packed) {
    if ((fmt & 0x3F) == 0x1A && w == 88 && h == 88 && packed.size() == size_t(88) * 88 * 4) {
        // the light A button (RGBA): PlayStation's shape for its action
        static const Target kA88 = {0x4f3f9c778fddfe55ull, 88, 88, input_map::kA, kCapFace, {27, 23, 61, 55}, "A"};
        if (fnv(guest, guestSize) != kA88.hash || resolved_style() != kPlayStation) return;
        if (caption(kA88, packed, kPlayStation, 4)) {
            static bool told = false;
            if (!told) told = true, LOG("[glyph] caption of A rewritten for the controller");
        }
        return;
    }
    if ((fmt & 0x3F) == 0x7 && w <= 160 && h <= 160) {
        if (packed.size() != size_t(w) * h * 2) return;
        const uint64_t full = fnv(guest, guestSize);
        // sources (as the game has them, before any change)
        auto keep = [&](std::vector<float>& dst) {
            const bool first = dst.empty();
            dst.resize(size_t(w) * h);
            for (size_t k = 0; k < dst.size(); k++) dst[k] = packed[2 * k];
            if (first) g_gen++;  // buttons uploaded before their sources: again
        };
        if (full == kHashB88) keep(g_src_b);  // (and on to the targets: the B is one of them)
        if (full == kHashL83) keep(g_src_l);
        if (full == kHashR83) keep(g_src_r);
        for (const Target& t : kTargets) {
            if (t.hash != full || t.w != int(w) || t.h != int(h)) continue;
            if (t.cap == kCapFaceSwap) {
                std::vector<uint8_t>& mine = t.action == input_map::kX ? g_ring_x : g_ring_y;
                if (mine.empty()) { mine = packed; g_gen++; }
            }
            const int style = resolved_style();
            if (style != kXbox && style != kPlayStation) return;
            if (caption(t, packed, style)) {
                static std::unordered_set<std::string> told;
                if (told.insert(t.name).second) LOG("[glyph] caption of %s rewritten for the controller", t.name);
            }
            return;
        }
        return;
    }
    // the message window's button icons (main2D: font_* 24x24, "Confirmar (A)"): a light disc / shape with a dark
    // letter, R4G4 (host R8G8: intensity, alpha) or RGBA8; the letter is redrawn, the shape and its alpha kept
    if (w == 24 && h == 24 && ((fmt & 0x3F) == 0x2 || (fmt & 0x3F) == 0x1A)) {
        struct Icon { uint64_t hash; int action; };
        static const Icon kIcons[] = {
            {0xe465896a56ea3a8aull, input_map::kA}, {0x63c5153458802c45ull, input_map::kB},
            {0xc72440d4be5ab238ull, input_map::kX}, {0x25f1d8bf685b5035ull, input_map::kY},
            {0x5bfae0412cefd520ull, input_map::kL}, {0x7e2adb387d3256d0ull, input_map::kR},
        };
        const uint64_t hh = fnv(guest, std::min<size_t>(guestSize, 4096));
        const int style = resolved_style();
        if (style != kXbox && style != kPlayStation) return;
        const input_map::Mapping m = input_map::current();
        for (const Icon& ic : kIcons) {
            if (ic.hash != hh) continue;
            std::vector<Glyph> word;
            if (ic.action == input_map::kL || ic.action == input_map::kR) word = shoulder_for(style, m.pad[ic.action]);
            else if (Glyph g = glyph_for(style, m.pad[ic.action]); g != kNone) word = {g};
            if (word.empty()) return;
            const uint32_t bw = word.size() > 1 ? 18 : 12, bh = word.size() > 1 ? 10 : 12;  // the letter's box, centred
            std::vector<uint8_t> cov;
            draw(word, bw, bh, cov);
            const size_t bpt = (fmt & 0x3F) == 0x2 ? 2 : 4;
            if (packed.size() != (size_t)w * h * bpt) return;
            for (uint32_t y = 0; y < h; y++)
                for (uint32_t x = 0; x < w; x++) {
                    uint8_t* px = &packed[((size_t)y * w + x) * bpt];
                    const int a = bpt == 2 ? px[1] : px[3];
                    if (!a) continue;
                    int c = 0;
                    const int lx = int(x) - int(w - bw) / 2, ly = int(y) - int(h - bh) / 2;
                    if (lx >= 0 && ly >= 0 && lx < (int)bw && ly < (int)bh) c = cov[(size_t)ly * bw + lx];
                    const uint8_t v = (uint8_t)(255 - c);
                    if (bpt == 2) px[0] = v;
                    else px[0] = px[1] = px[2] = v;
                }
            LOG("[glyph] message icon %d redrawn", ic.action);
            return;
        }
        return;
    }
    static int seen = 0;
    static const bool log_seen = getenv("WWHD_LOG_GLYPHS") != nullptr;
    if (log_seen && w <= 64 && h <= 64 && seen < 120 && (fmt & 0x3F) == 0x1) {  // diagnostics: what the game uploads
        ++seen;
        LOG("[glyph] seen format %X %ux%u packed %zu hash %016llx", fmt, w, h, packed.size(),
            (unsigned long long)fnv(guest, std::min<size_t>(guestSize, 4096)));
    }
    if ((fmt & 0x3F) != 0x1 || w > 64 || h > 64 || packed.size() != (size_t)w * h) return;
    static const bool log_tex = getenv("WWHD_LOG_TEX") != nullptr;
    static const bool dump_tex = getenv("WWHD_DUMP_TEX") != nullptr;  // diagnostics: small masks as PGM files in tphd_tex/
    uint64_t hash = fnv(guest, std::min<size_t>(guestSize, 4096));
    if (log_tex) LOG("[glyph] small texture %ux%u hash %016llx", w, h, (unsigned long long)hash);
    if (dump_tex) {  // PGM files of small masks (tphd_tex/): to see what the game draws
        static int dumped = 0;
        char path[128];
        snprintf(path, sizeof path, "tphd_tex/%016llx_%ux%u.pgm", (unsigned long long)hash, w, h);
        std::error_code ec;
        std::filesystem::create_directories("tphd_tex", ec);
        if (dumped < 150 && !std::filesystem::exists(path, ec) && ++dumped) if (FILE* f = fopen(path, "wb")) { fprintf(f, "P5\n%u %u\n255\n", w, h); fwrite(packed.data(), 1, packed.size(), f); fclose(f); }
    }
    const int style = resolved_style();
    if (style != kXbox && style != kPlayStation) return;
    const input_map::Mapping m = input_map::current();
    for (const Known& k : kKnown)
        if (k.hash == hash && k.w == w && k.h == h) {
            if (k.action == input_map::kR || k.action == input_map::kL) {
                const std::vector<Glyph> word = shoulder_for(style, m.pad[k.action]);
                if (word.empty()) return;  // not on a shoulder button: the game's own letter
                draw(word, w, h, packed);
                LOG("[glyph] shoulder %s shows its %s name", k.name, style == kPlayStation ? "PlayStation" : "Xbox");
                return;
            }
            if (style == kXbox) return;  // Xbox letters are swapped between the HUD's panes (button_text.cpp)
            Glyph g = glyph_for(style, m.pad[k.action]);
            if (g == kNone) return;  // not bound to a face button: the game's own icon
            draw(g, w, h, packed);
            LOG("[glyph] button %s shows %s", k.name, style == kPlayStation ? "a PlayStation shape" : "the Xbox name");
            return;
        }
}

uint32_t generation() {
    // style (Auto: the connected controller's), controls and option changes
    return g_gen.load() * 1000003u + (uint32_t)resolved_style() * 7919u + input_map::generation();
}
}  // namespace btnglyph
