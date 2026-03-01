from huggingface_hub import hf_hub_download
import os

ws = os.path.expanduser("~/audiogen-workspace")
for f in ["model.ckpt", "model_config.json"]:
    hf_hub_download("stabilityai/stable-audio-open-small", f, local_dir=ws)
print("Done.")
