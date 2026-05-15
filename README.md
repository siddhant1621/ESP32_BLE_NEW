# ESP32_BLE_NEW
# Bluetooth ADC Streaming — ESP32-C6 + React Native

## 🔍 Overview

This project implements a Bluetooth Low Energy (BLE) based ADC streaming system using an **ESP32-C6 board** and a **React Native mobile app**.

- The ESP32 samples an analog pin (floating) every 100 ms
- Every second, it bundles 10 samples into a JSON array
- The data is transmitted over BLE via a custom GATT characteristic
- A React Native mobile app connects to the ESP32, receives the bundle, and displays it in a table

---

## ⚙️ Embedded Code (ESP32-C6)

### 📂 Location:
    `embedded`
    - source code: main.c

### ✅ Features
- BLE device name: `ESP32_ADC_BLE`
- Samples ADC1 Channel 0 (GPIO4) every 100 ms
- Sends JSON array of 10 samples per second over BLE
- BLE UUIDs:
  - Service UUID: `0x181A`
  - Characteristic UUID: `0x2A58` (READ + NOTIFY)

---

### 🛠 Build & Flash Instructions (Using VS Code + ESP-IDF Extension)

> Make sure ESP-IDF extension is installed in VS Code and your environment is fully set up.

####  1. Create a new ESP-IDF project (if not already)
- Use VS Code’s ESP-IDF: **"Create project using template"**
- Or clone the basic `blink` example and replace `main.c`

####  2. Replace `main.c`
- Copy the `main.c` from `embedded/main.c` into:

####  3. Open VS Code and select ESP-IDF target
- Open your ESP32 project folder in VS Code
- Click bottom left ESP-IDF Target and select: `esp32c6`

####  4. Configure the project
- In VS Code Terminal, open menuconfig: 
        idf.py menuconfig

- OR use the VS Code Command Palette: `Ctrl + Shift + P → menuconfig`
- GOTO:
→ Component config → Bluetooth
    → NimBLE enabled ✅
    → Bluedroid disabled ❌

####  5. Connect the board via USB

####  6. Build the project

- idf.py build   
- OR use : `Ctrl + Shift + P → Build Your Project`

####  7. Flash the project

- idf.py -p COMx flash monitor (Replace COMx with your serial port)
- OR use: `Ctrl + Shift + P → Flash (UART) Your Project`

📝 Notes:
- GPIO4 is used as ADC input (ADC1_CH0), left floating intentionally for fluctuating random values
- nimble_port_init() sets up BLE with a custom GATT service
- adc_notify_task() handles 10-sample bundling and BLE notify every second

---

## 📱 Mobile App (React Native)

### 📂 Location:
- Mobile app folder: `mobile-app/`
- Source code: `App.tsx`

### ✅ Features
- Scans for BLE device named ESP32_ADC_BLE
- Connects and subscribes to the custom GATT characteristic
- Receives JSON array of 10 ADC values every second
- Displays the data in a scrollable table (dark theme)

### 🛠 Build & Run Instructions (Android)
####  1. Install Node & React Native CLI (if not already)
- In VS Code Terminal,
    npm install -g react-native-cli

####  2. Install project dependencies
- In VS Code Terminal, 
    cd mobile-app
    npm install

####  3. Enable Developer Mode & USB Debugging on your Android phone

####  4. Connect your Android device to PC via USB

####  5. Run the app
- In VS Code Terminal, 
    npx react-native run-android

- You should see the app launch on your phone. Click 'Connect to ESP32' and wait for live data to appear.

###  Optional: Use APK Without USB
You can build and install an APK directly:
1. Run `cd android && ./gradlew assembleDebug`
2. Find the APK in `android/app/build/outputs/apk/debug/app-debug.apk`
3. Transfer it to your phone and install manually


#### 📝 BLE Notes:
- BLE requires location & Bluetooth permissions on Android 12+
- These are automatically requested in the app using PermissionsAndroid
- BLE UUIDs must match the ESP32 side (181A and 2A58)

---

## 🎥 Demo Video

- The demo shows:
    - App launch
    - Connecting to ESP32 device
    - Real-time ADC data streaming into the table

### 🎬 Demo Video Link: https://drive.google.com/file/d/1V8BwpWJ83fjAObeUgW3flGT-jisa455L/view?usp=drive_link

---

## 🧾 Developer Notes
- Board Used: ESP32-C6 Zero
- ADC Pin: GPIO4 (ADC1 Channel 0), left floating
- Sampling: 10 samples per second, bundled every second
- BLE Format: JSON string array (e.g., `[322, 310, 307, ...]`)
- BLE GATT UUIDs:
  - Service: `0x181A`
  - Characteristic: `0x2A58` (READ + NOTIFY)
- Data Encoding: Base64 → JSON → Number[]

---

### 🛠 Technologies Used

| Layer            | Language      | Framework/Tool                                                              |
|------------------|---------------|-----------------------------------------------------------------------------|
| Embedded         | C             | ESP-IDF (v5+) with VS Code Extension                                        |
| Mobile App       | TypeScript    | React Native                                                                |
| BLE Library      | JavaScript/TS | [`react-native-ble-plx`](https://github.com/dotintent/react-native-ble-plx) |
| Data Decoding    | JavaScript    | [`react-native-base64`](https://www.npmjs.com/package/react-native-base64)  |
| IDEs Used        | -             | Visual Studio Code                                                          |
| Mobile Target    | -             | Android (via USB Debugging) (Sony Xperia)                                                 |

---

### 🤖 AI Assistant Support

- **ChatGPT (OpenAI GPT-4)** used for: 
    - Debugging BLE logic
    - Writing clean code
    - Refactoring and optimizing readability

---

## 📂 Folder Structure

        Bluetooth_ADC_Streaming/
        ├── embedded/
        │   └── main.c
        ├── mobile-app/
        │   ├── App.tsx
        │   ├── package.json
        │   ├── index.js
        │   ├── android/
        │   └── ...
        └── README.md
        
