# Solar Box

A solar power system with a smart microcontroller to optimize battery usage.

## Hardware

1. MCU: esp32cam, https://a.co/d/a0T2Vgn
2. 30A relay with 12v power supply: https://www.amazon.com/NOYITO-1-Channel-Optocoupler-Automation-Industrial/dp/B07DSXHG3J/ref=asc_df_B07DSXHG3J/?tag=hyprod-20&linkCode=df0&hvadid=242041198988&hvpos=&hvnetw=g&hvrand=9129812423990448585&hvpone=&hvptwo=&hvqmt=&hvdev=m&hvdvcmdl=&hvlocint=&hvlocphy=9013314&hvtargid=pla-489507304060&psc=1
3. RS232 to TTL converter: https://www.amazon.com/NOYITO-Module-Conversion-Arduino-communicates/dp/B07BJJ3TZR/ref=mp_s_a_1_3?crid=11JDKKFTD1RUJ&keywords=noyito+rs232+to+ttl&qid=1660241875&sprefix=noyito+rs232+to+ttl%2Caps%2C159&sr=8-3
4. Custom rj12 cable compatible with Renogy wanderer

## Schematics

Soon


## Reference

- Cable selection: https://renogy.boards.net/thread/535/using-rj11-cable-connect-raspberry
- https://diysolarforum.com/threads/off-grid-solar-battery-monitoring-and-control-freeware.6662/page-11
- Arduino UART: https://www.circuitbasics.com/how-to-set-up-uart-communication-for-arduino/
- Arduino Modbus: https://github.com/4-20ma/ModbusMaster
- ESP32 Bluetooth Reference: https://github.com/nkolban/esp32-snippets/blob/master/Documentation/BLE%20C%2B%2B%20Guide.pdf
- ESP32 deep sleep: https://randomnerdtutorials.com/esp32-timer-wake-up-deep-sleep/
- ESP32 BLE server: https://www.electronicshub.org/esp32-ble-tutorial/
- BLE Battery Service: https://circuitdigest.com/microcontroller-projects/esp32-ble-server-how-to-use-gatt-services-for-battery-level-indication

## Issues

No /dev/ttyUSB0

logs from dmesg -w

```
3809.299963] cp210x 1-2:1.0: cp210x converter detected
[ 3809.301171] usb 1-2: cp210x converter now attached to ttyUSB0
[ 3810.296679] usb 1-2: usbfs: interface 0 claimed by cp210x while 'brltty' sets config #1
[ 3810.297680] cp210x ttyUSB0: cp210x converter now disconnected from ttyUSB0
[ 3810.297741] cp210x 1-2:1.0: device disconnected

```

```
systemctl stop brltty-udev.service
sudo systemctl mask brltty-udev.service
systemctl stop brltty.service
systemctl disable brltty.service
```
