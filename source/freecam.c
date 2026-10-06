// Voodoo Vince Remastered (Steam 1.14.2.0) - cutscene free camera.
// Ships as a proxy XINPUT9_1_0.dll placed next to Vince.exe.
//
// How it works: the engine still contains the developers' "DEBUGCAM" camera
// object (class DebugCam), created on every level load but never activated.
// We hook the per-frame camera selection (0x50BCCB) and the main scene render
// call (0x50C129); while freecam is on we drive DEBUGCAM's transform ourselves
// and make it the render camera, overriding cutscene/game cameras.

typedef unsigned int u32; typedef unsigned short u16; typedef unsigned char u8;
typedef int BOOL; typedef void *HANDLE; typedef void *HMODULE;
#define WINAPI __stdcall

/* ---- kernel32 / user32 ---- */
__declspec(dllimport) HMODULE WINAPI LoadLibraryA(const char *);
__declspec(dllimport) void *WINAPI GetProcAddress(HMODULE, const char *);
__declspec(dllimport) u32 WINAPI GetSystemDirectoryA(char *, u32);
__declspec(dllimport) BOOL WINAPI VirtualProtect(void *, u32, u32, u32 *);
__declspec(dllimport) BOOL WINAPI FlushInstructionCache(HANDLE, const void *, u32);
__declspec(dllimport) HANDLE WINAPI GetCurrentProcess(void);
__declspec(dllimport) u32 WINAPI GetCurrentProcessId(void);
__declspec(dllimport) BOOL WINAPI QueryPerformanceCounter(long long *);
__declspec(dllimport) BOOL WINAPI QueryPerformanceFrequency(long long *);
__declspec(dllimport) u32 WINAPI GetModuleFileNameA(HMODULE, char *, u32);
__declspec(dllimport) u32 WINAPI GetPrivateProfileStringA(const char *, const char *, const char *, char *, u32, const char *);
__declspec(dllimport) HANDLE WINAPI CreateFileA(const char *, u32, u32, void *, u32, u32, HANDLE);
__declspec(dllimport) BOOL WINAPI WriteFile(HANDLE, const void *, u32, u32 *, void *);
__declspec(dllimport) BOOL WINAPI CloseHandle(HANDLE);
__declspec(dllimport) BOOL WINAPI DisableThreadLibraryCalls(HMODULE);
__declspec(dllimport) HMODULE WINAPI GetModuleHandleA(const char *);
__declspec(dllimport) short WINAPI GetAsyncKeyState(int);
__declspec(dllimport) void *WINAPI GetForegroundWindow(void);
__declspec(dllimport) u32 WINAPI GetWindowThreadProcessId(void *, u32 *);
typedef struct { int x, y; } POINT;
__declspec(dllimport) BOOL WINAPI GetCursorPos(POINT *);
__declspec(dllimport) BOOL WINAPI SetCursorPos(int, int);

int _fltused = 1;
void *memset(void *d, int c, unsigned n) { u8 *p = d; while (n--) *p++ = (u8)c; return d; }
void *memcpy(void *d, const void *s, unsigned n) { u8 *p = d; const u8 *q = s; while (n--) *p++ = *q++; return d; }

/* ---- x87 math helpers (no CRT) ---- */
static float fsin_(float x) { float r; __asm__("fsin" : "=t"(r) : "0"(x)); return r; }
static float fcos_(float x) { float r; __asm__("fcos" : "=t"(r) : "0"(x)); return r; }
static float fsqrt_(float x) { float r; __asm__("fsqrt" : "=t"(r) : "0"(x)); return r; }
static float fatan2_(float y, float x) { float r; __asm__("fpatan" : "=t"(r) : "0"(x), "u"(y) : "st(1)"); return r; }
static float fasin_(float s) { if (s > 1) s = 1; if (s < -1) s = -1; return fatan2_(s, fsqrt_(1 - s * s)); }
static float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }

/* ---- XInput proxy ---- */
typedef struct { u16 wButtons; u8 bLeftTrigger, bRightTrigger; short sThumbLX, sThumbLY, sThumbRX, sThumbRY; } XINPUT_GAMEPAD;
typedef struct { u32 dwPacketNumber; XINPUT_GAMEPAD Gamepad; } XINPUT_STATE;
typedef u32(WINAPI *XGetState_t)(u32, XINPUT_STATE *);
typedef u32(WINAPI *XSetState_t)(u32, void *);
static XGetState_t realGetState; static XSetState_t realSetState;

static void load_real_xinput(void) {
    if (realGetState) return;
    char path[300]; u32 n = GetSystemDirectoryA(path, 260);
    const char *f = "\\XINPUT9_1_0.dll"; for (int i = 0; f[i]; i++) path[n++] = f[i]; path[n] = 0;
    HMODULE h = LoadLibraryA(path);
    if (h) { realGetState = (XGetState_t)GetProcAddress(h, "XInputGetState"); realSetState = (XSetState_t)GetProcAddress(h, "XInputSetState"); }
}

/* ---- game addresses (Vince.exe 1.14.2.0) ---- */
static u32 D; /* ASLR delta: actual base - 0x400000 */
#define R(va) ((u32)(va) + D)
#define G_GAME      (*(u8 **)R(0xB653F4))   /* +0x40 = active render camera */
#define G_CAMMGR    (*(u8 **)R(0xB631E8))   /* camera manager */
#define G_WORLD     (*(u8 **)R(0xB631EC))   /* +0x1C = Vince */
typedef struct { union { char buf[16]; char *ptr; }; u32 size, cap; } MsvcString;
typedef u8 *(__thiscall *FindCamera_t)(u8 *mgr, MsvcString *name);
typedef float *(__thiscall *GetWorldMatrix_t)(u8 *node, float *out16);
typedef void(__thiscall *MarkDirty_t)(u8 *node);
typedef void(__thiscall *VinceCtl_t)(u8 *vince);
#define FindCamera ((FindCamera_t)R(0x4C58D0))
#define GetWorldMatrix ((GetWorldMatrix_t)R(0x4291A0))
#define MarkDirty ((MarkDirty_t)R(0x429840))
#define VinceDisableControl ((VinceCtl_t)R(0x51CE50))
#define VinceEnableControl ((VinceCtl_t)R(0x51F050))
#define CAM_POS 0x2C
#define CAM_ROT 0x38   /* quaternion x,y,z,w; forward = +Z, up = +Y */
#define CAM_FOV 0xA8
#define CAM_FOV_DIRTY 0xF8
#define VINCE_CONTROL 0x2DD

/* ---- settings (freecam.ini, optional) ---- */
static char g_ini[300];
static int cfg_toggle = 0x74;   /* F5 */
static float cfg_speed = 1.5f, cfg_mouse = 0.0025f, cfg_padlook = 1.8f, cfg_keylook = 1.5f;
static int cfg_invx = 0, cfg_invy = 0, cfg_freeze = 1;

static float parse_float(const char *s, float def) {
    float v = 0, sc = 1; int neg = 0, any = 0;
    while (*s == ' ') s++;
    if (*s == '-') { neg = 1; s++; }
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { /* hex int */
        s += 2; u32 h = 0; while (1) { char c = *s++; int d = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1; if (d < 0) break; h = h * 16 + d; any = 1; }
        return any ? (float)h : def;
    }
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s++ - '0'); any = 1; }
    if (*s == '.') { s++; while (*s >= '0' && *s <= '9') { sc /= 10; v += (*s++ - '0') * sc; any = 1; } }
    return any ? (neg ? -v : v) : def;
}
static float ini_f(const char *k, float def) { char b[64]; GetPrivateProfileStringA("freecam", k, "", b, 64, g_ini); return parse_float(b, def); }

/* ---- log ---- */
static char g_logpath[300];
static void logs(const char *s) {
    HANDLE h = CreateFileA(g_logpath, 4 /*FILE_APPEND_DATA*/, 1, 0, 4 /*OPEN_ALWAYS*/, 0x80, 0);
    if (h == (HANDLE)-1) return; u32 w, n = 0; while (s[n]) n++; WriteFile(h, s, n, &w, 0); WriteFile(h, "\r\n", 2, &w, 0); CloseHandle(h);
}

/* ---- state ---- */
static volatile int g_active;
static u8 *volatile g_dbg;
static u8 *g_lastMgr;
static float g_pos[3], g_yaw, g_pitch, g_fov = 1.47f, g_fovDefault = 1.47f;
static int g_frozeVince;
static long long g_qpf, g_last;
static u8 g_kprev[256];
static u16 g_padPrev;
static POINT g_anchor; static int g_mlook;

static int key(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }
static int focused(void) { u32 pid = 0; void *w = GetForegroundWindow(); if (!w) return 0; GetWindowThreadProcessId(w, &pid); return pid == GetCurrentProcessId(); }

static u8 *find_debugcam(void) {
    u8 *mgr = G_CAMMGR; if (!mgr) return 0;
    MsvcString s; memset(&s, 0, sizeof s); memcpy(s.buf, "DEBUGCAM", 9); s.size = 8; s.cap = 15;
    return FindCamera(mgr, &s);
}
static u8 *get_vince(void) { u8 *w = G_WORLD; return w ? *(u8 **)(w + 0x1C) : 0; }

static void snap_to_camera(u8 *src) {
    float m[16];
    if (!src) return;
    GetWorldMatrix(src, m);
    g_pos[0] = m[12]; g_pos[1] = m[13]; g_pos[2] = m[14];
    float fx = m[8], fy = m[9], fz = m[10], l = fsqrt_(fx * fx + fy * fy + fz * fz);
    if (l > 1e-6f) { fx /= l; fy /= l; fz /= l; g_yaw = fatan2_(fx, fz); g_pitch = fasin_(-fy); }
    float fov = *(float *)(src + CAM_FOV);
    if (fov > 0.05f && fov < 3.0f) g_fov = g_fovDefault = fov;
}

static void apply(u8 *cam) {
    float sy = fsin_(g_yaw * 0.5f), cy = fcos_(g_yaw * 0.5f), sp = fsin_(g_pitch * 0.5f), cp = fcos_(g_pitch * 0.5f);
    float *p = (float *)(cam + CAM_POS), *q = (float *)(cam + CAM_ROT);
    p[0] = g_pos[0]; p[1] = g_pos[1]; p[2] = g_pos[2];
    q[0] = cy * sp; q[1] = sy * cp; q[2] = -sy * sp; q[3] = cy * cp;
    *(float *)(cam + CAM_FOV) = g_fov; *(u8 *)(cam + CAM_FOV_DIRTY) = 0;
    MarkDirty(cam);
}

static void set_active(int on) {
    if (on == g_active) return;
    if (on) {
        u8 *cam = find_debugcam(); u8 *gm = G_GAME;
        if (!cam || !gm) { logs("toggle ignored: DEBUGCAM not found (no level loaded?)"); return; }
        u8 *src = *(u8 **)(gm + 0x40);
        if (src && src != cam) snap_to_camera(src);
        g_dbg = cam; g_active = 1; g_frozeVince = 0;
        u8 *v = get_vince();
        if (cfg_freeze && v && *(v + VINCE_CONTROL)) { VinceDisableControl(v); g_frozeVince = 1; }
        logs("freecam ON");
    } else {
        g_active = 0;
        u8 *v = get_vince();
        if (g_frozeVince && v && !*(v + VINCE_CONTROL)) VinceEnableControl(v);
        g_frozeVince = 0; g_mlook = 0;
        logs("freecam OFF");
    }
}

static float stick(short v) { float f = v / 32767.0f; float dz = 0.2f; if (f > dz) return (f - dz) / (1 - dz); if (f < -dz) return (f + dz) / (1 - dz); return 0; }

/* called on the game thread each frame, right after the game picks its camera */
void __cdecl OnUpdate(void) {
    long long now; QueryPerformanceCounter(&now);
    float dt = g_last ? (float)(now - g_last) / (float)g_qpf : 0; g_last = now;
    dt = clampf(dt, 0, 0.1f);

    u8 *mgr = G_CAMMGR;
    if (mgr != g_lastMgr) { g_lastMgr = mgr; if (g_active) { g_active = 0; g_frozeVince = 0; } g_dbg = 0; }

    int fg = focused();
    u8 kn[256]; for (int i = 0; i < 256; i++) kn[i] = 0;
#define K(vk) (kn[vk] = fg && key(vk))
#define PRESSED(vk) (kn[vk] && !g_kprev[vk])
    int toggles[] = { cfg_toggle, 'R', 'Z', 'X' };
    for (int i = 0; i < 4; i++) K(toggles[i]);

    XINPUT_STATE st; memset(&st, 0, sizeof st); u16 pb = 0;
    load_real_xinput();
    if (realGetState && realGetState(0, &st) == 0) pb = st.Gamepad.wButtons;
    int combo = (pb & 0x00C0) == 0x00C0, comboPrev = (g_padPrev & 0x00C0) == 0x00C0; /* L3+R3 */

    if (PRESSED(cfg_toggle) || (combo && !comboPrev)) set_active(!g_active);

    if (g_active) {
        u8 *cam = find_debugcam();
        if (!cam) { set_active(0); }
        else {
            g_dbg = cam;
            if (PRESSED('R')) { g_fov = g_fovDefault; }
            /* look */
            float lx = 0, ly = 0;
            if (fg) {
                lx += (key(0x27) - key(0x25)) * cfg_keylook * dt;   /* arrows */
                ly += (key(0x28) - key(0x26)) * cfg_keylook * dt;
                if (key(0x02)) { /* right mouse held */
                    POINT c; GetCursorPos(&c);
                    if (!g_mlook) { g_anchor = c; g_mlook = 1; }
                    else { lx += (c.x - g_anchor.x) * cfg_mouse; ly += (c.y - g_anchor.y) * cfg_mouse; SetCursorPos(g_anchor.x, g_anchor.y); }
                } else g_mlook = 0;
            }
            lx += stick(st.Gamepad.sThumbRX) * cfg_padlook * dt;
            ly -= stick(st.Gamepad.sThumbRY) * cfg_padlook * dt;
            if (cfg_invx) lx = -lx; if (cfg_invy) ly = -ly;
            g_yaw += lx; g_pitch = clampf(g_pitch + ly, -1.55f, 1.55f);
            /* move */
            float mx = 0, my = 0, mz = 0, sp = cfg_speed;
            if (fg) {
                mz += key('W') - key('S'); mx += key('D') - key('A'); my += key('E') + key(0x20) - key('Q');
                if (key(0x10)) sp *= 5; if (key(0x11)) sp *= 0.2f;
                if (key('Z')) g_fov -= 0.6f * dt; if (key('X')) g_fov += 0.6f * dt;
            }
            mx += stick(st.Gamepad.sThumbLX); mz += stick(st.Gamepad.sThumbLY);
            my += (st.Gamepad.bRightTrigger - st.Gamepad.bLeftTrigger) / 255.0f;
            if (pb & 0x8000) sp *= 5;      /* Y fast */
            if (pb & 0x4000) sp *= 0.2f;   /* X slow */
            if (pb & 0x0200) g_fov -= 0.6f * dt;  /* RB zoom in */
            if (pb & 0x0100) g_fov += 0.6f * dt;  /* LB zoom out */
            g_fov = clampf(g_fov, 0.1f, 2.8f);
            float syw = fsin_(g_yaw), cyw = fcos_(g_yaw), spt = fsin_(g_pitch), cpt = fcos_(g_pitch);
            float fwd[3] = { syw * cpt, -spt, cyw * cpt }, rgt[3] = { cyw, 0, -syw };
            for (int i = 0; i < 3; i++) g_pos[i] += (fwd[i] * mz + rgt[i] * mx) * sp * dt;
            g_pos[1] += my * sp * dt;
            apply(cam);
            u8 *gm = G_GAME; if (gm) *(u8 **)(gm + 0x40) = cam;
        }
    }
    for (int i = 0; i < 256; i++) g_kprev[i] = kn[i];
    g_padPrev = pb;
}

/* called right before the main scene render: keep our camera in the render slot */
void __cdecl OnRender(void) {
    if (g_active && g_dbg) { u8 *gm = G_GAME; if (gm) *(u8 **)(gm + 0x40) = g_dbg; }
}

/* asm trampolines */
__asm__(
    ".text\n"
    ".globl _upd_stub\n_upd_stub:\n"
    "  movl _g_pGame, %ecx\n  movl (%ecx), %ecx\n"
    "  movl %eax, 0x40(%ecx)\n"
    "  pushal\n  pushfl\n  cld\n  call _OnUpdate\n  popfl\n  popal\n"
    "  ret\n"
    ".globl _render_stub\n_render_stub:\n"
    "  pushal\n  pushfl\n  cld\n  call _OnRender\n  popfl\n  popal\n"
    "  pushl _g_renderOrig\n  ret\n");
u32 g_pGame, g_renderOrig;
extern void upd_stub(void); extern void render_stub(void);

int memcmp_(const void *a, const void *b, unsigned n);
static int patch(u8 *addr, const u8 *expect, const u8 *with, u32 n) {
    for (u32 i = 0; i < n; i++) if (addr[i] != expect[i]) return 0;
    u32 old; if (!VirtualProtect(addr, n, 0x40, &old)) return 0;
    for (u32 i = 0; i < n; i++) addr[i] = with[i];
    VirtualProtect(addr, n, old, &old); FlushInstructionCache(GetCurrentProcess(), addr, n);
    return 1;
}
static void put_rel(u8 *b, u32 from, void *to) { u32 r = (u32)to - (from + 5); memcpy(b + 1, &r, 4); }

static void install(void) {
    /* sanity: DebugCam vtable + "DEBUGCAM" string must be where we expect */
    if (*(u32 *)R(0x6D7B38) != R(0x4D1F70) || memcmp_((const void *)R(0x6DD948), "DEBUGCAM", 9)) { logs("ERROR: unsupported Vince.exe version, freecam disabled"); return; }
    u8 e1[9] = { 0x8B, 0x0D, 0, 0, 0, 0, 0x89, 0x41, 0x40 }; u32 ga = R(0xB653F4); memcpy(e1 + 2, &ga, 4);
    g_pGame = ga; g_renderOrig = R(0x427CB0);
    u8 w1[9] = { 0xE8, 0, 0, 0, 0, 0x90, 0x90, 0x90, 0x90 }; put_rel(w1, R(0x50BCCB), upd_stub);
    static const u8 e2[5] = { 0xE8, 0x82, 0xBB, 0xF1, 0xFF };
    u8 w2[5] = { 0xE8, 0, 0, 0, 0 }; put_rel(w2, R(0x50C129), render_stub);
    if (!patch((u8 *)R(0x50BCCB), e1, w1, 9)) { logs("ERROR: update hook site mismatch"); return; }
    if (!patch((u8 *)R(0x50C129), e2, w2, 5)) { logs("ERROR: render hook site mismatch"); return; }
    { char m[96] = "hooks installed, Vince.exe base 0x00000000 - press F5 (or L3+R3) to toggle";
      u32 v = R(0x400000); for (int i = 0; i < 8; i++) m[41 - i] = "0123456789ABCDEF"[(v >> (4 * i)) & 15]; logs(m); }
}

int memcmp_(const void *a, const void *b, unsigned n) { const u8 *p = a, *q = b; while (n--) { if (*p != *q) return *p - *q; p++; q++; } return 0; }

BOOL WINAPI DllMain(HMODULE h, u32 reason, void *r) {
    if (reason == 1) {
        DisableThreadLibraryCalls(h);
        D = (u32)GetModuleHandleA(0) - 0x400000;
        char dir[300]; u32 n = GetModuleFileNameA(h, dir, 260);
        while (n && dir[n - 1] != '\\') n--; dir[n] = 0;
        memcpy(g_ini, dir, n); memcpy(g_ini + n, "freecam.ini", 12);
        memcpy(g_logpath, dir, n); memcpy(g_logpath + n, "freecam.log", 12);
        cfg_toggle = (int)ini_f("toggle_key", 0x74);
        cfg_speed = ini_f("move_speed", 1.5f); cfg_mouse = ini_f("mouse_sensitivity", 0.0025f);
        cfg_padlook = ini_f("pad_look_speed", 1.8f); cfg_keylook = ini_f("key_look_speed", 1.5f);
        cfg_invx = (int)ini_f("invert_x", 0); cfg_invy = (int)ini_f("invert_y", 0); cfg_freeze = (int)ini_f("freeze_vince", 1);
        if (cfg_toggle <= 0 || cfg_toggle > 255) cfg_toggle = 0x74;
        QueryPerformanceFrequency(&g_qpf);
        install();
    }
    return 1;
}

/* exported XInput API */
__declspec(dllexport) u32 WINAPI XInputGetState(u32 i, XINPUT_STATE *s) {
    load_real_xinput();
    u32 r = realGetState ? realGetState(i, s) : 1167 /*ERROR_DEVICE_NOT_CONNECTED*/;
    if (r == 0 && i == 0 && g_active) {
        /* hide sticks/triggers/shoulders/face buttons from the game while flying; keep Start/Back */
        s->Gamepad.sThumbLX = s->Gamepad.sThumbLY = s->Gamepad.sThumbRX = s->Gamepad.sThumbRY = 0;
        s->Gamepad.bLeftTrigger = s->Gamepad.bRightTrigger = 0;
        s->Gamepad.wButtons &= 0x0030;
    }
    return r;
}
__declspec(dllexport) u32 WINAPI XInputSetState(u32 i, void *v) { load_real_xinput(); return realSetState ? realSetState(i, v) : 1167; }
