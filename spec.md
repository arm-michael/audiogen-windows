# Windows CLI Port Spec: AudioGen

## What You're Dealing With

The project has two phases that are independent:
- **Phase 1 — Model conversion**: Python scripts on your Mac. No changes needed.
- **Phase 2 — C++ runtime app**: This is what needs porting to Windows.

The C++ app has exactly **two Windows-incompatible things**:
1. `#include <unistd.h>` + `getopt()` — POSIX-only CLI argument parsing
2. `CMakeLists.txt` — hardcodes `.a` library extension, a Unix-only `flatc` placeholder filename, and enables Arm SME2 unconditionally

Everything else (`<chrono>`, `<fstream>`, `std::`, TFLite, SentencePiece) already compiles on Windows.

## Target Platform

| Item | Value |
|---|---|
| OS | Windows 10/11 x64 |
| Compiler | MSVC 2022 (via GitHub Actions) |
| Architecture | x86-64 (Arm SME2 features disabled) |
| Distribution | Single `.exe` + model files (no installer needed) |

> **Note on Arm Windows**: If your users have Arm-based Windows machines (Snapdragon X Elite, Surface Pro X), the SME2 path would work there. x64 is covered here as the common case.

---

## Code Changes Required

### Change 1 — `audiogen.cpp`: Replace `getopt` with a portable parser

Replace line 34 (`#include <unistd.h>`) and the `getopt` loop (~line 441) as follows.

**Remove:**
```cpp
#include <unistd.h>
```

**Add after the other includes (line 34):**
```cpp
#ifdef _WIN32
// Minimal portable getopt for Windows
static int optind = 1;
static char* optarg = nullptr;
static int getopt(int argc, char* const argv[], const char* optstring) {
    if (optind >= argc || argv[optind][0] != '-') return -1;
    char opt = argv[optind][1];
    const char* p = strchr(optstring, opt);
    if (!p) return '?';
    optind++;
    if (*(p + 1) == ':') {
        if (optind >= argc) return '?';
        optarg = argv[optind++];
    }
    return opt;
}
#else
#include <unistd.h>
#endif
```

That's it for the C++ file. No other changes needed.

### Change 2 — `CMakeLists.txt`: Three fixes

**Fix A** — Static library extension (around the line where `SENTENCEPIECE_LIB` is set):

```cmake
# Replace this:
set(SENTENCEPIECE_LIB ${BINARY_DIR}/src/libsentencepiece.a)

# With this:
if(WIN32)
  set(SENTENCEPIECE_LIB ${BINARY_DIR}/src/Release/sentencepiece.lib)
else()
  set(SENTENCEPIECE_LIB ${BINARY_DIR}/src/libsentencepiece.a)
endif()
```

**Fix B** — The `flatc` placeholder filename (two lines, one `WRITE` and one `REMOVE`):

```cmake
# Replace:
file(WRITE ${FLATBUFFERS_BIN_DIR}/_deps/flatbuffers-build/flatc "")
# ...
file(REMOVE ${FLATBUFFERS_BIN_DIR}/_deps/flatbuffers-build/flatc "")

# With:
if(WIN32)
  set(FLATC_PLACEHOLDER ${FLATBUFFERS_BIN_DIR}/_deps/flatbuffers-build/flatc.exe)
else()
  set(FLATC_PLACEHOLDER ${FLATBUFFERS_BIN_DIR}/_deps/flatbuffers-build/flatc)
endif()
file(WRITE ${FLATC_PLACEHOLDER} "")
# ...
file(REMOVE ${FLATC_PLACEHOLDER} "")
```

**Fix C** — Disable SME2 on non-Arm (replace the unconditional `ON`):

```cmake
# Replace:
set(XNNPACK_ENABLE_ARM_SME2 ON CACHE BOOL "" FORCE)

# With:
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)")
  set(XNNPACK_ENABLE_ARM_SME2 ON CACHE BOOL "" FORCE)
else()
  set(XNNPACK_ENABLE_ARM_SME2 OFF CACHE BOOL "" FORCE)
endif()
```

---

## Build Strategy: GitHub Actions (No Windows Machine Required)

Since you're on Mac and have no Windows machine, GitHub Actions is the right tool. Every push triggers a free Windows build on Microsoft's infrastructure. You download the resulting `.exe`.

### Step-by-step setup

**1. Fork the repository**

```bash
# On your Mac, clone your fork
git clone https://github.com/YOUR_USERNAME/ML-examples.git
cd ML-examples
```

**2. Apply the code changes above**

Navigate to `kleidiai-examples/audiogen/app/` and edit the two files as described.

**3. Create the GitHub Actions workflow**

Create this file at `.github/workflows/build-windows.yml` in the repository root:

```yaml
name: Build Windows CLI

on:
  push:
    paths:
      - 'kleidiai-examples/audiogen/app/**'
      - '.github/workflows/build-windows.yml'
  workflow_dispatch:   # allows manual trigger from GitHub UI

jobs:
  build-windows:
    runs-on: windows-latest
    defaults:
      run:
        working-directory: kleidiai-examples/audiogen/app

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      # Cache the TensorFlow source (~5 GB git clone) between runs
      - name: Cache TensorFlow source
        uses: actions/cache@v4
        with:
          path: ${{ github.workspace }}/tf_src
          key: tensorflow-5baea41aa158ce6c3396726bd84fda5cd81737a0
          restore-keys: tensorflow-

      - name: Configure CMake
        run: |
          cmake -B build -G "Visual Studio 17 2022" -A x64 `
            -DCMAKE_BUILD_TYPE=Release `
            -DTF_SRC_PATH=${{ github.workspace }}/tf_src

      - name: Build
        run: cmake --build build --config Release --parallel

      - name: Collect artifact
        run: |
          mkdir -p dist
          copy build\Release\audiogen.exe dist\
          # Collect any required DLLs
          Get-ChildItem build\Release\*.dll | Copy-Item -Destination dist\ -ErrorAction SilentlyContinue

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: audiogen-windows-x64
          path: kleidiai-examples/audiogen/app/dist/
          retention-days: 30
```

> **Why `TF_SRC_PATH`**: TensorFlow is a 5 GB clone. Without caching it, every build would spend 10-20 minutes cloning. The cache key pins it to the exact commit SHA that the project requires.

**4. Push and watch the build**

```bash
git add .
git commit -m "Windows port: portable getopt, CMake fixes"
git push
```

Go to `github.com/YOUR_USERNAME/ML-examples/actions` → click the running workflow → watch it build. First run takes 30-60 minutes (TF clone + full build). Subsequent runs with cache hit take ~15-20 minutes.

**5. Download the artifact**

When the workflow succeeds: Actions tab → click the run → scroll to Artifacts → download `audiogen-windows-x64.zip`.

---

## Testing Without a Windows Machine

You have three practical options, in order of effort:

### Option A — GitHub Actions test job (free, no extra accounts)

Add a second job to the workflow that runs a basic smoke test. Extend the YAML:

```yaml
  test-windows:
    needs: build-windows
    runs-on: windows-latest
    steps:
      - name: Download artifact
        uses: actions/download-artifact@v4
        with:
          name: audiogen-windows-x64
          path: dist

      - name: Smoke test (help flag)
        run: dist\audiogen.exe -h
        # Should print usage and exit 1 (non-zero) — that's expected behavior

      - name: Smoke test (missing args)
        run: |
          dist\audiogen.exe -m nonexistent -p "test" -t 4
          # Should print a model file error
```

This gives you a live Windows execution log in the Actions UI, including stack traces if it crashes.

### Option B — AWS/Azure spot VM (~$0.05-0.10/hr, pay-as-you-go)

Spin up a Windows Server 2022 VM only when you need to do interactive testing:

```bash
# AWS CLI example — Windows Server 2022, t3.medium (~$0.05/hr)
aws ec2 run-instances \
  --image-id ami-0c7217cdde317cfec \
  --instance-type t3.medium \
  --key-name your-key \
  --security-groups allow-rdp
```

Connect via RDP from macOS using **Microsoft Remote Desktop** (free from Mac App Store). Copy your `.exe` over, run it, terminate the instance when done.

### Option C — Wine on macOS (quick sanity checks, limited)

Wine can run simple Windows executables on macOS. It won't handle complex CPU delegate code well, but will catch obvious issues like missing symbols or startup crashes:

```bash
brew install --cask wine-stable
wine audiogen.exe -h
```

---

## Complete Workflow: End to End

```
Your Mac                      GitHub (free)              Windows VM (optional)
-----------                   ---------------            --------------------
1. Python model conversion
   (no Windows involved)

2. Edit audiogen.cpp +
   CMakeLists.txt

3. git push  ──────────────►  4. GitHub Actions runs
                                  windows-latest runner
                                  cmake + MSVC build
                                  upload audiogen.exe

5. Download artifact ◄────────
   audiogen-windows-x64.zip

6. Ship to user                                          7. User runs:
   (include model files)                                    audiogen.exe -m models\
                                                                        -p "jazz drums"
                                                                        -t 8
```

---

## Distribution Package

What you ship to Windows users:

```
audiogen-windows-x64/
├── audiogen.exe
├── README.txt          ← usage instructions
└── models/             ← user must populate from HuggingFace + conversion scripts
    ├── conditioners_float32.tflite
    ├── dit_model.tflite
    ├── autoencoder_model.tflite
    ├── autoencoder_encoder_model.tflite
    └── spiece.model
```

The model files are ~2-8 GB and come from HuggingFace + the Python conversion scripts on your Mac. They are NOT included in the build artifact — they must be produced separately and distributed alongside the exe.

---

## Known Limitations on x64 Windows

| Feature | Status |
|---|---|
| Text-to-audio generation | Works |
| Audio style transfer | Works |
| XNNPACK CPU acceleration | Works (x64 path) |
| Arm KleidiAI / SME2 optimizations | Disabled (x64 has no Arm SIMD) |
| Performance vs Arm native | ~2-3x slower — expected |

If your target users have Arm Windows machines (Snapdragon X Elite laptops), add a second GitHub Actions job with `-A ARM64` and the SME2 path will be active again.

---

## Summary: Files to Create/Modify

| File | Action |
|---|---|
| `kleidiai-examples/audiogen/app/audiogen.cpp` | Edit: add `#ifdef _WIN32` getopt block, remove `#include <unistd.h>` |
| `kleidiai-examples/audiogen/app/CMakeLists.txt` | Edit: 3 fixes (sentencepiece lib path, flatc placeholder, SME2 flag) |
| `.github/workflows/build-windows.yml` | Create: new file |
