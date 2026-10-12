# ESP32 Air Mouse

Wave your hand, move the cursor. An ESP32, an MPU6050 gyro and two buttons, acting as a Bluetooth mouse. No receiver, no app on the computer. It pairs like any normal BT mouse.

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

Edit the config block at the top of the sketch and re-upload:

- `X_AXIS` / `Y_AXIS`: which gyro axis drives the cursor (0 = X, 1 = Y, 2 = Z). Change these if the board is mounted differently.
- `X_SIGN` / `Y_SIGN`: set to `1` or `-1` to flip a direction.
- `SENS`: cursor speed. Higher is faster.
- `DEADZONE`: rotation below this (°/s) is ignored. Raise it if the cursor creeps.
- `SMOOTH`: 0 to 0.95. Reduces hand jitter, too high feels laggy.

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
