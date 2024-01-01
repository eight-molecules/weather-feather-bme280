# Adafruit Wunderground Weather Station
**Providing humidity, temperature, and pressure
to Weather.com through the PWS Upload Protocol.**

## Summary
This Arduino application for the Feather takes temperature readings and uploads them to a personal weather station registered to a Wunderground account. It takes a measurement every 60s and attempts to upload it every 60s, deep sleeping in between.

## Compatible Devices
Adafruit Feather ESP32-S2 with BME280

## Dependencies
Microsoft Visual Studio Code - https://code.visualstudio.com/Download
PlatformIO - https://platformio.org/install/ide?install=vscode

# Setup
1. Sign up for or log in to Wunderground.
2. Go to the Account Menu in the top right of the desktop website and select My Devices.
8. Create a new device with type "other".
3. Go to API Keys.
4. Generate an API Key.
5. Copy the API Key.
6. Paste the API Key into `env.h`.
7. Go to My Devices.
8. Copy the Device ID into `env.h`.
9. Copy the Device Password into `env.h`.

## Installation
1. Install Visual Studio Code and the PlatformIO extension.
2. Open the PlatformIO project in Visual Studio Code.
3. Attach a Feather ESP32-S2 with BME280 to the computer.
4. Build, Upload, and Monitor the project

If you encounter flashing errors attempt to reset the device while holding down the boot button.