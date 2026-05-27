import subprocess
import json
import os
import re
from datetime import datetime

VERSION_FILE = "include/version.h"
JSON_PATH    = "server/firmware.json"
BIN_SRC      = ".pio/build/esp32dev/firmware.bin"
BIN_DEST     = "server/firmware.bin"

user_profile = os.environ.get("USERPROFILE", "")
pio_exe = os.path.join(user_profile, ".platformio", "penv", "Scripts", "pio.exe")

# Read and auto-increment the build number 
# version.h stores: #define FIRMWARE_BUILD 5
if os.path.exists(VERSION_FILE):
    with open(VERSION_FILE, "r") as f:
        content = f.read()
    match = re.search(r'FIRMWARE_BUILD\s+(\d+)', content)
    build = int(match.group(1)) + 1 if match else 1  # increment by 1
else:
    build = 1

# Format as "1.0.<build>" 
version = f"1.0.{build}"
print(f"New version: {version}")

# Write the new build number back to version.h 
os.makedirs("include", exist_ok=True)
with open(VERSION_FILE, "w") as f:
    f.write(f'#pragma once\n')
    f.write(f'#define FIRMWARE_VERSION "{version}"\n')
    f.write(f'#define FIRMWARE_BUILD   {build}\n')

print(f"Updated version.h → {version}")

# Build the firmware 
print("Building firmware...")
subprocess.run([pio_exe, "run"], check=True)

# Update firmware.json 
os.makedirs("server", exist_ok=True)
data = {"version": version, "url": "https://raw.githubusercontent.com/Erikgdfgdf/OTA/refs/heads/main/OTA-ESP32/server/firmware.bin"}
with open(JSON_PATH, "w") as f:
    json.dump(data, f, indent=2)

print(f"Updated firmware.json → {version}")

# Copy .bin to server folder 
with open(BIN_SRC, "rb") as src, open(BIN_DEST, "wb") as dst:
    dst.write(src.read())

print("Copied firmware.bin")

# Push to GitHub 
print("Pushing to GitHub...")
subprocess.run(["git", "add", VERSION_FILE, JSON_PATH, BIN_DEST], check=True)
subprocess.run(["git", "commit", "-m", f"OTA release {version}"], check=True)
subprocess.run(["git", "push"], check=True)

print(f"Done! Version {version} is live. ESP32 will update within 30 seconds.")