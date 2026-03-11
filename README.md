<!--
    SPDX-FileCopyrightText: Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>

    SPDX-License-Identifier: Apache-2.0
-->

# Building and Running the Audio Generation Application on Arm® CPUs with the Stable Audio Open Small Model

Welcome to the home of audio generation on Arm® CPUs, featuring Stable Audio Open Small with Arm® KleidiAI™. This project provides everything you need to:

- Convert models to LiteRT-compatible formats
- Run these models on Arm® CPUs using the LiteRT runtime, with support from XNNPack and Arm® KleidiAI™

## Prerequisites

- Experience with Arm® cross-compilation on Android™ or Linux®, or macOS®
- Proficiency with Android™ shell commands
- An Android™ mobile phone or platform with an Arm® CPU with at least <strong>FEAT_DotProd</strong> (dotprod) and optionally <strong>FEAT_I8MM</strong> (i8mm) feature.
- Minimum device RAM requirement: 8GB.

## Introduction
![Stable Audio Open Model](./model.png?raw=true "Stable Audio Open Model")

The Stable Audio Open Small Model is made of three submodules: Conditioners (Text conditioner and number conditioners), a diffusion transformer (DiT), and an AutoEncoder:
* Conditioners: Consist of T5-based text encoder for the input prompt and a number conditioner for total seconds input. The conditioners encode the inputs into numerical values to be passed to DiT model.
* Diffusion transformer (DiT): It takes a random noise, and denoises it through a defined number of steps, to resemble what the conditioners intent.
* AutoEncoder: It compresses the input waveforms into a manageable sequence length to be processed by the DiT model. At the end of de-noising step, it decompresses the result into a waveform.

## Setup

### Step 1
Clone this repository in your terminal, then navigate to `kleidiai-examples/audiogen/` directory.

### Step 2
Create a workspace directory for managing the dependencies and repositories. Export the WORKSPACE variable to point to this directory, which you will use in the following steps:
```bash
mkdir my-workspace
export WORKSPACE=$PWD/my-workspace
```

### Step 3
Download the Stable Audio Open Small Model. The model is open-source and optimized for generating short audio samples, sound effects, and production elements using text prompts.

Login to HuggingFace and navigate to the model landing page: [https://huggingface.co/stabilityai/stable-audio-open-small/tree/main](https://huggingface.co/stabilityai/stable-audio-open-small/tree/main)

Download and copy the configuration file `model_config.json` and the model itself, `model.ckpt`, to your workspace directory, and verify they exist by running the command:
```bash
ls $WORKSPACE/model_config.json $WORKSPACE/model.ckpt
```

## Part 1 - Converting the submodules to LiteRT format
The instructions to convert the three submodules of the Stable Audio Open Small Model to LiteRT format are detailed in the [README.md](./scripts/README.md) file in the `scripts/` folder.

## Part 2 - Build instructions
The instructions to build the audio generation application to run on Android™ are detailed in the [README.md](./app/README.md) file in the `app/` folder. Instructions are valid for both Linux® and macOS® systems.

## Part 3 - Windows (x64 and ARM64)

`audiogen.exe` is supported on Windows 10/11 x64 and ARM64 (Windows on Arm). No Windows machine or local build environment is required — binaries for both targets are built and tested entirely via GitHub Actions.

### Getting the binary

Download `audiogen.exe` from the latest GitHub Actions run:

1. Go to **Actions → AudioGen — Build Windows**
2. Open the most recent successful run
3. Download **`audiogen-windows-x64`** (Intel/AMD) or **`audiogen-windows-arm64`** (ARM64 / Windows on Arm)

### Running inference on Windows

Pre-converted model files are available in the [GitHub Release](../../releases/tag/audiogen-models-v1). Download all 5 files and place them in a `models\` folder next to the binary, then run:

```cmd
audiogen.exe -m models -p "jazz piano" -t 4 -n 8 -l 10
```

See [app/README-windows.md](./app/README-windows.md) for full setup instructions, directory layout, and troubleshooting.

### CI inference test

To run end-to-end inference on a Windows GitHub Actions runner without a local Windows machine:

**x64:**
1. Go to **Actions → AudioGen — Inference Test Windows x64**
2. Click **Run workflow**
3. Download the **`audiogen-output`** artifact and play `output.wav`

**ARM64 (Windows on Arm):**
1. Go to **Actions → AudioGen — Inference Test Windows ARM64**
2. Click **Run workflow**
3. Download the **`audiogen-output-arm64`** artifact and play `output.wav`

## Trademarks

* Arm® and KleidiAI™ are registered trademarks or trademarks of Arm® Limited (or its subsidiaries) in the US and/or
  elsewhere.
* Android™ is a trademark of Google LLC.
* macOS® is a trademark of Apple Inc.
