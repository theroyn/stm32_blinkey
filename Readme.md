# STM32 Blinkey
##### Build
```
make
```

##### flash build results
```
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
-c "program build/firmware.elf verify exit"
```
##### debugging
terminal 1
```
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
```

terminal 2
```
gdb-multiarch build/firmware.elf
(gdb) target extended-remote :3333
(gdb) monitor reset halt
(gdb) load
(gdb) monitor reset halt
```
##### reset to factory demo program
```
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
-c "program factory_backup.bin verify reset exit 0x08000000"
```