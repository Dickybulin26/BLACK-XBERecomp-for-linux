// Menu animations off: a page's slide in and slide out play through in one tick. The Flash player is stepped a frame
// at a time, as a slow tick steps it (0x1106A1), so the actions and sounds on every frame still run.
#include "common/options.h"
#include "game/flash.h"
#include "game/layout.h"
#include "xdk/hle.h"

using namespace kernel;
using namespace layout;

namespace {

constexpr uint32_t kFlashTick = 0x112520;  // cdecl (ms): steps the player once per frame time passed
constexpr uint32_t kMenuNumber = 0x120840;  // eax level, ecx variable; (ui): its value on that level as an integer
constexpr uint32_t kCharacterFrameMs = 0x24;
constexpr uint32_t kPlayingOut = 2;  // kPageLeaving while Main plays out
constexpr int kMaxSteps = 150;  // past every page's intro, press start's Main stops on frame 124

bool g_intro_done = false;  // the page's intro was played through

// the frame time the player steps by, the movie its tick reads (0x11065F)
uint32_t FrameMs() {
    uint32_t movies = R32(R32(flash::kPlayer) + flash::kPlayerMovies);
    uint32_t first = movies ? R32(R32(movies) + flash::kClipNext) : 0;
    if (flash::Type(first) != flash::kMovie) return 0;
    uint32_t data = R32(first + flash::kClipData);
    return R32(R32(data + flash::kDataCharacter) + kCharacterFrameMs);
}

bool Playing(uint32_t clip) {
    if (flash::Type(clip) != flash::kSprite) return false;
    return R32(R32(clip + flash::kClipData) + flash::kDataFlags) & flash::kDataPlaying;
}

int MenuNumber(X86Context& ctx, uint32_t ui, const char* name) {
    ctx.eax = flash::kMenuLevel;
    ctx.ecx = flash::Text(name);
    return int(CallGuest(ctx, kMenuNumber, {ui}));
}

// the game's own test (0x1245B0): Main on its last frame
bool PlayedOut(X86Context& ctx, uint32_t ui) {
    return MenuNumber(ctx, ui, "Main._currentframe") == MenuNumber(ctx, ui, "Main._totalframes");
}

// Main slides in and stops; its TextAnim fades the buttons in after
bool IntroPlaying(X86Context& ctx) {
    uint32_t main = flash::Member(ctx, flash::Level(flash::kMenuLevel), "Main");
    return Playing(main) || Playing(flash::Member(ctx, main, "TextAnim"));
}

bool Animating(X86Context& ctx, uint32_t ui) {
    uint32_t release = R32(ui + kPageLeaving);
    if (release == kPlayingOut) return !PlayedOut(ctx, ui);
    return release == 0 && !g_intro_done && IntroPlaying(ctx);
}

// the frames stepped
int PlayThrough(X86Context& ctx, uint32_t ui, uint32_t frame_ms) {
    int steps = 0;
    while (steps < kMaxSteps && Animating(ctx, ui)) {
        CallGuest(ctx, kFlashTick, {frame_ms});
        steps++;
    }
    return steps;
}

}  // namespace

// ebp the front end, ecx the ms its Flash player tick is about to step
void MenuAnimations(X86Context& ctx, uint8_t*) {
    uint32_t ui = ctx.ebp;
    bool running = R32(flash::kPlayerReady) == 1 && R32(flash::kPlayer) && R32(ui + kPageReady) == kPageRunning;
    if (!running) g_intro_done = false;
    if (!running || game::Opt().menu_animations) return;
    uint32_t frame_ms = FrameMs();
    if (!frame_ms) return;
    X86Context saved = ctx;
    int steps = PlayThrough(ctx, ui, frame_ms);
    if (R32(ui + kPageLeaving) != 0) g_intro_done = false;
    else if (!IntroPlaying(ctx)) g_intro_done = true;
    ctx = saved;
    // a played out Main stays on its last frame until the next page loads
    if (steps) ctx.ecx = 0;
}
