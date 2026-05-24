// ============================================================
// StructSense v3 FINAL
// C J Sambhram & Aneesh U | BMSCE Bengaluru
// MYOSA Event 5.0
// ============================================================

#include <myosa.h>
#include <AccelAndGyro.h>
#include <BarometricPressure.h>
#include <oled.h>
#include <Wire.h>
#include <arduinoFFT.h>

// ============================================================
// MYOSA Objects
// ============================================================

MYOSA myosa;

AccelAndGyro imu;

BarometricPressure baro;

LightProximityAndGesture gesture;

oLed oled(128, 64);

// ============================================================
// FFT Settings
// ============================================================

#define SAMPLES     64
#define SAMPLE_RATE 500.0

// Adaptive threshold multiplier
#define K_FACTOR    2.5

// Moving average filter size
#define MA_SIZE     4

// ============================================================
// FFT Object
// ============================================================

double vReal[SAMPLES];
double vImag[SAMPLES];

ArduinoFFT<double> FFT(
  vReal,
  vImag,
  SAMPLES,
  SAMPLE_RATE
);

// ============================================================
// Baseline Variables
// ============================================================

float baseline_rms      = 0;
float baseline_stddev   = 0;

float baseline_pressure = 0;
float baseline_pstddev  = 0;

float baseline_freq     = 0;

// ============================================================
// Thresholds
// ============================================================

float thresh_warn_rms   = 0;
float thresh_crit_rms   = 0;

float thresh_pressure   = 0;
float thresh_freq_shift = 0;

// ============================================================
// Moving Average Filter
// ============================================================

float ma_buf[MA_SIZE] = {0};

int ma_idx = 0;

float ma_sum = 0;

// ============================================================
// State Variables
// ============================================================

bool calibrated = false;

int alertLevel = 0;

float domFreq = 0;

unsigned long lastBleUpdate = 0;

// ============================================================
// Struct
// ============================================================

struct VibData {

  float rms;

  float freq;
};

// ============================================================
// Function Prototypes
// ============================================================

void showOLED(
  const char* l1,
  const char* l2,
  const char* l3
);

void calibrateBaseline();

VibData collectRaw();

float smoothRMS(float newVal);

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  Wire.begin();

  Wire.setClock(100000);

  // =========================================================
  // Initialize MYOSA
  // =========================================================

  Serial.println(myosa.begin());

  // =========================================================
  // Sensor Checks
  // =========================================================

  if (!imu.begin()) {

    Serial.println("MPU6050 NOT FOUND");
  }

  if (!baro.begin()) {

    Serial.println("BMP180 NOT FOUND");
  }

  if (!oled.begin()) {

    Serial.println("OLED NOT FOUND");
  }

  // =========================================================
  // Gesture Sensor
  // =========================================================

  if (!gesture.begin()) {

    Serial.println("APDS9960 NOT FOUND");

  } else {

    gesture.enableGestureSensor();
  }

  // =========================================================
  // Welcome Screen
  // =========================================================

  oled.clearDisplay();

  oled.setTextSize(1);

  oled.setCursor(0, 0);
  oled.print("StructSense v3");

  oled.setCursor(0, 16);
  oled.print("MYOSA Event 5.0");

  oled.setCursor(0, 32);
  oled.print("Initializing...");

  oled.display();

  delay(2000);

  // =========================================================
  // Calibration
  // =========================================================

  showOLED(
    "Calibrating...",
    "Keep STILL!",
    "~6 seconds"
  );

  delay(500);

  calibrateBaseline();

  showOLED(
    "Ready!",
    "Adaptive SHM",
    "Active v3"
  );

  delay(1000);
}

// ============================================================
// OLED Helper
// ============================================================

void showOLED(
  const char* l1,
  const char* l2,
  const char* l3
) {

  oled.clearDisplay();

  oled.setTextSize(1);

  oled.setCursor(0, 0);
  oled.print(l1);

  oled.setCursor(0, 22);
  oled.print(l2);

  oled.setCursor(0, 44);
  oled.print(l3);

  oled.display();
}

// ============================================================
// Calibration
// ============================================================

void calibrateBaseline() {

  const int N = 20;

  float rS[N];
  float fS[N];
  float pS[N];

  float rSum = 0;
  float fSum = 0;
  float pSum = 0;

  for (int i = 0; i < N; i++) {

    VibData v = collectRaw();

    rS[i] = v.rms;

    fS[i] = v.freq;

    pS[i] =
      baro.getPressurePascal() / 100.0;

    rSum += rS[i];

    fSum += fS[i];

    pSum += pS[i];

    delay(150);
  }

  baseline_rms =
    rSum / N;

  baseline_freq =
    fSum / N;

  baseline_pressure =
    pSum / N;

  // =========================================================
  // Standard Deviations
  // =========================================================

  float rv = 0;
  float fv = 0;
  float pv = 0;

  for (int i = 0; i < N; i++) {

    rv += pow(
      rS[i] - baseline_rms,
      2
    );

    fv += pow(
      fS[i] - baseline_freq,
      2
    );

    pv += pow(
      pS[i] - baseline_pressure,
      2
    );
  }

  baseline_stddev =
    sqrt(rv / N);

  float freq_std =
    sqrt(fv / N);

  baseline_pstddev =
    sqrt(pv / N);

  // =========================================================
  // Adaptive Thresholds
  // =========================================================

  thresh_warn_rms =
    baseline_rms +
    K_FACTOR * baseline_stddev;

  thresh_crit_rms =
    baseline_rms +
    2.0 * K_FACTOR *
    baseline_stddev;

  thresh_freq_shift =
    K_FACTOR * freq_std;

  thresh_pressure =
    K_FACTOR * baseline_pstddev;

  // =========================================================
  // Initialize Moving Average
  // =========================================================

  for (int i = 0; i < MA_SIZE; i++) {

    ma_buf[i] = baseline_rms;
  }

  ma_sum =
    baseline_rms * MA_SIZE;

  calibrated = true;

  Serial.printf(
    "Baseline RMS=%.4f Freq=%.2fHz P=%.2fhPa\n",
    baseline_rms,
    baseline_freq,
    baseline_pressure
  );

  Serial.printf(
    "Thresholds: Warn=%.4f Crit=%.4f dF=%.2f dP=%.3f\n",
    thresh_warn_rms,
    thresh_crit_rms,
    thresh_freq_shift,
    thresh_pressure
  );
}

// ============================================================
// FFT Analysis
// ============================================================

VibData collectRaw() {

  for (int i = 0; i < SAMPLES; i++) {

    float ax =
      imu.getAccelX(false);

    float ay =
      imu.getAccelY(false);

    float az =
      imu.getAccelZ(false);

    // =======================================================
    // Magnitude in milli-g
    // Remove 1000mg gravity offset
    // =======================================================

    float mag =
      sqrt(
        ax * ax +
        ay * ay +
        az * az
      ) - 1000.0;

    vReal[i] = mag;

    vImag[i] = 0;

    delayMicroseconds(2000);
  }

  FFT.windowing(
    FFTWindow::Hamming,
    FFTDirection::Forward
  );

  FFT.compute(
    FFTDirection::Forward
  );

  FFT.complexToMagnitude();

  float sumSq = 0;

  float peakMag = 0;

  int peakIdx = 1;

  for (
    int i = 1;
    i < SAMPLES / 2;
    i++
  ) {

    sumSq +=
      vReal[i] * vReal[i];

    if (vReal[i] > peakMag) {

      peakMag = vReal[i];

      peakIdx = i;
    }
  }

  VibData out;

  out.rms =
    sqrt(sumSq / (SAMPLES / 2));

  out.freq =
    peakIdx *
    (SAMPLE_RATE / SAMPLES);

  return out;
}

// ============================================================
// Moving Average Filter
// ============================================================

float smoothRMS(float newVal) {

  ma_sum -= ma_buf[ma_idx];

  ma_buf[ma_idx] = newVal;

  ma_sum += ma_buf[ma_idx];

  ma_idx =
    (ma_idx + 1) % MA_SIZE;

  return ma_sum / MA_SIZE;
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  // =========================================================
  // Collect Sensor Data
  // =========================================================

  VibData raw =
    collectRaw();

  float rms =
    smoothRMS(raw.rms);

  domFreq =
    raw.freq;

  // =========================================================
  // Pressure
  // =========================================================

  float pressure =
    baro.getPressurePascal() / 100.0;

  float pressureDelta =
    abs(
      pressure -
      baseline_pressure
    );

  float freqShift =
    abs(
      domFreq -
      baseline_freq
    );

  // =========================================================
  // Anomaly Flags
  // =========================================================

  bool vibAnom =
    (rms > thresh_warn_rms);

  bool freqAnom =
    (freqShift > thresh_freq_shift);

  bool presAnom =
    (pressureDelta > thresh_pressure);

  // =========================================================
  // Alert Logic
  // =========================================================

  if (calibrated) {

    // CRITICAL
    if (
      rms > thresh_crit_rms
    ) {

      alertLevel = 2;
    }

    else if (
      vibAnom &&
      freqAnom &&
      rms >
      thresh_warn_rms * 1.2
    ) {

      alertLevel = 2;
    }

    // WARNING
    else if (
      vibAnom
    ) {

      alertLevel = 1;
    }

    // NORMAL
    else {

      alertLevel = 0;
    }
  }

  // =========================================================
  // OLED Display
  // =========================================================

  oled.clearDisplay();

  oled.setTextSize(1);

  oled.setCursor(0, 0);

  if (alertLevel == 0) {

    oled.print("STATUS: NORMAL");

  } else if (alertLevel == 1) {

    oled.print("STATUS: WARNING");

  } else {

    oled.print("!! CRITICAL !!");
  }

  oled.setCursor(0, 14);

  oled.print("V:");
  oled.print(rms, 2);

  oled.print(" F:");
  oled.print(domFreq, 1);
  oled.print("Hz");

  oled.setCursor(0, 28);

  oled.print("dF:");
  oled.print(freqShift, 1);

  oled.print(" dP:");
  oled.print(pressureDelta, 2);

  oled.setCursor(0, 42);

  oled.print("Alert:");
  oled.print(alertLevel);

  oled.print(" K:");
  oled.print(K_FACTOR, 1);

  oled.display();

  // =========================================================
  // Gesture Recalibration
  // =========================================================

static unsigned long lastGestureTime = 0;

char* g = gesture.getGesture(false);

if (
    g != nullptr &&
    strcmp(g, "LEFT") == 0 &&
    millis() - lastGestureTime > 5000
)
{
    lastGestureTime = millis();

    showOLED(
      "RECALIBRATING",
      "Keep still...",
      ""
    );

    delay(500);

    calibrateBaseline();

    showOLED(
      "Done!",
      "New baseline",
      "set."
    );

    delay(1000);
}
  // =========================================================
  // BLE Update
  // =========================================================

  unsigned long now = millis();

  if (
    now - lastBleUpdate >= 1500
  ) {

    myosa.sendBleData();

    lastBleUpdate = now;
  }

  delay(50);
}