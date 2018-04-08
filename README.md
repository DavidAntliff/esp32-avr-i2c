# ESP32 communication with AVR slave via I2C

Used to develop and test the [esp32-poolmon](https://github.com/DavidAntliff/esp32-poolmon) AVR operation.

Requires ESP IDF v3.0rc1.

## Dependencies

Requires [esp32-smbus](https://github.com/DavidAntliff/esp32-smbus) and [avr-poolmon](https://github.com/DavidAntliff/avr-poolmon). These are included as submodules:

`$ git update submodules --init`

## Registers

See [registers.h](https://github.com/DavidAntliff/avr-poolmon/blob/master/registers.h).

