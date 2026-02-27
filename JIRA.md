# JIRA.md — AudioGen Windows CLI Port

> Replaces Jira. This file is the authoritative source of truth for all work breakdown,
> task status, and push history. Update after every GitHub push.

Status legend: `[ ]` To Do · `[~]` In Progress · `[x]` Done · `[!]` Blocked

---

## Epics

### WIN-1: Port AudioGen C++ CLI to Windows x64

**Status:** `[x]` Done — all stories merged, CI pipeline live
**Goal:** Produce a working `audiogen.exe` for Windows 10/11 x64 built entirely via GitHub Actions, requiring zero Windows machine for the developer.
**Spec:** `spec.md`
**Acceptance:** GitHub Actions `windows-latest` build passes; `audiogen.exe -h` runs on Windows; macOS build unbroken.

---

## Stories

### WIN-1.1 — Portable C++ Source (audiogen.cpp)

**Status:** `[x]` Done — PR #1 merged
**Priority:** P0 — blocks all other stories
**Parent:** WIN-1
**Files:** `app/audiogen.cpp`

**User Story:**
As a developer, I want `audiogen.cpp` to compile with MSVC on Windows without modifications beyond a single `#ifdef _WIN32` block, so that the Windows and Unix builds share one source file.

**Acceptance Criteria:**
- Given an MSVC compiler, when `audiogen.cpp` is compiled, then there are zero errors related to `unistd.h` or `getopt`.
- Given the portable `getopt` shim, when `-m`, `-p`, `-t`, `-s`, `-i`, `-x`, `-l`, `-n`, `-o`, `-h` flags are parsed, then each produces the correct value (same behavior as POSIX getopt).
- Given a macOS build, when `audiogen.cpp` is compiled with clang, then behavior is identical to pre-change.

**Test Design:**
- Build verification: MSVC compile step produces no errors or warnings about missing headers
- Smoke test: `audiogen.exe -h` prints usage text
- Smoke test: `audiogen.exe -m x -p y -t 4` reaches model-load error (not arg-parse error)
- Regression: macOS build (`cmake -DCMAKE_BUILD_TYPE=Release ..`) still passes

**Tasks:**
- [x] Remove `#include <unistd.h>`
- [x] Add `#ifdef _WIN32` block with portable `getopt`, `optind`, `optarg`
- [x] Add `#else` branch restoring `#include <unistd.h>` for Unix
- [x] Verify no other POSIX-only includes exist in the file

**Rollback:** Revert the single `#ifdef` block; no other files touched.

---

### WIN-1.2 — CMake Windows Compatibility

**Status:** `[x]` Done — PR #2 merged
**Priority:** P0 — blocks WIN-1.3
**Parent:** WIN-1
**Files:** `app/CMakeLists.txt`

**User Story:**
As a developer, I want `CMakeLists.txt` to build correctly on both Windows (MSVC) and Unix (clang/gcc), so that a single CMake file serves all platforms.

**Acceptance Criteria:**
- Given a Windows CMake configure step, when `SENTENCEPIECE_LIB` is resolved, then it points to `sentencepiece.lib` (not `libsentencepiece.a`).
- Given a Windows CMake configure step, when the flatc placeholder is written, then the filename is `flatc.exe` (not `flatc`), matching what the MSVC build of flatbuffers produces.
- Given an x86-64 Windows target, when CMake configures XNNPACK, then `XNNPACK_ENABLE_ARM_SME2` is `OFF` and no Arm-specific errors occur.
- Given a macOS or Linux configure step, then all three items above behave as they did before this change.

**Test Design:**
- Build verification: CMake configure step on `windows-latest` produces no errors
- Build verification: `cmake --build` step completes with exit code 0
- Regression: macOS CMake configure and build still succeed

**Tasks:**
- [x] Make `SENTENCEPIECE_LIB` conditional on `WIN32` (`.lib` vs `.a`)
- [x] Extract flatc placeholder path to `FLATC_PLACEHOLDER` variable with `WIN32` conditional (`.exe` suffix)
- [x] Replace `file(WRITE ...)` and `file(REMOVE ...)` to use `FLATC_PLACEHOLDER`
- [x] Make `XNNPACK_ENABLE_ARM_SME2` conditional on `CMAKE_SYSTEM_PROCESSOR` matching aarch64/arm64

**Rollback:** Revert `CMakeLists.txt`; macOS builds are unaffected.

---

### WIN-1.3 — GitHub Actions Windows CI Pipeline

**Status:** `[x]` Done — PR #3 merged, workflow live
**Priority:** P0
**Parent:** WIN-1
**Files:** `.github/workflows/build-windows.yml` *(new file)*
**Depends on:** WIN-1.1, WIN-1.2

**User Story:**
As a developer without a Windows machine, I want a GitHub Actions workflow that builds `audiogen.exe` on every push and uploads it as a downloadable artifact, so that I can distribute the binary without a local Windows environment.

**Acceptance Criteria:**
- Given a push to any branch touching `app/**`, when the workflow runs, then a `windows-latest` runner builds `audiogen.exe` successfully.
- Given a completed build, when I go to the Actions run page, then I can download `audiogen-windows-x64.zip` containing `audiogen.exe`.
- Given repeated pushes, when the TensorFlow source has been cached, then the build step skips the 5 GB clone and finishes faster.
- Given a completed build, when the smoke test job runs, then `audiogen.exe -h` exits with output on stdout/stderr and does not crash.
- Given the `workflow_dispatch` trigger, when I manually run the workflow from the GitHub UI, then it builds successfully.

**Test Design:**
- Workflow trigger: push to `app/**` fires the job
- Workflow trigger: manual `workflow_dispatch` fires the job
- Build step: `cmake --build` exits 0
- Artifact: zip contains `audiogen.exe`
- Smoke test job: `audiogen.exe -h` produces usage text
- Smoke test job: `audiogen.exe` with no args produces error text (not a crash/segfault)
- Cache: second run restores TF source from cache (verify via Actions timing)

**Tasks:**
- [x] Create `.github/workflows/audiogen-build-windows.yml`
- [x] Add checkout step
- [x] Add TF source cache step (key: TF commit SHA)
- [x] Add CMake configure step (`Visual Studio 17 2022`, x64, `Release`, `TF_SRC_PATH`)
- [x] Add CMake build step (parallel)
- [x] Add artifact collection and upload step (30-day retention)
- [x] Add `smoke-test-windows` job depending on `build-windows`
- [x] Add smoke test steps in test job
- [x] Add `workflow_dispatch` trigger
- [x] Verify `paths` filter covers `app/**` and the workflow file itself

**Rollback:** Delete the workflow file; no existing builds are affected.

---

### WIN-1.4 — Distribution Package & Documentation

**Status:** `[x]` Done — PR #4 merged
**Priority:** P1
**Parent:** WIN-1
**Files:** `app/README-windows.md` *(new file)*
**Depends on:** WIN-1.3

**User Story:**
As a Windows user receiving the binary, I want clear instructions for downloading, setting up model files, and running `audiogen.exe`, so that I can generate audio without needing developer tools.

**Acceptance Criteria:**
- Given the `README-windows.md`, when a Windows user follows the instructions step-by-step, then they can run `audiogen.exe -m models\ -p "jazz drums" -t 8` successfully.
- Given the distribution zip, when it is extracted, then the directory structure matches `spec.md §Distribution Package`.
- Given the README, when a user encounters a missing model file error, then the README explains where to get model files.

**Tasks:**
- [x] Write `app/README-windows.md` covering: prerequisites, download, directory layout, model setup, usage examples, common errors
- [x] Document the expected directory structure (`models/` subfolder with all `.tflite` files and `spiece.model`)
- [x] Document the `ffmpeg` command for converting audio input files (style transfer use case)
- [x] Add known limitations section (no SME2, ~2-3x slower than Arm native)

**Rollback:** No code changes; documentation only.

---

---

### WIN-1.5 — BUG: CMake path escaping on Windows Actions runner

**Status:** `[x]` Done — PR #5 merged
**Severity:** SEV-1 (blocks all CI builds)
**Found:** CI run 22504607061 — `build-windows` job, Configure CMake step
**Root cause:** GitHub Actions `windows-latest` provides workspace path with backslashes (`D:\a\...`). CMake interprets `\a` as an escape sequence. TFLite `CMakeLists.txt:746` fails with `Invalid character escape '\a'` for every source file path containing `\a`.
**Fix:** `file(TO_CMAKE_PATH "${TF_SRC_PATH}" TF_SRC_PATH_NORMALIZED)` normalizes backslashes to forward slashes before `TENSORFLOW_SOURCE_DIR` is set.

**Tasks:**
- [x] Identify root cause from CI logs
- [x] Apply `file(TO_CMAKE_PATH)` fix to `CMakeLists.txt`
- [x] PR, squash merge, branch cleanup
- [x] Update JIRA.md push log

---

## Backlog

| ID | Title | Priority | Notes |
|----|-------|----------|-------|
| WIN-2.1 | Windows ARM64 build variant | P2 | Add second Actions job with `-A ARM64`; re-enables SME2 path for Snapdragon X Elite |
| WIN-2.2 | macOS build in CI | P2 | Add macOS Actions job to catch regressions automatically |
| WIN-2.3 | Codesign the exe | P3 | Needed for Windows Defender not to flag unsigned binary |
| WIN-2.4 | GitHub Release automation | P2 | Publish tagged release with artifacts on `v*` tags |

---

## Push Log

> Updated after every GitHub push. Most recent entry first.

| Date | Branch | PR | Stories Updated | Summary |
|------|--------|----|-----------------|---------|
| 2026-02-27 | main | — | — | chore: scaffold — planning docs, extended .gitignore |
| 2026-02-27 | feat/WIN-1.1-portable-getopt | #1 | WIN-1.1 ✅ | fix(audiogen): portable getopt for Windows MSVC |
| 2026-02-27 | build/WIN-1.2-cmake-windows-compat | #2 | WIN-1.2 ✅ | build(cmake): sentencepiece lib, flatc placeholder, SME2 conditional |
| 2026-02-27 | build/WIN-1.3-github-actions-pipeline | #3 | WIN-1.3 ✅ | build(ci): GitHub Actions build + smoke test pipeline |
| 2026-02-27 | docs/WIN-1.4-windows-readme | #4 | WIN-1.4 ✅ | docs(audiogen): Windows CLI user guide |
| 2026-02-27 | fix/WIN-1.5-cmake-path-normalization | #5 | WIN-1.5 ✅ | fix(cmake): normalize TF_SRC_PATH backslashes — CI SEV-1 fix |

---

## ADRs

| ID | Title | Status | Date |
|----|-------|--------|------|
| ADR-1 | Use GitHub Actions for Windows builds instead of MinGW cross-compilation | Accepted | 2026-02-27 |
| ADR-2 | Target Windows x64 (not ARM64) as primary platform | Accepted | 2026-02-27 |
| ADR-3 | Use inline `#ifdef _WIN32` getopt shim instead of third-party library | Accepted | 2026-02-27 |

### ADR-1: GitHub Actions over MinGW cross-compilation

**Context:** Developer has no Windows machine. Two main options: cross-compile via MinGW-w64 on macOS, or build natively via GitHub Actions.

**Decision:** GitHub Actions `windows-latest` runner with MSVC.

**Alternatives:**
| Alternative | Pros | Cons | Why Not |
|---|---|---|---|
| MinGW-w64 cross-compile on macOS | No CI needed | TFLite has poor MinGW support; runtime ABI differences; harder debugging | TFLite's own CMake is not tested with MinGW |
| GitHub Actions (chosen) | Real MSVC, same as users' machines; free; artifact download; no local Windows needed | Slower iteration loop (push to test) | — |

### ADR-2: x64 as primary target

**Context:** Arm Windows (Snapdragon X Elite) is growing but x64 is still the dominant Windows install base.

**Decision:** Target x64 first; ARM64 is backlogged as WIN-2.1.

### ADR-3: Inline getopt shim

**Context:** `audiogen.cpp` uses POSIX `getopt`. Options: add a dependency (e.g., `getopt-win`), use a header-only library, or write a minimal inline shim.

**Decision:** Inline `#ifdef _WIN32` shim (~12 lines). The shim only needs to handle single-char flags with optional arguments — exactly what `audiogen.cpp` uses. No new dependency, no new file, minimal diff.
