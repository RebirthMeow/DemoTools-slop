AI generated readme below


## Per-frag feature categories

**Mid-air & target state** *(added v11)* — `target_airborne`, `victim_airtime_ms`, `target_z_phase` (`ascending`/`apex`/`falling`/`ground`), `vertical_delta`, `target_crossing_dot`, `target_lead_distance`, `target_dir_changes_during_flight`, `target_air_cause` (`jumppad`/`explosion`/`jump`/`fall`/`teleport`/`unknown`).

**View-flick window** *(added v12)* — `view_flick_window_deg`, `view_flick_window_ms`, `view_flick_window_speed_deg_per_sec`, `view_flick_align_ms`. A windowed search around the kill (and the projectile-fire moment for missile weapons) for the largest sustained angular swing.

**Corpse trajectory** *(added v13)* — `target_corpse_path_distance`, `target_corpse_peak_z_delta`, `target_corpse_min_z_delta`, `target_corpse_z_delta_signed`, plus legacy `target_corpse_travel_distance` / `target_corpse_travel_z_distance`.

**Match state** — `target_had_flag`, `attacker_team`, `target_team`.

## What v13 changes & why it matters

The v13 work cleans up several subtle correctness bugs and adds three feature groups that downstream classifiers benefit from. Each piece is small in isolation; together they're the difference between "noisy stats" and "stats you can train on."

### Bug fixes (output-changing)

- **`target_had_flag` attribution.** On timing edges where flag pickup and kill happened in the same snapshot, the field was occasionally crediting the wrong player. Now resolved against snapshot ordering rather than the post-snapshot world state. Important for any consumer keying off flag-carrier kills.
- **Embedded player events (jump / pain / fall).** Events nested inside snapshot deltas weren't being processed, so jump-pad jumps and falling deaths were silently missed. Without these, the mid-air `target_air_cause` classifier can't distinguish a deliberate jump from a passive walk-off-a-ledge.
- **Corpse stat accumulation.** `peak_z` was tracking signed displacement (so a body that went up then dropped past its start-Z recorded the *down*-phase as the peak), and `path_distance` was including teleport hops as travel. Both fixed; legacy fields kept for compatibility.
- **`playerHistory` walk-back.** The circular buffer used uninitialized slots when walking back across the recording boundary, producing nonsense "movement in the last second" values for kills in the first second of a demo.
- **`kills_in_5s_window`.** The old `multi_kills` boolean conflated doubles and 6-kill multis. The new integer count gives downstream consumers a richer signal — they can recover the old boolean trivially.

### Mid-air detection (v11)

Mid-air state classification mirrors UberDemoTools' Q3 air-frag finder approach: a per-client `playerTrack[]` ring buffer with sub-snap interpolation via `BG_EvaluateTrajectory` (the same evaluation path the engine uses internally), then a classifier at impact time.

`target_air_cause` is the part that earns its keep. Distinguishing "the target *chose* to be airborne" (jumped, used a jump-pad) from "the target was *blasted* airborne" (took splash, fell off something) separates jump-pad rocket airs from chase rockets. Highlight classifiers for rocket frags care a lot about that difference.

### View-flick window (v12)

The view-angle history was already collected pre-v12, but kills were only sampling the angle-delta at one instant. The flick window searches a configurable lookback (and, for missile weapons, around the projectile-fire moment rather than the kill moment) for the largest sustained angular swing. The output gives both magnitude (`_deg`), duration (`_ms`), normalized speed (`_deg_per_sec`), and how cleanly the swing aligned with the actual impact (`_align_ms`).

This is the feature that helps separate clean bryar / disruptor flick snipes from "the crosshair happened to be there" hits.

### Corpse trajectory (v13)

The legacy `corpse_travel_distance` was straight-line displacement — start point to end point. A body that flew up and fell back to roughly its starting position read as "didn't go anywhere," which threw away the discriminating signal for explosive kills.

The new `corpse_path_distance` is path-integrated: sum of frame-to-frame segment lengths over the corpse's tracked trajectory. Combined with `peak_z_delta` (max height *above* kill point) and `min_z_delta` (max *drop* below), the corpse-shape signal recovers the difference between "blasted up off a ledge" (high peak Z, short path) and "knocked off the edge" (negative min Z, longer path).

### Infrastructure

- View-angle history collection moved from `svc_serverCommand` to `svc_snapshot` — one sample per snap (~50ms) instead of "whenever a server command happens to fire," which produced ragged sample timing on busy servers.
- Per-client `playerTrack[]` struct + `BG_EvaluateTrajectory` for sub-snap precision throughout.
- Root `version` field for schema forward compatibility — older consumers can detect and degrade gracefully.

If you've been training against pre-v13 output, **re-extract**: the bug fixes change values for `target_had_flag` and the corpse fields, so cross-version model comparisons aren't apples-to-apples.

### What actually helped prediction

Verdict after running the v13 features through a per-weapon stacking ensemble (XGBoost / LightGBM / extra trees / gradient boosting). Real lift, no benchmark theater:

**Big wins:**
- **The corpse-stat fix.** Legacy `corpse_travel_distance` was almost noise — round-trip trajectories canceled out. `corpse_path_distance` jumped from "weakly informative" to top-5 feature importance for rocket kills. If you only port one v13 change, port that one.
- **`target_air_cause`.** The categorical that splits "target jumped / used a pad" from "target was blasted airborne" is the single most useful new field for mid-air rocket classification. Boolean `target_airborne` alone wasn't enough — every jumper reads as airborne.
- **`peak_z_delta` split from `min_z_delta`.** Recovers a discriminating axis the older signed field smeared together. "Blasted up off a ledge" and "knocked off the edge falling" stop looking the same.

**Modest:**
- **`view_flick_window_speed_deg_per_sec`** for bryar pistol and disruptor — real lift on flick-heavy weapons. Doesn't help rocket much; rocket aim is slower-paced and the relevant signal is captured by `view_flick_align_ms` (how cleanly the swing aligns with the projectile-fire moment) rather than peak swing speed.
- **`victim_airtime_ms`.** Helps the model weigh "barely off the ground for 80ms" against "two-second jumppad hangtime." Useful but redundant with `target_z_phase` past a threshold.

**Whiffs:**
- **`target_dir_changes_during_flight`.** Too noisy in tournament-scale matches with 12 players visible — direction-change counts spike from clutter, not from the target dodging.
- **`target_lead_distance`.** Strongly correlated with `target_xy_speed`, which is already in the feature set. Drop one, lose nothing.
- **Legacy `target_corpse_travel_distance`.** Subsumed by `corpse_path_distance`. Kept for backwards compatibility, but if you're starting fresh, ignore it.

**Caveat worth being upfront about:** the biggest ML lifts on this dataset weren't from features at all — they came from data hygiene (dropping ambiguously-labeled training rows, training per-weapon models instead of one pooled classifier, ensemble stacking). The v13 features matter on the margin. They don't transform a baseline classifier; they make a decent classifier slightly better in the specific places it was guessing — mid-air state, corpse shape, flick magnitude. Set expectations accordingly. None of the new features is a free lunch; the bug fixes mostly are.

## Performance

Heavily optimized — see [`JKDemoMetadata/OPTIMIZATION_NOTES.md`](JKDemoMetadata/OPTIMIZATION_NOTES.md). A 12.5 MB demo went from ~112 seconds in the original baseline to ~563 ms (≈198× speedup) via configstring caching, binary-search history lookups, snapshot-loop consolidation, reliable-command de-duplication, and cumulative distance tracking.

Tournament-scale full match recordings (~30 MB, 12 players, 20 minutes) take a few seconds per demo. Single-threaded but trivially parallelizable across files — most consumers run a worker pool over a directory of demos.

