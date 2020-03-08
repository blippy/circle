# TinyBASIC Plus


## Compiling custom QEMU

../configure --target-list=arm-softmmu CFLAGS="-Wno-error" 
make CFLAGS=-Wno-error

sudo apt install gcc-arm-none-eabi


cd /home/pi/Documents/qemu/builds/arm-softmmu
./qemu-system-arm -M raspi2 -bios  ~/pi3/Documents/circle/sample/08-usbkeyboard/kernel7.img -usbdevice keyboard



Config.mk:
```
# For QEMU
# https://www.raspberrypi.org/forums/viewtopic.php?f=72&t=90130&start=250
DEFINE += -DNO_PHYSICAL_COUNTER -DUSE_QEMU_USB_FIX
RASPPI=2

# As at 24-Feb-2020
# See include/circle/sysconfig.h
# Choices are DE (default), ES, FR, IT, UK, US 
#define DEFAULT_KEYMAP		"UK"
DEFINE += -DDEFAULT_KEYMAP=\"UK\"
```

## Links to other sites

* [CircleOS + BASIC](https://www.raspberrypi.org/forums/viewtopic.php?f=72&t=266203&p=1617466#p1617466)
* [Circle - C++ bare metal environment (with USB)](https://www.raspberrypi.org/forums/viewtopic.php?f=72&t=90130&start=250)

