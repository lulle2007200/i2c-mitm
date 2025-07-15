*** YOU PROBABLY DON'T WANT TO USE THIS ***

Originally, i made this  to monitor i2c communications. Can be used to override the i2c voltage change commands to the battery ic, but that alone won't be enough for hos to properly charge higher voltage batteries.

Allows overriding the battery charging voltage.
Config goes to `/config/i2c_mitm/i2c_mitm.ini` on SD card.

Config format:

```
[battery]
# voltage in mV, must be 3504-4400
chrg_voltage=4200
```
