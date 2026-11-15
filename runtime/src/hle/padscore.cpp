// padscore: Wii Remote / Pro Controller (WPAD, KPAD). Only a Pro Controller on channel 0 exists,
// and only while the keyboard/host controllers are set to act as one (Input menu). Struct layouts
// and constants follow Cemu's padscore.
#include <cstdlib>
#include "../crashrec.h"
#include "../runtime.h"
#include "../input.h"
#include "../motion/motion.h"
#include "../rumble.h"

namespace {
constexpr int32_t kWpadErrNone = 0, kWpadErrNoController = -1;
constexpr int32_t kKpadErrNone = 0, kKpadErrNoController = -2;
constexpr uint8_t kDevURCC = 31, kDevNone = 253, kFormatURCC = 22;

// TPHD Windows: a virtual Pro Controller on channel 0 is always connected (fed by the keyboard and any
// SDL controller), so the game's "Controller Selection" screen accepts the Pro Controller without
// hardware. The GamePad keeps working through VPAD. WWHD_NO_VIRTUAL_PRO=1 restores the old behaviour
// (connected only while the Input menu has the keyboard/controllers act as a Pro Controller).
bool connected(uint32_t chan) {
    static const bool virtualPro = !getenv("WWHD_NO_VIRTUAL_PRO");
    return chan == 0 && (virtualPro || input::pro_controller());
}

// VPAD button bits (input::PadState) -> Pro Controller (URCC) bits
uint32_t pro_buttons(uint32_t v) {
    static const uint32_t map[][2] = {
        {input::kUp, 0x1},     {input::kLeft, 0x2},    {input::kZR, 0x4},     {input::kX, 0x8},
        {input::kA, 0x10},     {input::kY, 0x20},      {input::kB, 0x40},     {input::kZL, 0x80},
        {input::kR, 0x200},    {input::kPlus, 0x400},  {input::kHome, 0x800}, {input::kMinus, 0x1000},
        {input::kL, 0x2000},   {input::kDown, 0x4000}, {input::kRight, 0x8000},
        {input::kStickR, 0x10000}, {input::kStickL, 0x20000},
    };
    uint32_t out = 0;
    for (auto& m : map)
        if (v & m[0]) out |= m[1];
    return out;
}
}  // namespace

HLE(padscore, KPADInit) { LOG("[pad] KPADInit(%08X, %08X)", arg(c, 0), arg(c, 1)); }
HLE(padscore, KPADInitEx) { LOG("[pad] KPADInitEx(%08X, %08X)", arg(c, 0), arg(c, 1)); }
HLE(padscore, KPADShutdown) { LOG("[pad] KPADShutdown(%08X, %08X)", arg(c, 0), arg(c, 1)); }
HLE(padscore, KPADEnableDPD) {}
HLE(padscore, KPADDisableDPD) {}
HLE(padscore, KPADEnableMpls) {}
HLE(padscore, KPADDisableMpls) {}
// -> the previous callback; connections are never reported (the Pro Controller is there or not)
// The game learns that the Pro Controller arrived from this callback (its title's "Please connect a Wii U Pro
// Controller" waits for callback(chan, 0), which the real library calls from an alarm). As Cemu: at once
// callback(chan, -1) (no controller), later callback(chan, 0) when it is there. Fired from padscore_tick.
static uint32_t g_connect_cb[4];
static int g_connect_state[4], g_connect_wait[4];
HLE(padscore, KPADSetConnectCallback) {
    uint32_t chan = arg(c, 0), cb = arg(c, 1);
    LOG("[pad] KPADSetConnectCallback(%u, %08X)", chan, cb);
    uint32_t old = 0;
    if (chan < 4) { old = g_connect_cb[chan]; g_connect_cb[chan] = cb; g_connect_state[chan] = 0; g_connect_wait[chan] = 0; }
    ret(c, old);
}
HLE(padscore, KPADGetMplsWorkSize) { ret(c, 0x5FE0); }
HLE(padscore, KPADSetMplsWorkarea) {}
HLE(padscore, WPADEnableURCC) { LOG("[pad] WPADEnableURCC(%08X, %08X)", arg(c, 0), arg(c, 1)); }
HLE(padscore, WPADEnableWiiRemote) { LOG("[pad] WPADEnableWiiRemote(%08X, %08X)", arg(c, 0), arg(c, 1)); }
HLE(padscore, WPADDisconnect) { LOG("[pad] WPADDisconnect(%08X, %08X)", arg(c, 0), arg(c, 1)); }
// The Pro Controller motor: cmd 0 stops it, 1 runs it until the game stops it. The Pro
// Controller is only connected while the keyboard/controllers act as one (see connected).
HLE(padscore, WPADControlMotor) {
    uint32_t chan = arg(c, 0), cmd = arg(c, 1);
    if (connected(chan)) {
        TRACE("[pad] WPADControlMotor(%u, %u) -> %s", chan, cmd, cmd ? "rumble" : "stop");
        rumble::pro_motor(chan, cmd != 0);
    }
    ret(c, (uint32_t)kWpadErrNone);
}
HLE(padscore, WPADGetBatteryLevel) { ret(c, 4); }  // full
HLE(padscore, WPADCanSendStreamData) { ret(c, 0); }
HLE(padscore, WPADSendStreamData) { ret(c, (uint32_t)kWpadErrNoController); }
HLE(padscore, WPADControlSpeaker) { ret(c, (uint32_t)kWpadErrNoController); }

HLE(padscore, WPADProbe) {
    // (chan, uint32* type) -> WPAD error
    uint32_t chan = arg(c, 0), type = arg(c, 1);
    bool on = connected(chan);
    TRACE("[pad] WPADProbe(%u) -> %s", chan, on ? "pro" : "none");
    {   // always-on, low volume: the first probes and every change (tphd.log)
        static int n = 0; static int lastOn[4] = {-1, -1, -1, -1};
        if (chan < 4 && (n < 8 || lastOn[chan] != (int)on)) { LOG("[pad] WPADProbe(%u) -> %s", chan, on ? "pro" : "none"); n++; }
        if (chan < 4) lastOn[chan] = on;
    }
    if (type) st32(type, on ? kDevURCC : kDevNone);
    ret(c, (uint32_t)(on ? kWpadErrNone : kWpadErrNoController));
}

HLE(padscore, KPADReadEx) {
    // (chan, KPADStatus* buf, count, int32* err) -> samples written
    uint32_t chan = arg(c, 0), st = arg(c, 1), count = arg(c, 2), err = arg(c, 3);
    TRACE("[pad] KPADReadEx(%u, %08X, %u) connected=%d", chan, st, count, connected(chan));
    if (!connected(chan) || !st || (int32_t)count <= 0) {
        if (err) st32(err, (uint32_t)(connected(chan) ? kKpadErrNone : kKpadErrNoController));
        ret(c, 0);
        return;
    }
    static uint32_t last = 0;
    input::PadState p = crashrec::read(1);  // crash recovery records/replays it
    motion::right_stick(p.rx, p.ry);  // the Pro Controller's stick decides gyro use too (motion.h)
    uint32_t hold = pro_buttons(p.buttons);
    memset(mem::ptr(st), 0, 0xF0);
    st8(st + 0x5C, kDevURCC);     // devType
    st8(st + 0x5D, 0);            // wpadErr
    st8(st + 0x5F, kFormatURCC);  // data_format
    st32(st + 0x60, hold);                // ex_status.uc.hold
    st32(st + 0x64, hold & ~last);        // trig
    st32(st + 0x68, last & ~hold);        // release
    {   // always-on, low volume: first reads and every change of the buttons (tphd.log)
        static int n = 0;
        if (n < 3 || hold != last) { LOG("[pad] KPADReadEx(%u) hold=%05X lstick=%.2f,%.2f", chan, hold, p.lx, p.ly); n++; }
    }
    last = hold;
    stf32(st + 0x6C, p.lx); stf32(st + 0x70, p.ly);  // lstick
    stf32(st + 0x74, p.rx); stf32(st + 0x78, p.ry);  // rstick
    st32(st + 0x7C, 1);                   // charge
    st32(st + 0x80, 1);                   // cable
    if (err) st32(err, kKpadErrNone);
    ret(c, 1);
}

// Game thread, once per frame (VPADRead): reports connections to the callbacks the game registered.
void padscore_tick(Cpu* c) {
    for (uint32_t chan = 0; chan < 4; chan++) {
        uint32_t cb = g_connect_cb[chan];
        if (!cb) continue;
        int& st = g_connect_state[chan];
        if (st == 0) {
            st = 1; g_connect_wait[chan] = 0;
            LOG("[pad] connect callback(%u, -1)", chan);
            Cpu saved = *c; guest_call(c, cb, {chan, (uint32_t)-1}); *c = saved;
        } else if (st == 1 && ++g_connect_wait[chan] > 20) {
            if (!connected(chan)) continue;
            st = 2;
            LOG("[pad] connect callback(%u, 0): Pro Controller connected", chan);
            Cpu saved = *c; guest_call(c, cb, {chan, 0u}); *c = saved;
        }
    }
}
