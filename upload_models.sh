#!/bin/bash
gh release create audiogen-models-v1 \
  conditioners_tflite/conditioners_float32.tflite \
  dit_model.tflite \
  autoencoder_model.tflite \
  autoencoder_encoder_model.tflite \
  conditioners_tflite/spiece.model \
  --title "AudioGen Model Files v1" \
  --notes "TFLite models from stabilityai/stable-audio-open-small"
