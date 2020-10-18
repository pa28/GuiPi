# Automatically Start HamChrono

Or other programs for that matter.

If you are running HamChrono on a Raspberry Pi, directly on the
screen frame buffer, and you want it to start whenever the Pi
boots perform the following steps:

1. Setup the Pi to boot to command line.

1. Install HamChrono, possibly from my [repository](Repository.md).

1. Log into the account that will be running HamChrono (pi).

1. Make sure that account is in the video group:

```
$ groups
pi adm dialout cdrom sudo audio video plugdev games users input netdev gpio i2c spi
```

If not `usermod -a -G video pi`

Then edit the accounts `crontab` file:
```
crontab -e
```
and append these two lines:
```
@reboot sudo chgrp video /sys/class/backlight/rpi_backlight/brightness; sudo chmod g+w /sys/class/backlight/rpi_backlight/brightness
@reboot sleep 30; /usr/local/bin/hamchrono > ~/.hamchrono/log 2>&1
```
The first sets up backlight brighness control for members of the
`video` group. The second waits for 30 seconds to give WiFi and
other subsystems a chance to come up, then starts `hamchrono`.
