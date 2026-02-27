<!--
SPDX-FileCopyrightText: Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
SPDX-License-Identifier: Apache-2.0
-->

# AudioGen — Windows x64 CLI

Generate audio from text prompts on Windows using the Stable Audio Open Small model.

---

## Prerequisites

| Requirement | Notes |
|---|---|
| Windows 10 or 11 (x64) | ARM64 Windows is not yet supported |
| 8 GB RAM minimum | The DiT model is memory-intensive |
| ~3 GB free disk space | For model files |

No compiler, Visual Studio, or Python installation is required to run the binary.

---

## Getting the Binary

1. Go to the [Actions tab](https://github.com/arm-michael/ML-examples/actions/workflows/audiogen-build-windows.yml)
2. Click the most recent successful run
3. Scroll to **Artifacts** at the bottom of the run page
4. Download **`audiogen-windows-x64.zip`**
5. Extract it — you will find `audiogen.exe` inside

---

## Getting the Model Files

The model files are not included in the binary distribution. They must be obtained separately.

### Option A — Use pre-converted files (if provided)

If you received a `models/` folder alongside this README, skip to [Directory Layout](#directory-layout).

### Option B — Convert the model yourself (requires a Mac or Linux machine with Python)

Follow the full model conversion instructions in `../scripts/README.md`:

```bash
# 1. Install Python dependencies
bash ../install_requirements.sh

# 2. Download the model from HuggingFace
#    https://huggingface.co/stabilityai/stable-audio-open-small
#    Download: model_config.json and model.ckpt

# 3. Export conditioners (T5 encoder)
python ../scripts/export_conditioners.py

# 4. Export DiT and AutoEncoder
python ../scripts/export_dit_autoencoder.py

# 5. Download the SentencePiece tokenizer
#    https://huggingface.co/google-t5/t5-base/blob/main/spiece.model
```

---

## Directory Layout

Arrange files as follows before running:

```
audiogen-windows-x64\
├── audiogen.exe
└── models\
    ├── conditioners_float32.tflite
    ├── dit_model.tflite
    ├── autoencoder_model.tflite
    ├── autoencoder_encoder_model.tflite
    └── spiece.model
```

The `-m` flag points to the `models\` folder. The names of the `.tflite` files and `spiece.model` must match exactly.

---

## Usage

Open **Command Prompt** or **PowerShell** and `cd` to the folder containing `audiogen.exe`.

### Basic — text to audio

```bat
audiogen.exe -m models -p "warm arpeggios on house beats 120BPM with drums" -t 4
```

This generates a WAV file named after the prompt in the current directory (e.g. `warm_arpeggios_on_house_beats_120bpm_with_drums_99.wav`).

### All options

```
audiogen.exe -m <models_path> -p <prompt> -t <threads> [options]

Required:
  -m <path>     Path to the models folder
  -p <text>     Text description of the audio to generate
  -t <n>        Number of CPU threads (try 4–8 to start)

Optional:
  -s <seed>     Random seed for reproducibility (default: 99)
  -l <seconds>  Audio length in seconds (default: 10)
  -n <steps>    Diffusion steps — more steps = higher quality, slower (default: 8)
  -o <file>     Output WAV filename (default: <prompt>_<seed>.wav)
  -i <file>     Input WAV for audio-to-audio style transfer (see below)
  -x <0.0-1.0>  Noise level for style transfer (default: 1.0)
  -h            Show this help
```

### Examples

```bat
:: Set the seed for reproducible output
audiogen.exe -m models -p "jazz piano late night" -t 4 -s 42

:: Longer clip, more diffusion steps
audiogen.exe -m models -p "cinematic orchestra strings" -t 8 -l 20 -n 16

:: Explicit output filename
audiogen.exe -m models -p "lo-fi hip hop chill" -t 4 -o lofi_output.wav
```

---

## Audio-to-Audio Style Transfer

You can use an existing audio file to guide the generation.
The input file **must** be 44.1 kHz, stereo, 32-bit float WAV.

Convert any audio file with [ffmpeg](https://ffmpeg.org/download.html):

```bat
ffmpeg -i input.mp3 -ar 44100 -ac 2 -c:a pcm_f32le -f wav input_converted.wav
```

Then pass it with `-i`:

```bat
audiogen.exe -m models -p "jazz piano late night" -t 4 -i input_converted.wav -x 0.7
```

`-x` controls how much of the original audio is kept (0.0 = all noise / original ignored, 1.0 = full noise / use original structure).

---

## Performance Notes

| Topic | Detail |
|---|---|
| Thread count | Start with 4. Increase to the number of physical cores for best throughput. Hyperthreaded cores add little benefit for inference. |
| Generation time | A 10-second clip at 8 steps takes roughly 3–8 minutes on a modern x64 laptop. |
| Arm SME2 | This build is x64 and does not use Arm KleidiAI optimisations. It is ~2–3× slower than the native Arm build. |
| Memory | Keep other applications closed. The three models together require ~3–4 GB of RAM at peak. |

---

## Common Errors

| Error message | Cause | Fix |
|---|---|---|
| `ERROR: Missing required arguments` | `-m`, `-p`, or `-t` not supplied | Add all three required flags |
| `Error at audiogen.cpp:N` | Model file not found or wrong path | Check `-m` points to the folder, not a file; verify all five model files are present |
| `BAD file, or unsupported format` | Input audio is wrong format | Run the `ffmpeg` conversion command above |
| `Unsupported WAV format` | Input audio is not 44.1 kHz / stereo / 32-bit float | Same fix as above |
| Application exits immediately with no output | Missing Visual C++ runtime | Download and install [Microsoft Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist) |

---

## Building from Source

If you need to build the binary yourself, see [`../spec.md`](../spec.md) for the complete Windows port specification, including the GitHub Actions CI setup that produces official builds.
