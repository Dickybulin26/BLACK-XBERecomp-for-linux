// Gyro aiming: the controller's gyro turns the view like a mouse, beside the right stick.
// Yaw follows the turn about the real world's up, so the controller can be held at any tilt.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <mutex>

#include <SDL3/SDL.h>

#include "common/menu.h"
#include "common/mouse_keyboard.h"
#include "common/options.h"
#include "core/log.h"

namespace input {

namespace {

constexpr float kDegreesPerRadian = 57.2957795f;
constexpr float kSecondsPerNs = 1e-9f;
constexpr float kMaxSampleGap = 0.05f;  // seconds; a longer gap between readings is a stall, not motion
constexpr uint64_t kStaleNs = 100'000'000;  // turn gathered while the look update did not run is dropped
constexpr uint64_t kScanNs = 1'000'000'000;  // controllers plugged in since get their sensors on
constexpr float kGravitySeconds = 0.25f;  // the accelerometer's smoothing into the controller's up
// a controller at rest stays within these through a whole window; the gyro's mean there is its drift
constexpr float kRestSeconds = 1.0f;
constexpr float kRestGyroSpread = 1.5f;  // degrees a second
constexpr float kRestAccelSpread = 0.2f;  // metres a second squared
constexpr float kMaxDrift = 8.0f;  // degrees a second; a steady turn faster than this is no drift

// SDL's axes: x right, y up out of the face, z toward the player
struct Vec3 {
    float x = 0;
    float y = 0;
    float z = 0;
};

Vec3 Add(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 Sub(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 Scale(const Vec3& a, float s) { return {a.x * s, a.y * s, a.z * s}; }
float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
float Largest(const Vec3& a) { return std::max({std::fabs(a.x), std::fabs(a.y), std::fabs(a.z)}); }

// the lowest and highest reading on each axis
struct Spread {
    Vec3 low;
    Vec3 high;
    bool empty = true;

    void Add(const Vec3& v) {
        if (empty) {
            low = v;
            high = v;
            empty = false;
            return;
        }
        low = {std::min(low.x, v.x), std::min(low.y, v.y), std::min(low.z, v.z)};
        high = {std::max(high.x, v.x), std::max(high.y, v.y), std::max(high.z, v.z)};
    }
    float Widest() const { return empty ? 0.0f : Largest(Sub(high, low)); }
};

// the window the controller is tested for rest through
struct RestWindow {
    Spread gyro;
    Spread accel;
    Vec3 gyro_sum;
    int gyro_count = 0;
    float seconds = 0;
};

// the controller aiming: the first whose gyro reports, until it is unplugged
struct Motion {
    SDL_JoystickID pad = 0;
    uint64_t gyro_ns = 0;  // sensor clock of the last readings
    uint64_t accel_ns = 0;
    Vec3 gravity;  // smoothed accelerometer, pointing up
    Vec3 drift;  // degrees a second the gyro reads at rest
    RestWindow rest;
    float yaw = 0;  // degrees turned since the last take, left positive
    float pitch = 0;  // down positive
};

std::mutex g_lock;
Motion g_motion;

// seconds since the last reading, 0 for the first and for a stall
float Gap(uint64_t& last, uint64_t now) {
    int64_t ns = last ? int64_t(now - last) : 0;
    last = now;
    float seconds = float(ns) * kSecondsPerNs;
    return seconds > 0 && seconds <= kMaxSampleGap ? seconds : 0.0f;
}

Vec3 Up(const Motion& m) {
    float length = std::sqrt(Dot(m.gravity, m.gravity));
    if (length < 1.0f) return {0, 1, 0};  // no accelerometer yet
    return Scale(m.gravity, 1.0f / length);
}

void ReadAccel(Motion& m, const Vec3& accel, uint64_t ns) {
    bool first = !m.accel_ns;
    float dt = Gap(m.accel_ns, ns);
    float k = first ? 1.0f : dt / (kGravitySeconds + dt);
    m.gravity = Add(m.gravity, Scale(Sub(accel, m.gravity), k));
    m.rest.accel.Add(accel);
}

// a window the controller lay still through sets the drift to the gyro's mean over it
void UpdateDrift(Motion& m, const Vec3& gyro, float dt) {
    RestWindow& w = m.rest;
    w.gyro.Add(gyro);
    w.gyro_sum = Add(w.gyro_sum, gyro);
    w.gyro_count++;
    w.seconds += dt;
    if (w.seconds < kRestSeconds) return;
    Vec3 mean = Scale(w.gyro_sum, 1.0f / float(w.gyro_count));
    bool still = w.gyro.Widest() < kRestGyroSpread && w.accel.Widest() < kRestAccelSpread;
    if (still && Largest(mean) < kMaxDrift) m.drift = mean;
    w = {};
}

void ReadGyro(Motion& m, const Vec3& radians, uint64_t ns) {
    float dt = Gap(m.gyro_ns, ns);
    Vec3 gyro = Scale(radians, kDegreesPerRadian);
    UpdateDrift(m, gyro, dt);
    Vec3 turn = Sub(gyro, m.drift);
    m.yaw += Dot(turn, Up(m)) * dt;
    m.pitch -= turn.x * dt;  // turning about x raises the controller's front
}

// SDL's thread that reads the controllers, every reading as it arrives
bool SDLCALL OnEvent(void*, SDL_Event* e) {
    std::lock_guard lock(g_lock);
    if (e->type == SDL_EVENT_GAMEPAD_REMOVED && e->gdevice.which == g_motion.pad) g_motion = {};
    if (e->type != SDL_EVENT_GAMEPAD_SENSOR_UPDATE) return true;
    const SDL_GamepadSensorEvent& s = e->gsensor;
    if (!g_motion.pad && s.sensor == SDL_SENSOR_GYRO) g_motion.pad = s.which;
    if (s.which != g_motion.pad) return true;
    Vec3 data{s.data[0], s.data[1], s.data[2]};
    uint64_t ns = s.sensor_timestamp ? s.sensor_timestamp : s.timestamp;
    if (s.sensor == SDL_SENSOR_GYRO) ReadGyro(g_motion, data, ns);
    if (s.sensor == SDL_SENSOR_ACCEL) ReadAccel(g_motion, data, ns);
    return true;
}

void EnableSensors() {
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    if (!ids) return;
    for (int i = 0; i < count; i++) {
        SDL_Gamepad* pad = SDL_GetGamepadFromID(ids[i]);
        if (!pad || !SDL_GamepadHasSensor(pad, SDL_SENSOR_GYRO)) continue;
        if (SDL_GamepadSensorEnabled(pad, SDL_SENSOR_GYRO)) continue;
        SDL_SetGamepadSensorEnabled(pad, SDL_SENSOR_GYRO, true);
        SDL_SetGamepadSensorEnabled(pad, SDL_SENSOR_ACCEL, true);
        LOGI("gyro: %s", SDL_GetGamepadName(pad));
    }
    SDL_free(ids);
}

// once the controllers are up: the event watch, then the sensors of new controllers each scan
void EnsureSensors() {
    static bool watching = false;
    static uint64_t scanned = 0;
    if (!SDL_WasInit(SDL_INIT_GAMEPAD)) return;
    if (!watching) watching = SDL_AddEventWatch(OnEvent, nullptr);
    uint64_t now = SDL_GetTicksNS();
    if (scanned && now - scanned < kScanNs) return;
    scanned = now;
    EnableSensors();
}

}  // namespace

bool TakeGyro(float& yaw, float& pitch) {
    static uint64_t last_take = 0;
    const game::Options& o = game::Opt();
    if (!o.gyro_aiming) return false;
    EnsureSensors();
    uint64_t now = SDL_GetTicksNS();
    bool stale = now - last_take > kStaleNs;
    last_take = now;
    std::lock_guard lock(g_lock);
    yaw = g_motion.yaw * o.gyro_sensitivity;
    pitch = g_motion.pitch * o.gyro_sensitivity;
    g_motion.yaw = 0;
    g_motion.pitch = 0;
    return !stale && !app::MenuOpen() && (yaw != 0 || pitch != 0);
}

}  // namespace input
