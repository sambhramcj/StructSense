---
publishDate: 2026-05-24T00:00:00Z
title: StructSense - Real-Time Structural Health Monitoring Using Multi-Sensor Fusion and Edge Intelligence on the MYOSA Platform
excerpt: StructSense is a low-cost IoT system built on the MYOSA Mini Kit that detects structural anomalies in bridges and beams using FFT-based vibration analysis, resonance frequency tracking, and adaptive multi-sensor fusion — all running on-device on the ESP32.
image: StructSense/structsense-cover.jpeg
tags:
  - StructuralHealthMonitoring
  - IoT
  - ESP32
  - MYOSA
  - FFT
  - SensorFusion
---

> "We don't just detect vibration — we detect structural behavior change in the frequency domain."

---

## Acknowledgements

We thank **Dr. Soniya Agrawal**, Assistant Professor, Dept. of EEE, BMS College of Engineering, Bengaluru, for her guidance and support as our faculty mentor. We also thank the **IEEE Sensors Council** and the **MYOSA team** for this opportunity.

---

## Overview

Infrastructure failure is one of the most preventable yet persistent crises globally. A vast aging bridge and infrastructure network lacks continuous structural monitoring, and structural failures often occur due to undetected gradual degradation — damage that builds up over months before becoming visible. Conventional SHM systems are expensive and difficult to deploy at scale, making them inaccessible for most public infrastructure.

**StructSense** proves that effective, continuous structural monitoring is achievable under ₹3,000 using the MYOSA Mini Kit.

**Key features:**
* FFT-based dominant frequency extraction using Hamming windowing — detects resonance shifts that indicate structural damage
* Fully adaptive thresholds — auto-calibrated from your environment using mean + K×σ, nothing hardcoded
* Multi-signal analysis combining vibration amplitude and resonance shift — improves robustness over simple threshold systems
* Moving average noise filter — 4-sample circular buffer smooths MPU6050 output before thresholding
* Real-time OLED status display — NORMAL / WARNING / CRITICAL
* Contactless gesture recalibration via APDS9960 — wave hand to reset baseline
* Live data on MYOSA BLE Android App dashboard via myosa.sendBleData()
* BMP180 as auxiliary environmental sensor for atmospheric compensation

**Alert classification:**
* **NORMAL** — all signals within adaptive baseline parameters
* **WARNING** — vibration RMS anomaly detected (single-signal)
* **CRITICAL** — RMS exceeds critical threshold alone, OR vibration + resonance shift dual-signal confirmation

<p align="center">
  <img src="/assets/images/StructSense/structsense-cover.jpeg" width="800"><br/>
  <i>StructSense — MYOSA Mini Kit mounted on demo bridge beam</i>
</p>

---

## Demo / Examples

### Images

<p align="center">
  <img src="/assets/images/StructSense/structsense-setup.jpeg" width="800"><br/>
  <i>Full setup — MYOSA board with all sensors connected via JST cables, mounted on the demo bridge model</i>
</p>

<p align="center">
  <img src="/assets/images/StructSense/structsense-normal.jpeg" width="800"><br/>
  <i>OLED display showing STATUS: NORMAL — structure vibrating within adaptive baseline parameters</i>
</p>

<p align="center">
  <img src="/assets/images/StructSense/structsense-warning.jpeg" width="800"><br/>
  <i>OLED display showing STATUS: WARNING — vibration anomaly detected</i>
</p>

<p align="center">
  <img src="/assets/images/StructSense/structsense-critical.jpeg" width="800"><br/>
  <i>OLED showing !! CRITICAL !! — dual-signal confirmation, damage signature detected</i>
</p>

<p align="center">
  <img src="/assets/images/StructSense/structsense-app.jpeg" width="800"><br/>
  <i>MYOSA BLE App showing live sensor data — accelerometer readings and system status</i>
</p>

### Videos

**Recorded Presentation (5 min):**

<video controls width="100%">
  <source src="/assets/images/StructSense/structsense-presentation.mp4" type="video/mp4">
</video>

**Demonstration Video (3 min):**

<video controls width="100%">
  <source src="/assets/images/StructSense/structsense-demo.mp4" type="video/mp4">
</video>

---

## Features

### 1. FFT-Based Vibration Analysis and Dominant Frequency Extraction

The MPU6050 accelerometer samples tri-axial acceleration at 500 Hz — sufficient for structural vibrations which occur below 100 Hz (Nyquist theorem requires 200 Hz minimum; 500 Hz provides comfortable margin). 64 readings per cycle are collected and passed through a Fast Fourier Transform with Hamming windowing to reduce spectral leakage.

Two key values are extracted:
- **Vibration RMS** — overall vibration energy magnitude
- **Dominant Frequency** — peak frequency bin computed as `peakIndex × (SAMPLE_RATE / SAMPLES)`

Structural degradation can alter resonance characteristics — healthy structures vibrate at consistent frequencies, damage shifts them. StructSense tracks this shift via FFT, not just raw amplitude.

<p align="center">
  <img src="/assets/images/StructSense/structsense-setup.jpeg" width="800"><br/>
  <i>MPU6050 captures tri-axial acceleration; FFT extracts frequency content for anomaly detection</i>
</p>

### 2. Fully Adaptive Thresholds

Unlike fixed-threshold systems, StructSense computes all thresholds from 20 calibration readings during startup:

```
baseline_rms      = mean of 20 RMS readings
baseline_freq     = mean of 20 dominant frequency readings
baseline_pressure = mean of 20 pressure readings

threshold_warning_rms  = baseline_rms + K × std_dev(RMS)
threshold_critical_rms = baseline_rms + 2K × std_dev(RMS)
threshold_freq_shift   = K × std_dev(frequency)
threshold_pressure     = K × std_dev(pressure)
```

Where K = 2.5 (adjustable via a single constant). All three signals share one sensitivity parameter — clean, justifiable, and self-adapting to any structure or environment.

### 3. Multi-Signal Analysis

StructSense combines independent physical signals for alert classification:

| Alert Level | Condition |
|---|---|
| NORMAL | All signals within baseline parameters |
| WARNING | Vibration RMS anomaly detected |
| CRITICAL | RMS exceeds critical threshold alone |
| CRITICAL | Vibration anomaly AND frequency shift (resonance damage signature) |

The BMP180 barometric pressure sensor serves as an auxiliary environmental sensor to improve system robustness against atmospheric variations.

### 4. Moving Average Noise Filter

A 4-sample circular moving average buffer smooths the raw MPU6050 RMS signal before thresholding, reducing sensor noise while preserving genuine structural anomalies. The buffer initialises to `baseline_rms` during calibration to avoid cold-start spikes.

### 5. Contactless Gesture Recalibration

The APDS9960 gesture sensor enables hands-free recalibration. Any detected gesture triggers a full re-calibration cycle — recomputing all baselines and adaptive thresholds from 20 fresh readings. Critical for real-world deployment where structures experience gradual baseline drift from thermal expansion or load changes.

### 6. MYOSA BLE App Dashboard

All sensor data is transmitted to the MYOSA BLE Android App via `myosa.sendBleData()` at 1.5-second intervals. The app displays live accelerometer readings, pressure values, and system status — enabling real-time remote monitoring with no Wi-Fi or cloud connectivity required.

<p align="center">
  <img src="/assets/images/StructSense/structsense-app.jpeg" width="800"><br/>
  <i>MYOSA BLE App showing live StructSense data stream</i>
</p>

---

## Usage Instructions

### Hardware Setup

1. Connect all sensor modules to the MYOSA Motherboard using the provided JST FF cables:
   - MPU6050 → any I2C port
   - BMP180 → any I2C port
   - APDS9960 → any I2C port
   - OLED Display → any I2C port
   - Actuator (buzzer) → any I2C port
2. Connect MYOSA board to laptop via USB-C cable
3. Mount the board firmly on your structure using double-sided tape

### Software Setup

```
1. Install Arduino IDE 2.x from arduino.cc/en/software

2. Add ESP32 Board Manager URL in File > Preferences:
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

3. Install MYOSA Libraries:
   Go to https://github.com/myosa-sensors/arduino-libraries
   Download ZIP > Extract > Copy all folders to Documents/Arduino/libraries/

4. Install arduinoFFT library:
   Tools > Manage Libraries > search "arduinoFFT" by kosme > Install

5. Install MYOSA BLE App APK:
   https://github.com/myosa-sensors/android-apk/raw/refs/heads/main/base.apk
```

### Running StructSense

```
1. Upload StructSense_Final.ino to MYOSA board
   (Tools > Board: ESP32 Dev Module)

2. Power on — OLED shows "Calibrating... Keep STILL!" for ~6 seconds

3. Keep board completely still during calibration

4. OLED shows "Ready! Adaptive SHM Active v3"

5. Open MYOSA BLE App > scan > connect to "MYOSA"

6. Live data appears in app

7. Tap or vibrate the structure to trigger WARNING or CRITICAL

8. Wave hand over APDS9960 to recalibrate at any time
```

---

## Tech Stack

* **MYOSA Motherboard** — ESP32 WROOM-32E (Wi-Fi + BLE)
* **AccelAndGyro.h** — Official MYOSA library for MPU6050
* **BarometricPressure.h** — Official MYOSA library for BMP180
* **LightProximityAndGesture.h** — Official MYOSA library for APDS9960
* **oled.h** — Official MYOSA library for SSD1306 OLED (128×64)
* **myosa.h** — Official MYOSA master class (BLE data transmission)
* **arduinoFFT** — FFT signal processing by kosme (only non-MYOSA library)
* **MYOSA BLE Android App** — Official live sensor dashboard

---

## Requirements

```
Arduino IDE 2.x
ESP32 Board Manager (Espressif Systems)
MYOSA Libraries: https://github.com/myosa-sensors/arduino-libraries
arduinoFFT: install via Arduino Library Manager (kosme)
MYOSA BLE App: https://github.com/myosa-sensors/android-apk
```

---

## File Structure

```
StructSense/
├── structsense-cover.jpeg
├── structsense-setup.jpeg
├── structsense-normal.jpeg
├── structsense-warning.jpeg
├── structsense-critical.jpeg
├── structsense-app.jpeg
├── structsense-demo.mp4
├── structsense-presentation.mp4
└── StructSense_Final.ino
structsense.md
```

---

## License

Open source. Developed by C J Sambhram and Aneesh U, BMS College of Engineering, Bengaluru, for MYOSA Event 5.0. MYOSA libraries remain property of MakeSense Edutech / MYOSA Sensors.
