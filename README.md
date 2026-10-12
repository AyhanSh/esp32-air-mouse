# ESP32 Air Mouse

Wave your hand, move the cursor. An ESP32, an MPU6050 gyro and two buttons, acting as a Bluetooth mouse. No receiver, no app on the computer. It pairs like any normal BT mouse.

Comes with a browser tuner that shows the gyro live and lets you set axes, speed, deadzone and smoothing while the real cursor responds.

**Tuner:** https://ayhansh.github.io/esp32-air-mouse/tuner/

## Parts

- ESP32 DevKit (classic ESP32, 30-pin)
- MPU6050 module
- 2 push buttons
- Breadboard + jumpers

## Wiring

| From | To |
|---|---|
| MPU6050 VCC | 3V3 |
| MPU6050 GND | GND |
| MPU6050 SDA | D21 |
| MPU6050 SCL | D22 |
| Left button | D19 and GND |
| Right button | D18 and GND |

Buttons use the internal pullups, no resistors. Everything sits on one side of the DevKit, so it fits a single breadboard.

## Setup

1. **Board core:** Boards Manager, install **esp32 by Espressif** version **3.2.0**. Newer 3.3.x changed the BLE stack and the mouse crashes on pairing.
2. **Library:** install [ESP32-BLE-Mouse](https://github.com/T-vK/ESP32-BLE-Mouse) as a .zip.
3. **Patch the library.** It won't compile on core 3.x as is. In `BleMouse.cpp`:

   Line 143:
   ```cpp
   BLEDevice::init(String(bleMouseInstance->deviceName.c_str()));
   ```
   Line 151:
   ```cpp
   bleMouseInstance->hid->manufacturer()->setValue(String(bleMouseInstance->deviceManufacturer.c_str()));
   ```
4. Open `firmware/air_mouse/air_mouse.ino`, select **ESP32 Dev Module**, upload.
5. Pair **AirMouse2** from your computer's Bluetooth settings. On a Mac, hover over it in Nearby Devices to see the Connect button.

Keep the board still for a few seconds after connecting. It calibrates the gyro.

## Tuning

1. Plug the ESP32 in over USB. **Close the Arduino Serial Monitor**, only one app can hold the port.
2. Open the [tuner](https://ayhansh.github.io/esp32-air-mouse/tuner/) in Chrome or Edge (Safari has no Web Serial).
3. Connect, then hit **Auto-detect axes**. It asks you to push right, then up, and maps the axes for however your board is mounted.
4. Drag the sliders. Changes go to the board live.
5. **Copy config** and paste it over the config block at the top of the sketch. Re-upload to make it stick.

## Troubleshooting

- **Mac shows it but won't connect / spins forever.** Forget the device on every phone and computer it ever paired with. The ESP32 only takes one connection. The sketch also sets its own Bluetooth address, so changing the bytes in `newMac` gives you a fresh device the Mac has never seen.
- **Watchdog reset (`TG1WDT_SYS_RESET`) right after pairing.** You're on esp32 core 3.3.x. Go back to 3.2.0.
- **Connects but cursor doesn't move.** Check Serial for `MPU found at 0x68`. If missing, it's SDA/SCL wiring.
- **Cursor creeps when still.** Raise the deadzone or recalibrate.
- **`bad CPU type in executable` on Apple Silicon.** Run `softwareupdate --install-rosetta --agree-to-license`.

## How it works

Gyro only, no accelerometer tilt. Rotation speed maps to cursor speed, same idea as real air mice, and it feels a lot better than tilt. The MPU stays asleep until Bluetooth connects, because I2C traffic during pairing was knocking the connection over. Leftover fractions of a pixel carry to the next frame so slow precise moves still register. Buttons do press and release separately, so drag works.

## License

MIT
