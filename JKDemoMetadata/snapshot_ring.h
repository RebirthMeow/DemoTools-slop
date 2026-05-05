#pragma once
#include <deque>
#include "qcommon/q_shared.h"    // already in OpenJK headers

struct SnapLite {
    int   serverTime;    // ms
    vec3_t viewangles;   // yaw, pitch, roll
};

// one ring buffer per client
constexpr int     SNAP_KEEP = 12;
extern std::deque<SnapLite> g_snapHist[MAX_CLIENTS];
