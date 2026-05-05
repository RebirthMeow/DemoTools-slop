# JKDemoMetadata Optimization Notes

This document tracks the performance optimizations applied to the `jkdemometadata` tool to improve processing speed for large JKA demo files.

## Performance Benchmarks
**Test Environment:** Windows 10, Mingw-w64 (GCC 10.3)
**Test File:** `1.1673a.dm_26` (12.5 MB)

| Version | Execution Time | Speedup |
| :--- | :--- | :--- |
| **Baseline (Original)** | **111,605 ms** | 1x |
| **Optimized (v1.1)** | **563 ms** | **~198x** |

---

## Applied Optimizations

### 1. Configstring Caching (`utils.cpp`)
*   **Problem:** Every snapshot call to `getPlayerName`, `getUniqueId`, etc., triggered redundant `Info_ValueForKey` parses and expensive UTF-8 conversions/allocations.
*   **Fix:** Implemented a `clientCache` that monitors the `stringOffsets` of the gamestate. Values are only re-parsed when the offset (indicating a configstring update) changes.
*   **Impact:** Massive reduction in CPU time and heap fragmentation.

### 2. Binary Search for History Lookups (`main.cpp`)
*   **Problem:** `findPlayerHistoryAt` performed a linear $O(N)$ scan of 1,000 slots per query.
*   **Fix:** Implemented binary search ($O(\log N)$) over the circular `playerHistory` buffer.
*   **Impact:** Improves scalability for demos with high entity density or complex analytics.

### 3. Entity Loop Consolidation (`main.cpp`)
*   **Problem:** The main snapshot loop iterated over entities twice (once for mapping, once for events).
*   **Fix:** Consolidated all entity processing (mapping, event detection, obituary logic, and maker-mod tracking) into a single pass.
*   **Impact:** ~35% reduction in snapshot processing time.

### 4. Reliable Command Optimization (`main.cpp`)
*   **Problem:** The tool re-parsed the entire reliable command buffer (128 commands) every snapshot to check for `sv_serverid` changes.
*   **Fix:** Advanced `lastExecutedServerCommand` correctly so each command is only tokenized and parsed once.

### 5. Cumulative Distance Tracking (`main.cpp`)
*   **Problem:** Calculating "movement in the last second" required a linear scan and multiple `sqrt` calls for every kill event.
*   **Fix:** Added `cumDistance` to `playerHistory_t`. Distance is calculated once when a snapshot is recorded. Movement queries are now $O(1)$ (end - start).

---

## Build Instructions
The project now includes a `build.bat` for Windows environments using the `msys64` Mingw-w64 toolchain.

```batch
# Requirements: gcc, g++, libjansson
./build.bat
```
