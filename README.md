# no-OS-FatFS-SD-SDIO-SPI-RPi-Pico

## C/C++ Library for SD Cards on the Pico

At the heart of this library is ChaN's [FatFs - Generic FAT Filesystem Module](http://elm-chan.org/fsw/ff/00index_e.html).
It also contains a Serial Peripheral Interface (SPI) SD Card block driver for the [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/)
derived from [SDBlockDevice from Mbed OS 5](https://os.mbed.com/docs/mbed-os/v5.15/apis/sdblockdevice.html),
and a 4-bit Secure Digital Input Output (SDIO) driver derived from 
[ZuluSCSI-firmware](https://github.com/ZuluSCSI/ZuluSCSI-firmware). 
It is wrapped up in a complete runnable project, with a little command line interface, some self tests, and an example data logging application.

## Migration
### Migrating from no-OS-FatFS-SD-SPI-RPi-Pico [master](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/tree/master) branch
The object model for hardware configuration has changed.
If you are migrating a project from [no-OS-FatFS-SD-SPI-RPi-Pico](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico), you will have to change the hardware configuration customization. The `sd_card_t` now contains a new object that specifies the configuration of either an SPI interface or an SDIO interface. See the [Customizing for the Hardware Configuration](#customizing-for-the-hardware-configuration) section below.

For example, if you were using a `hw_config.c` containing 
```
static sd_card_t sd_cards[] = {  // One for each SD card
    {
        .pcName = "0:",   // Name used to mount device
        .spi = &spis[0],  // Pointer to the SPI driving this card
        .ss_gpio = 17,    // The SPI slave select GPIO for this SD card//...
```        
that would now become
```
static sd_spi_if_t spi_ifs[] = {
    { 
        .spi = &spis[0],          // Pointer to the SPI driving this card
        .ss_gpio = 17,             // The SPI slave select GPIO for this SD card
//...
static sd_card_t sd_cards[] = {  // One for each SD card
    {
        .pcName = "0:",           // Name used to mount device
        .type = SD_IF_SPI,
        .spi_if_p = &spi_ifs[0],  // Pointer to the SPI interface driving this card
```
### Migrating from no-OS-FatFS-SD-SPI-RPi-Pico [sdio](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/tree/sdio) branch
Instances of the interface classes `sd_spi_t` and `sd_sdio_t` are no longer embedded in `sd_card_t` as `spi_if` and `sdio_if`. They are moved to the top level as instances of `sd_spi_if_t` and `sd_sdio_if_t` and pointed to by instances of `sd_card_t`. For example, if you were using a `hw_config.c` containing:
```
static sd_card_t sd_cards[] = {  // One for each SD card
    {
        .pcName = "0:",  // Name used to mount device
        .type = SD_IF_SPI,
        .spi_if.spi = &spis[0],  // Pointer to the SPI driving this card
        .spi_if.ss_gpio = 7,     // The SPI slave select GPIO for this SD card
//...        
```
that would become:
```
static sd_spi_if_t spi_ifs[] = {
    {
        .spi = &spis[0],  // Pointer to the SPI driving this card
        .ss_gpio = 7,     // The SPI slave select GPIO for this SD card
//...
static sd_card_t sd_cards[] = {  // One for each SD card
    {
        .pcName = "0:",  // Name used to mount device
        .type = SD_IF_SPI,
        .spi_if_p = &spi_ifs[0],  // Pointer to the SPI interface driving this card     
//...           
```
For details, see the [Customizing for the Hardware Configuration](#customizing-for-the-hardware-configuration) section below. 

## Features:
* Supports multiple SD cards, all in a common file system
* Supports desktop compatible SD card formats
* Supports 4-bit wide SDIO by PIO, or SPI using built in SPI controllers, or both
* Supports multiple SPIs
* Supports multiple SD Cards per SPI
* Designed to support multiple SDIO buses
* Supports Real Time Clock for maintaining file and directory time stamps
* Supports Cyclic Redundancy Check (CRC)
* Plus all the neat features provided by [FatFS](http://elm-chan.org/fsw/ff/00index_e.html)

## Resources Used
* SPI attached cards:
  * One or two Serial Peripheral Interface (SPI) controllers may be used.
  * For each SPI controller used, two DMA channels are claimed with `dma_claim_unused_channel`.
  * A configurable DMA IRQ is hooked with `irq_add_shared_handler` and enabled.
  * For each SPI controller used, one GPIO is needed for each of RX, TX, and SCK. Note: each SPI controller can only use a limited set of GPIOs for these functions.
  * For each SD card attached to an SPI controller, a GPIO is needed for slave (or "chip") select (SS or CS), and, optionally, another for Card Detect (CD or "Det").
* SDIO attached card:
  * A PIO block
  * Two DMA channels claimed with `dma_claim_unused_channel`
  * A configurable DMA IRQ is hooked with `irq_add_shared_handler` and enabled.
  * Six GPIOs for signal pins, and, optionally, another for CD (Card Detect). Four pins must be at fixed offsets from D0 (which itself can be anywhere):
    * CLK_gpio = D0_gpio - 2.
    * D1_gpio = D0_gpio + 1;
    * D2_gpio = D0_gpio + 2;
    * D3_gpio = D0_gpio + 3;

SPI and SDIO can share the same DMA IRQ.

For the complete [examples/command_line](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main/examples/command_line) application, 
configured for one SPI attached card and one SDIO-attached card, release build, as reported by link flag `-Wl,--print-memory-usage`:
```
Memory region         Used Size  Region Size  %age Used
           FLASH:      167056 B         2 MB      7.97%
             RAM:       17524 B       256 KB      6.68%
```
A `MinSizeRel` build for a single SPI-attached card:
```
Memory region         Used Size  Region Size  %age Used
           FLASH:      124568 B         2 MB      5.94%
             RAM:       12084 B       256 KB      4.61%
```
A `MinSizeRel` build configured for three SPI-attached and one SDIO-attached SD cards:
```
Memory region         Used Size  Region Size  %age Used
           FLASH:      133704 B         2 MB      6.38%
             RAM:       21172 B       256 KB      8.08%
```
## Performance
Writing and reading a file of 200 MiB of psuedorandom data on the same Silicon Power 3D NAND U1 32GB microSD card, 
once on SPI and one on SDIO, `MinSizeRel` build,  using the command `big_file_test bf 200 2`:

* SPI:
  * Writing
    * Elapsed seconds 81.7
    * Transfer rate 2.45 MiB/s (2.57 MB/s), or 2507 KiB/s (2567 kB/s) (20539 kb/s)
  * Reading
    * Elapsed seconds 73.0
    * Transfer rate 2.74 MiB/s (2.87 MB/s), or 2804 KiB/s (2871 kB/s) (22967 kb/s)

* SDIO:
  * Writing
    * Elapsed seconds 28.1
    * Transfer rate 7.12 MiB/s (7.47 MB/s), or 7293 KiB/s (7468 kB/s) (59743 kb/s)
  * Reading
    * Elapsed seconds 16.3
    * Transfer rate 12.3 MiB/s (12.9 MB/s), or 12558 KiB/s (12860 kB/s) (102877 kb/s)

Results from a port of SdFat's `bench`:

SPI:
```
Card size: 31.95 GB (GB = 1E9 bytes)
...
FILE_SIZE_MB = 5
BUF_SIZE = 20480
...
write speed and latency
speed,max,min,avg
KB/Sec,usec,usec,usec
2064.1,114193,8013,9907
2379.9,43894,7991,8591
...
read speed and latency
speed,max,min,avg
KB/Sec,usec,usec,usec
2785.8,8137,7168,7351
2787.3,7967,7168,7350
...
```
SDIO:
```
...
write speed and latency
speed,max,min,avg
KB/Sec,usec,usec,usec
6448.8,7644,2613,3161
6472.7,6683,2612,3154
...
read speed and latency
speed,max,min,avg
KB/Sec,usec,usec,usec
11472.4,2370,1649,1786
11472.4,2183,1650,1784
...
```

## Choosing the Interface Type(s)
The main reason to use SDIO is for the much greater speed that the 4-bit wide interface gets you. 
However, you pay for that in pins. 
SPI can get by with four GPIOs for the first card and one more for each additional card.
SDIO needs at least six GPIOs, and the 4 bits of the data bus have to be on consecutive GPIOs.
It is possible to put more than one card on an SDIO bus (each card has an address in the protocol), but at the higher speeds (higher than this implementation can do) the tight timing requirements don't allow it. I haven't tried it.

One strategy: use SDIO for cache and SPI for backing store. 
A similar strategy that I have used: SDIO for fast, interactive use, and SPI to offload data.

## Notes about FreeRTOS
This library is not specifically designed for use with FreeRTOS. 
For use with FreeRTOS, I suggest you consider [FreeRTOS-FAT-CLI-for-RPi-Pico](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico) instead. That implementation is designed from the ground up for FreeRTOS.

While FatFS has some support for “re-entrancy” or thread safety (where "thread" == "task", in FreeRTOS terminology), it is limited to operations such as:

- f_read
- f_write
- f_sync
- f_close
- f_lseek
- f_closedir
- f_readdir
- f_truncate

There does not appear to be sufficient FAT and directory locking in FatFS to make operations like f_mkdir, f_chdir and f_getcwd thread safe. If your application has a static directory tree, it should be OK in a multi-tasking application with FatFS. Otherwise, you probably need to add some additional locking. 

Then, there are the facilities used for mutual exclusion and various ways of waiting (delay, wait for interrupt, etc.). This library uses the Pico SDK facilities (which are oriented towards multi-processing on the two cores), but FreeRTOS-FAT-CLI-for-RPi-Pico uses the FreeRTOS facilities, which put the waiting task into a Blocked state, allowing other, Ready tasks to run. 

FreeRTOS-FAT-CLI-for-RPi-Pico is designed to maximize parallelism. So, if you have two cores and multiple SD card buses (SPI or SDIO), multiple FreeRTOS tasks can keep them all busy simultaneously.

## Hardware
### My boards
* [Pico SD Card Development Board](https://forums.raspberrypi.com/viewtopic.php?p=2123146#p2123146)
![PXL_20230726_200951753a](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/assets/50121841/986ff919-e39e-40ef-adfb-78407f6e1e41)

* [Pico Stackable, Plug & Play SD Card Expansion Module](https://forums.raspberrypi.com/viewtopic.php?t=356864)
![PXL_20230926_212422091](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/assets/50121841/7edfea8c-59b0-491c-8321-45487bce9693)

### Prewired boards with SD card sockets:
There are a variety of RP2040 boards on the market that provide an integrated µSD socket. As far as I know, most are useable with this library.
* [Maker Pi Pico](https://www.cytron.io/p-maker-pi-pico) works on SPI1. Looks fine for 4-bit wide SDIO.
* I don't think the [Pimoroni Pico VGA Demo Base](https://shop.pimoroni.com/products/pimoroni-pico-vga-demo-base) can work with a built in RP2040 SPI controller. It looks like RP20040 SPI0 SCK needs to be on GPIO 2, 6, or 18 (pin 4, 9, or 24, respectively), but Pimoroni wired it to GPIO 5 (pin 7). SDIO? For sure it could work with one bit SDIO, but I don't know about 4-bit. It looks like it *can* work, depending on what other functions you need on the board.
* The [SparkFun RP2040 Thing Plus](https://learn.sparkfun.com/tutorials/rp2040-thing-plus-hookup-guide/hardware-overview) works well on SPI1. For SDIO, the data lines are consecutive, but in the reverse order! I think that it could be made to work, but you might have to do some bit twiddling. A downside to this board is that it's difficult to access the signal lines if you want to look at them with, say, a logic analyzer or an oscilloscope.
* [Challenger RP2040 SD/RTC](https://ilabs.se/challenger-rp2040-sd-rtc-datasheet/) looks usable for SPI only. 
* Here is one list of RP2040 boards: [earlephilhower/arduino-pico: Raspberry Pi Pico Arduino core, for all RP2040 boards](https://github.com/earlephilhower/arduino-pico) Only a fraction of them have an SD card socket.
  
### Rolling your own
Prerequisites:
* Raspberry Pi Pico
* Something like the [Adafruit Micro SD SPI or SDIO Card Breakout Board](https://www.adafruit.com/product/4682)[^3] or [SparkFun microSD Transflash Breakout](https://www.sparkfun.com/products/544)
* Breadboard and wires
* Raspberry Pi Pico C/C++ SDK
* (Optional) A couple of ~10 kΩ - 50 kΩ resistors for pull-ups
* (Optional) 100 nF, 1 µF, and 10 µF capacitors for decoupling
* (Optional) 22 µH inductor for decoupling

![image](https://www.raspberrypi.com/documentation/microcontrollers/images/pico-pinout.svg "Pinout")
<!--
|       | SPI0  | GPIO  | Pin   | SPI       | MicroSD 0 | Description            | 
| ----- | ----  | ----- | ---   | --------  | --------- | ---------------------- |
| MISO  | RX    | 16    | 21    | DO        | DO        | Master In, Slave Out   |
| CS0   | CSn   | 17    | 22    | SS or CS  | CS        | Slave (or Chip) Select |
| SCK   | SCK   | 18    | 24    | SCLK      | CLK       | SPI clock              |
| MOSI  | TX    | 19    | 25    | DI        | DI        | Master Out, Slave In   |
| CD    |       | 22    | 29    |           | CD        | Card Detect            |
| GND   |       |       | 18,23 |           | GND       | Ground                 |
| 3v3   |       |       | 36    |           | 3v3       | 3.3 volt power         |
-->

Please see [here](https://docs.google.com/spreadsheets/d/1BrzLWTyifongf_VQCc2IpJqXWtsrjmG7KnIbSBy-CPU/edit?usp=sharing) for an example wiring table for an SPI attached card and an SDIO attached card on the same Pico. SDIO is pretty demanding electrically. 
You need good, solid wiring, especially for grounds. A printed circuit board with a ground plane would be nice!

![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/PXL_20230201_232043568.jpg "Protoboard, top")
![image](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico/blob/master/images/PXL_20230201_232026240_3.jpg "Protoboard, bottom")


### Construction:
* The wiring is so simple that I didn't bother with a schematic. 
I just referred to the table above, wiring point-to-point from the Pin column on the Pico to the MicroSD 0 column on the Transflash.
* Card Detect is optional. Some SD card sockets have no provision for it. 
Even if it is provided by the hardware, if you have no requirement for it you can skip it and save a Pico I/O pin.
* You can choose to use none, either or both of the Pico's SPIs.
* You can choose to use zero or more PIO SDIO interfaces. [However, currently, the library has only been tested with zero or one.]
I don't know that there's much call for it.
* It's possible to put more than one card on an SDIO bus, but there is currently no support in this library for it.
* For SDIO, data lines D0 - D3 must be on consecutive GPIOs, with D0 being the lowest numbered GPIO.
Furthermore, the CMD signal must be on GPIO D0 GPIO number - 2, modulo 32. (This can be changed in the PIO code.)
* Wires should be kept short and direct. SPI operates at HF radio frequencies.

### Pull Up Resistors and other electrical considerations
* The SPI MISO (**DO** on SD card, **SPI**x **RX** on Pico) is open collector (or tristate). In the old MMC days, it was imperative to pull this up. The Pico internal gpio_pull_up is weak: around 56uA or 60kΩ. It's best to add an external pull up resistor of around 5-50 kΩ to 3.3v. However, modern SD cards use strong push pull tristateable outputs and don't seem to need this pull up. The internal gpio_pull_up can be disabled in the hardware configuration by setting the `no_miso_gpio_pull_up` attribute of the `spi_t` object.
On some SD cards, you can even configure the card's output drivers using the Driver Stage Register (DSR).[^4]).
* The SPI Slave Select (SS), or Chip Select (CS) line enables one SPI slave of possibly multiple slaves on the bus. This is what enables the tristate buffer for Data Out (DO), among other things. It's best to pull CS up so that it doesn't float before the Pico GPIO is initialized. It is imperative to pull it up for any devices on the bus that aren't initialized. For example, if you have two SD cards on one bus but the firmware is aware of only one card (see hw_config); you shouldn't let the CS float on the unused one. 
* Driving the SD card directly with the GPIOs is not ideal. Take a look at the CM1624 (https://www.onsemi.com/pdf/datasheet/cm1624-d.pdf). Unfortunately, it's a tiny little surface mount part -- not so easy to work with, but the schematic in the data sheet is still instructive. Besides the pull up resistors, it's a good idea to have 25 - 100 Ω series source termination resistors in each of the signal lines. This gives a cleaner signal, allowing higher baud rates. Even if you don't care about speed, it also helps to control the slew rate and current, which can reduce EMI and noise in general. (This can be important in audio applications, for example.) Ideally, the resistor should be as close as possible to the driving end of the line. That would be the Pico end for CS, SCK, MOSI, and the SD card end for MISO. For SDIO, the data lines are bidirectional, so, ideally, you'd have a source termination resistor at each end. Practically speaking, the clock is by far the most important to terminate, because each edge is significant. The other lines probably have time to bounce around before being clocked. 
* It can be helpful to add a decoupling capacitor or three (e.g., 100 nF, 1 µF, and 10 µF) between 3.3 V and GND on the SD card. ChaN also [recommends](http://elm-chan.org/docs/mmc/mmc_e.html#hotplug) putting a 22 µH inductor in series with the Vcc (or "Vdd") line to the SD card.
* Note: the [Adafruit Breakout Board](https://learn.adafruit.com/assets/93596) takes care of the pull ups and decoupling caps, but the Sparkfun one doesn't. And, you can never have too many decoupling caps.

## Notes about Card Detect
* There is one case in which Card Detect can be important: when the user can hot swap the physical card while the file system is mounted. In this case, the file system might have no way of knowing that the card was swapped, and so it will continue to assume that its prior knowledge of the FATs and directories is still valid. File system corruption and data loss are the likely results.
* If Card Detect is used, in order to detect a card swap there needs to be a way for the application to be made aware of a change in state when the card is removed. This could take the form of a GPIO interrupt (see [examples/command_line](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/blob/6c523f713ffa80dfeed2a61444a9ac58ac2bd1f8/examples/command_line/main.cpp#L652)), or polling.
* Some workarounds for absence of Card Detect:
  * Periodically poll sd_test_com() which can be called any time after sd_init_driver() is called. This function minimally accesses the bus to check for the presence of an SD card. The internals of this call automatically flags the SD interface for reinitialization when false is returned. If false is returned when the previous call returned true, it is important to invalidate any file handles that are still opened, unmount, and reset any mounted flags. Then don't try to remount until sd_test_com() returns true once again.
  * If you don't care much about performance or battery life, you could mount the card before each access and unmount it after. This might be a good strategy for a slow data logging application, for example.
  * Some other form of polling: if the card is periodically accessed at rate faster than the user can swap cards, then the temporary absence of a card will be noticed, so a swap will be detected. For example, if a data logging application writes a log record to the card once per second, it is unlikely that the user could swap cards between accesses.

## Firmware:
* Follow instructions in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf) to set up the development environment.
* Install source code:
  `git clone -b sdio --recurse-submodules git@github.com:carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico.git no-OS-FatFS`
* Customize:
  * Configure the code to match the hardware: see section [Customizing for the Hardware Configuration](#customizing-for-the-hardware-configuration), below.
  * Customize `ff14a/source/ffconf.h` as desired
  * Customize `pico_enable_stdio_uart` and `pico_enable_stdio_usb` in CMakeLists.txt as you prefer. 
(See *4.1. Serial input and output on Raspberry Pi Pico* in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf) and *2.7.1. Standard Input/Output (stdio) Support* in [Raspberry Pi Pico C/C++ SDK](https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf).) 
* Build:
```  
   cd no-OS-FatFS/examples/command_line
   mkdir build
   cd build
   cmake ..
   make
```   
  * Program the device
  
## Customizing for the Hardware Configuration 
This library can support many different hardware configurations. 
Therefore, the hardware configuration is not defined in the library[^1]. 
Instead, the application must provide it. 
The configuration is defined in "objects" of type `spi_t` (see `sd_driver/spi.h`), 
`sd_spi_if_t`, `sd_sdio_if_t`, and `sd_card_t` (see `sd_driver/sd_card.h`). 
* Instances of `sd_card_t` describe the configuration of SD card sockets.
* Each instance of `sd_card_t` is associated (one to one) with an `sd_spi_if_t` or `sd_sdio_if_t` interface object, 
and points to it with `spi_if_p` or `sdio_if_p`[^5].
* Instances of `sdio_if_p` specify the configuration of an SDIO/PIO interface.
* Each instance of `sd_spi_if_t` is assocated (many to one) with an instance of `spi_t` and points to it with `spi_t *spi`. (It is a many to one relationship because multiple SD cards can share a single SPI bus, as long as each has a unique slave (or "chip") select (SS, or "CS") line.) It describes the configuration of a specific SD card's interface to a specific SPI hardware component.
* Instances of `spi_t` describe the configuration of the RP2040 SPI hardware components used.
There can be one or more objects of all three types.
Attributes (or "fields", or "members") of these objects specify which pins to use for what, baud rates, features like Card Detect, etc.
* Generally, anything not specified will default to 0 or false. (This is the user's responsibility if using Dynamic Configuration, but in a Static Configuration [see [Static vs. Dynamic Configuration](#static-vs-dynamic-configuration)], below), the C runtime initializes static memory to 0.)

![Illustration of the configuration dev_brd.hw_config.c](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/assets/50121841/0eedadea-f6cf-44cb-9b76-544ec74287d2)

Illustration of the configuration [dev_brd.hw_config.c](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/blob/main/examples/command_line/dev_brd.hw_config.c)

### An instance of `sd_card_t` describes the configuration of one SD card socket.
```
struct sd_card_t {
    const char *pcName;
    sd_if_t type;
    union {
        sd_spi_if_t *spi_if_p;
        sd_sdio_if_t *sdio_if_p;
    };
    bool use_card_detect;
    uint card_detect_gpio;    // Card detect; ignored if !use_card_detect
    uint card_detected_true;  // Varies with card socket; ignored if !use_card_detect
    bool card_detect_use_pull;
    bool card_detect_pull_hi;
//...
}
```
* `pcName` FatFs [Logical Drive](http://elm-chan.org/fsw/ff/doc/filename.html) name (or "number"). In general, this is "0:", "1:", "2:", ... where the number is the index that gets that particular `sd_card_t` object when `sd_card_t *sd_get_by_num(size_t num)` is called. In a static, `hw_config.c` kind of configuration, that would be the (zero origin indexed) position in the `sd_cards[]` array. However, this can be changed with [FF_STR_VOLUME_ID](http://elm-chan.org/fsw/ff/doc/config.html#str_volume_id).
* `type` Type of interface: either `SD_IF_SPI` or `SD_IF_SDIO`
* `use_card_detect` Whether or not to use Card Detect, meaning the hardware switch featured on some SD card sockets. This requires a GPIO pin.
* `card_detect_gpio` GPIO number of the Card Detect, connected to the SD card socket's Card Detect switch (sometimes marked DET)
* `card_detected_true` Ignored if not `use_card_detect`. What the GPIO read returns when a card is present (Some sockets use active high, some low)
* `card_detect_use_pull` Ignored if not `use_card_detect`. Otherwise, if true, use the `card_detect_gpio`'s pad's Pull Up / Pull Down resistors; 
if false, no pull resistor is applied.
* `card_detect_pull_hi` Ignored if not `use_card_detect`. Ignored if not `card_detect_use_pull`. Otherwise, if true, pull up; if false, pull down.

### An instance of `sd_sdio_if_t` describes the configuration of one SDIO to SD card interface.
```
struct sd_sdio_if_t {
    // See sd_driver\SDIO\rp2040_sdio.pio for SDIO_CLK_PIN_D0_OFFSET
    uint CLK_gpio;  // Must be (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32
    uint CMD_gpio;
    uint D0_gpio;      // D0
    uint D1_gpio;      // Must be D0 + 1
    uint D2_gpio;      // Must be D0 + 2
    uint D3_gpio;      // Must be D0 + 3
    PIO SDIO_PIO;      // either pio0 or pio1
    uint DMA_IRQ_num;  // DMA_IRQ_0 or DMA_IRQ_1
    bool use_exclusive_DMA_IRQ_handler;
    uint baud_rate;
    // Drive strength levels for GPIO outputs:
    // GPIO_DRIVE_STRENGTH_2MA 
    // GPIO_DRIVE_STRENGTH_4MA
    // GPIO_DRIVE_STRENGTH_8MA 
    // GPIO_DRIVE_STRENGTH_12MA
    bool set_drive_strength;
    enum gpio_drive_strength CLK_gpio_drive_strength;
    enum gpio_drive_strength CMD_gpio_drive_strength;
    enum gpio_drive_strength D0_gpio_drive_strength;
    enum gpio_drive_strength D1_gpio_drive_strength;
    enum gpio_drive_strength D2_gpio_drive_strength;
    enum gpio_drive_strength D3_gpio_drive_strength;
//...
} sd_sdio_t;
```
Pins `CLK_gpio`, `D1_gpio`, `D2_gpio`, and `D3_gpio` are at offsets from pin `D0_gpio`.
The offsets are determined by `sd_driver\SDIO\rp2040_sdio.pio`.
  As of this writing, `SDIO_CLK_PIN_D0_OFFSET` is 30,
    which is -2 in mod32 arithmetic, so:
    
  * CLK_gpio = D0_gpio - 2
  * D1_gpio = D0_gpio + 1
  * D2_gpio = D0_gpio + 2
  * D3_gpio = D0_gpio + 3 

These pin assignments are set implicitly and must not be set explicitly.
* `CLK_gpio` RP2040 GPIO to use for Clock (CLK).
Implicitly set to `(D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32` where `SDIO_CLK_PIN_D0_OFFSET` is defined in `sd_driver/SDIO/rp2040_sdio.pio`.
As of this writing, `SDIO_CLK_PIN_D0_OFFSET` is 30, which is -2 in mod32 arithmetic, so:
  * CLK_gpio = D0_gpio - 2
* `CMD_gpio` RP2040 GPIO to use for Command/Response (CMD)
* `D0_gpio` RP2040 GPIO to use for Data Line [Bit 0]. The PIO code requires D0 - D3 to be on consecutive GPIOs, with D0 being the lowest numbered GPIO.
* `D1_gpio` RP2040 GPIO to use for Data Line [Bit 1]. Implicitly set to D0_gpio + 1.
* `D2_gpio` RP2040 GPIO to use for Data Line [Bit 2]. Implicitly set to D0_gpio + 2.
* `D3_gpio` RP2040 GPIO to use for Card Detect/Data Line [Bit 3]. Implicitly set to D0_gpio + 3.
* `SDIO_PIO` Which PIO block to use. Defaults to `pio0`. Can be changed to avoid conflicts.
* `DMA_IRQ_num` Which IRQ to use for DMA. Defaults to DMA_IRQ_0. Set this to avoid conflicts with any exclusive DMA IRQ handlers that might be elsewhere in the system.
* `use_exclusive_DMA_IRQ_handler` If true, the IRQ handler is added with the SDK's `irq_set_exclusive_handler`. The default is to add the handler with `irq_add_shared_handler`, so it's not exclusive. 
* `baud_rate` The frequency of the SDIO clock in Hertz. This may be no higher than the system clock frequency divided by `CLKDIV` in `sd_driver\SDIO\rp2040_sdio.pio`, which is currently four. For example, if the system clock frequency is 125 MHz, `baud_rate` cannot exceed 31250000 (31.25 MHz). The default is 10 MHz.
* `set_drive_strength` If true, enable explicit specification of output drive strengths on `CLK_gpio`, `CMD_gpio`, and `D0_gpio` - `D3_gpio`. 
The GPIOs on RP2040 have four different output drive strengths, which are nominally called 2, 4, 8 and 12mA modes.
If `set_drive_strength` is false, all will be implicitly set to 4 mA.
If `set_drive_strength` is true, each GPIO's drive strength can be set individually. Note that if it is not explicitly set, it will default to 0, which equates to `GPIO_DRIVE_STRENGTH_2MA` (2 mA nominal drive strength).

* ```
  CLK_gpio_drive_strength
  CMD_gpio_drive_strength 
  D0_gpio_drive_strength 
  D1_gpio_drive_strength 
  D2_gpio_drive_strength 
  D3_gpio_drive_strength 
  ``` 
  Ignored if `set_drive_strength` is false. Otherwise, these can be set to one of the following:
  ```
  GPIO_DRIVE_STRENGTH_2MA 
  GPIO_DRIVE_STRENGTH_4MA
  GPIO_DRIVE_STRENGTH_8MA 
  GPIO_DRIVE_STRENGTH_12MA
  ```
  You might want to do this for electrical tuning. A low drive strength can give a cleaner signal, with less overshoot and undershoot. 
  In some cases, this allows operation at higher baud rates.
  In other cases, the signal lines might have a lot of capacitance to overcome.
  Then, a higher drive strength might allow operation at higher baud rates.
  A low drive strength generates less noise. This might be important in, say, audio applications.

### An instance of `sd_spi_if_t` describes the configuration of one SPI to SD card interface.
```
struct sd_spi_if_t {
    spi_t *spi;
    // Slave select is here instead of in spi_t because multiple SDs can share an SPI.
    uint ss_gpio;                   // Slave select for this SD card
    // Drive strength levels for GPIO outputs:
    // GPIO_DRIVE_STRENGTH_2MA 
    // GPIO_DRIVE_STRENGTH_4MA
    // GPIO_DRIVE_STRENGTH_8MA 
    // GPIO_DRIVE_STRENGTH_12MA
    bool set_drive_strength;
    enum gpio_drive_strength ss_gpio_drive_strength;
} sd_spi_if_t;
```
* `spi` Points to the instance of `spi_t` that is to be used as the SPI to drive the interface for this card
* `ss_gpio` Slave Select (SS) (or Chip Select [CS]) GPIO for this SD card socket
* `set_drive_strength` Enable explicit specification of output drive strength of `ss_gpio_drive_strength`. 
If false, the GPIO's drive strength will be implicitly set to 4 mA.
* `ss_gpio_drive_strength` Drive strength for the SS (or CS).
  Ignored if `set_drive_strength` is false. Otherwise, it can be set to one of the following:
  ```
  GPIO_DRIVE_STRENGTH_2MA 
  GPIO_DRIVE_STRENGTH_4MA
  GPIO_DRIVE_STRENGTH_8MA 
  GPIO_DRIVE_STRENGTH_12MA
  ```
### An instance of `spi_t` describes the configuration of one RP2040 SPI controller.
```
struct spi_t {   
    spi_inst_t *hw_inst;  // SPI HW
    uint miso_gpio;  // SPI MISO GPIO number (not pin number)
    uint mosi_gpio;
    uint sck_gpio;
    uint baud_rate;
    uint DMA_IRQ_num; // DMA_IRQ_0 or DMA_IRQ_1
    bool use_exclusive_DMA_IRQ_handler;
    bool no_miso_gpio_pull_up;

    /* Drive strength levels for GPIO outputs.
        GPIO_DRIVE_STRENGTH_2MA, 
        GPIO_DRIVE_STRENGTH_4MA, 
        GPIO_DRIVE_STRENGTH_8MA,
        GPIO_DRIVE_STRENGTH_12MA
    */
    bool set_drive_strength;
    enum gpio_drive_strength mosi_gpio_drive_strength;
    enum gpio_drive_strength sck_gpio_drive_strength;

    // State variables:
// ...
```
* `hw_inst` Identifier for the hardware SPI instance (for use in SPI functions). e.g. `spi0`, `spi1`, declared in `pico-sdk\src\rp2_common\hardware_spi\include\hardware\spi.h`
* `miso_gpio` SPI Master In, Slave Out (MISO) GPIO number (not Pico pin number). This is connected to the card's Data In (DI).
* `mosi_gpio` SPI Master Out, Slave In (MOSI) GPIO number. This is connected to the card's Data Out (DO).
* `sck_gpio` SPI Serial Clock GPIO number. This is connected to the card's Serial Clock (SCK).
* `baud_rate` Frequency of the SPI Serial Clock, in Hertz. The default is 10 MHz.
* `set_drive_strength` Specifies whether or not to set the RP2040 GPIO drive strength. 
If `set_drive_strength` is false, all will be implicitly set to 4 mA. 
If `set_drive_strength` is true, each GPIO's drive strength can be set individually. Note that if it is not explicitly set, it will default to 0, which equates to `GPIO_DRIVE_STRENGTH_2MA` (2 mA nominal drive strength).
* `mosi_gpio_drive_strength` SPI Master Out, Slave In (MOSI) drive strength, 
* and `sck_gpio_drive_strength` SPI Serial Clock (SCK) drive strength.
  Ignored if `set_drive_strength` is false. Otherwise, these can be set to one of the following:
  ```
  GPIO_DRIVE_STRENGTH_2MA 
  GPIO_DRIVE_STRENGTH_4MA
  GPIO_DRIVE_STRENGTH_8MA 
  GPIO_DRIVE_STRENGTH_12MA
  ```
  You might want to do this for electrical tuning. A low drive strength can give a cleaner signal, with less overshoot and undershoot. 
  In some cases, this allows operation at higher baud rates.
  In other cases, the signal lines might have a lot of capacitance to overcome.
  Then, a higher drive strength might allow operation at higher baud rates.
  A low drive strength generates less noise. This might be important in, say, audio applications.

* `DMA_IRQ_num` Which IRQ to use for DMA. Defaults to DMA_IRQ_0. Set this to avoid conflicts with any exclusive DMA IRQ handlers that might be elsewhere in the system.
* `use_exclusive_DMA_IRQ_handler` If true, the IRQ handler is added with SDK's `irq_set_exclusive_handler`. The default is to add the handler with `irq_add_shared_handler`, so it's not exclusive. 
* `no_miso_gpio_pull_up` According to the standard, an SD card's DO MUST be pulled up (at least for the old MMC cards). 
However, it might be done externally. If `no_miso_gpio_pull_up` is false, the library will set the RP2040 GPIO internal pull up.

### You must provide a definition for the functions declared in `sd_driver/hw_config.h`:  
`size_t sd_get_num()` Returns the number of SD cards  
`sd_card_t *sd_get_by_num(size_t num)` Returns a pointer to the SD card "object" at the given (zero origin) index.  

### Static vs. Dynamic Configuration
The definition of the hardware configuration can either be built in at build time, which I'm calling "static configuration", or supplied at run time, which I call "dynamic configuration". 
In either case, the application simply provides an implementation of the functions declared in `sd_driver/hw_config.h`. 
* See `simple_example.dir/hw_config.c` or `example/hw_config.c` for examples of static configuration.
* See `dynamic_config_example/hw_config.cpp` for an example of dynamic configuration.
* One advantage of static configuration is that the fantastic GNU Linker (ld) strips out anything that you don't use.

## Using the Application Programming Interface
After `stdio_init_all()`, `time_init()`, and whatever other Pico SDK initialization is required, 
you may call `sd_init_driver()` to initialize the block device driver. `sd_init_driver()` is now[^2] called implicitly by `disk_initialize`, but you might want to call it sooner so that the GPIOs get configured, e.g., if you want to set up a Card Detect interrupt.
* Now, you can start using the [FatFs Application Interface](http://elm-chan.org/fsw/ff/00index_e.html). Typically,
  * f_mount - Register/Unregister the work area of the volume
  * f_open - Open/Create a file
  * f_write - Write data to the file
  * f_read - Read data from the file
  * f_close - Close an open file
  * f_unmount
* There is a simple example in the `simple_example` subdirectory.
* There is also POSIX-like API wrapper layer in `ff_stdio.h` and `ff_stdio.c`, written for compatibility with [FreeRTOS+FAT API](https://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_FAT/index.html) (mainly so that I could reuse some tests from that environment.)

### Messages
Sometimes problems arise when attempting to use SD cards. At the [FatFs Application Interface](http://elm-chan.org/fsw/ff/00index_e.html) level, it can be difficult to diagnose problems. You get a [return code](http://elm-chan.org/fsw/ff/doc/rc.html), but it might just tell you `FR_NOT_READY` ("The physical drive cannot work"), for example, without telling you what you need to know in order to fix the problem. The library generates messages that might help. These are classed into Error, Informational, and Debug messages. 

Two compile definitions control how these are handled:
* `USE_PRINTF` If this is defined and not zero, 
these message output functions will use the Pico SDK's stdout.
* `USE_DBG_PRINTF` If this is not defined or is zero or `NDEBUG` is defined, 
`DBG_PRINTF` statements will be effectively stripped from the code.

Messages are sent using `EMSG_PRINTF`, `IMSG_PRINTF`, and `DBG_PRINTF` macros, which can be redefined (see [my_debug.h](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/blob/main/src/include/my_debug.h)). By default, these call `error_message_printf`, `info_message_printf`, and `debug_message_printf`, which are implemented as [weak](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html) functions, meaning that they can be overridden by strongly implementing them in user code. If `USE_PRINTF` is defined and not zero, the weak implementations will write to the Pico SDK's stdout. Otherwise, they will format the messages into strings and forward to `put_out_error_message`, `put_out_info_message`, and `put_out_debug_message`. These are implemented as weak functions that do nothing. You can override these to send the output somewhere.


## C++ Wrapper
At heart, this is a C library, but I have made a (thin) C++ wrapper for it: `include\FatFsSd.h`.
For details on most of the functions, refer to [FatFs - Generic FAT Filesystem Module, "Application Interface"](http://elm-chan.org/fsw/ff/00index_e.html).

See `examples\PlatformIO\one_SPI.C++\src\main.cpp` for an example of the use of this API.

### namespace FatFsNs
The C++ API is in namespace `FatFsNs`, to avoid name clashes with other packages (e.g.: SdFat).

### class FatFs
This is a pure static class that represents the global file system as a whole. 
It stores the hardware configuration objects internally, 
which is useful in Arduino-like environments where you have a `setup` and a `loop`.
The objects can be constructed in the `setup` and added to `FatFs` and the copies won't go out of scope
when control leaves `setup` and goes to `loop`. 
It automatically provides these functions required internally by the library:

* `size_t sd_get_num()` Returns the number of SD cards  
* `sd_card_t *sd_get_by_num(size_t num)` Returns a pointer to the SD card "object" at the given (zero origin) index.  

Static Public Member Functions:
* `static SdCard* add_sd_card(SdCard& SdCard)` Use this to add an instance of `SdCard`or `sd_card_t` to the configuration. Returns a pointer that can be used to access the newly created SDCard object.
* `static size_t        SdCard_get_num ()` Get the number of SD cards in the configutation
* `static SdCard *SdCard_get_by_num (size_t num)` Get pointer to an `SdCard` by (zero-origin) index
* `static SdCard *SdCard_get_by_name (const char *const name)` Get pointer to an `SdCard` by logical drive identifier
* `static FRESULT       chdrive (const TCHAR *path)` Change current drive to given logical drive
* `static FRESULT       setcp (WORD cp)` Set current code page
* `static bool  begin ()` Initialize driver and all SD cards in configuration

### class SdCard
Represents an SD card socket. It is generalized: the SD card can be either SPI or SDIO attached.

Public Member Functions:
* `const char * get_name ()` Get the the FatFs [logical drive](http://elm-chan.org/fsw/ff/doc/filename.html#vol) identifier.
* `FRESULT      mount ()` Mount SD card
* `FRESULT      unmount ()` Unmount SD card
* `FRESULT      format ()` Create a FAT volume with defaults
* `FATFS *      fatfs ()` Get filesystem object structure (FATFS)
* `uint64_t     get_num_sectors ()` Get number of blocks on the drive
* `void cidDmp(printer_t printer)` Print information from Card IDendtification register
* `void csdDmp(printer_t printer)` Print information from Card-Specific Data register

Static Public Member Functions
* `static FRESULT mkfs (const TCHAR* path, const MKFS_PARM* opt, void* work, UINT len) Create a FAT volume
* `static FRESULT fdisk (BYTE pdrv, const LBA_t ptbl[], void* work)` Divide a physical drive into some partitions
* `static FRESULT getfree (const TCHAR *path, DWORD *nclst, FATFS **fatfs)` Get number of free blocks on the drive
* `static FRESULT getlabel (const TCHAR *path, TCHAR *label, DWORD *vsn)` Get volume label
* `static FRESULT setlabel (const TCHAR *label)` Set volume label

### class File

 * `FRESULT      open (const TCHAR *path, BYTE mode)` Open or create a file 
 * `FRESULT      close ()` Close an open file object
 * `FRESULT      read (void *buff, UINT btr, UINT *br)` Read data from the file
 * `FRESULT      write (const void *buff, UINT btw, UINT *bw)` Write data to the file 
 * `FRESULT      lseek (FSIZE_t ofs)` Move file pointer of the file object
 * `FRESULT      expand (uint64_t file_size)` Prepare or allocate a contiguous data area to the file with default option
 * `FRESULT      truncate ()` Truncate the file 
 * `FRESULT      sync ()` Flush cached data of the writing file
 * `int  putc (TCHAR c)` Put a character to the file 
 * `int  puts (const TCHAR *str)` Put a string to the file
 * `int  printf (const TCHAR *str,...)` Put a formatted string to the file
 * `TCHAR *      gets (TCHAR *buff, int len)` Get a string from the file
 * `bool         eof ()` Test for end-of-file
 * `BYTE         error ()` Test for an error
 * `FSIZE_t      tell ()` Get current read/write pointer
 * `FSIZE_t      size ()` Get size in bytes
 * `FRESULT      rewind ()` Move the file read/write pointer to 0 (beginning of file)
 * `FRESULT      forward (UINT(*func)(const BYTE *, UINT), UINT btf, UINT *bf)` Forward data to the stream
 * `FRESULT      expand (FSIZE_t fsz, BYTE opt)` Prepare or allocate a contiguous data area to the file

### class Dir 

Public Member Functions:
* `FRESULT      rewinddir ()` Rewind the read index of the directory object 
* `FRESULT      rmdir (const TCHAR *path)` Remove a sub-directory
* `FRESULT      opendir (const TCHAR *path)` Open a directory
* `FRESULT      closedir ()` Close an open directory
* `FRESULT      readdir (FILINFO *fno)` Read a directory item
* `FRESULT      findfirst (FILINFO *fno, const TCHAR *path, const TCHAR *pattern)` Open a directory and read the first item matched
* `FRESULT      findnext (FILINFO *fno)` Read a next item matched
Static Public Member Functions:
* `static FRESULT       mkdir (const TCHAR *path)` Create a sub-directory
* `static FRESULT       unlink (const TCHAR *path)` Remove a file or sub-directory
* `static FRESULT       rename (const TCHAR *path_old, const TCHAR *path_new)` Rename/Move a file or sub-directory
* `static FRESULT       stat (const TCHAR *path, FILINFO *fno)` Check existance of a file or sub-directory
* `static FRESULT       chmod (const TCHAR *path, BYTE attr, BYTE mask)` Change attribute of a file or sub-directory
* `static FRESULT       utime (const TCHAR *path, const FILINFO *fno)` Change timestamp of a file or sub-directory
* `static FRESULT       chdir (const TCHAR *path)` Change current directory
* `static FRESULT       chdrive (const TCHAR *path)` Change current drive
* `static FRESULT       getcwd (TCHAR *buff, UINT len)` Retrieve the current directory and drive

## PlatformIO Libary
This library is available at https://registry.platformio.org/libraries/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.
It is currently running with 
```
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
board_build.core = earlephilhower
```

## Next Steps
* There is a example data logging application in `data_log_demo.c`. 
It can be launched from the `no-OS-FatFS/example` CLI with the `start_logger` command.
(Stop it with the `stop_logger` command.)
It records the temperature as reported by the RP2040 internal Temperature Sensor once per second 
in files named something like `/data/2021-03-21/11.csv`.
Use this as a starting point for your own data logging application!

* If you want to use no-OS-FatFS-SD-SPI-RPi-Pico as a library embedded in another project, use something like:
  ```
  git submodule add -b sdio git@github.com:carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.git
  ```
  or
  ```
  git submodule add -b sdio https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico.git
  ```
  
You might also need to pick up the library in CMakeLists.txt:
```
add_subdirectory(no-OS-FatFS-SD-SPI-RPi-Pico/src build)
target_link_libraries(_my_app_ src)
```
and `#include "ff.h"`.

Happy hacking!

## Future Directions
You are welcome to contribute to this project! Just submit a Pull Request. Here are some ideas for future enhancements:
* Disable the card's internal card detect pull-up resistor with ACMD42
* Port SDIO driver to [FreeRTOS-FAT-CLI-for-RPi-Pico](https://github.com/carlk3/FreeRTOS-FAT-CLI-for-RPi-Pico)
* Battery saving: at least stop the SDIO clock when it is not needed
* Support 1-bit SDIO
* Try multiple cards on a single SDIO bus
* Multiple SDIO buses?
* ~~PlatformIO library~~ Done: See https://registry.platformio.org/libraries/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico
* [RP2040: Enable up to 42 MHz SDIO bus speed](https://github.com/ZuluSCSI/ZuluSCSI-firmware/tree/rp2040_highspeed_sdio)
* SD UHS Double Data Rate (DDR): clock data on both edges of the clock

![image](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/assets/50121841/8a28782e-84c4-40c8-8757-a063a4b83292)

## Appendix A: Adding Additional Cards
When you're dealing with information storage, it's always nice to have redundancy. There are many possible combinations of SPIs and SD cards. One of these is putting multiple SD cards on the same SPI bus, at a cost of one (or two) additional Pico I/O pins (depending on whether or you care about Card Detect). I will illustrate that example here. 

To add a second SD card on the same SPI, connect it in parallel, except that it will need a unique GPIO for the Card Select/Slave Select (CSn) and another for Card Detect (CD) (optional).

Name|SPI0|GPIO|Pin |SPI|SDIO|MicroSD 0|MicroSD 1
----|----|----|----|---|----|---------|---------
CD1||14|19||||CD
CS1||15|20|SS or CS|DAT3||CS
MISO|RX|16|21|DO|DAT0|DO|DO
CS0||17|22|SS or CS|DAT3|CS|
SCK|SCK|18|24|SCLK|CLK|SCK|SCK
MOSI|TX|19|25|DI|CMD|DI|DI
CD0||22|29|||CD|
|||||||
GND|||18, 23|||GND|GND
3v3|||36|||3v3|3v3

### Wiring: 
As you can see from the table above, the only new signals are CD1 and CS1. Otherwise, the new card is wired in parallel with the first card.
### Firmware:
* `hw_config.c` (or equivalent) must be edited to add a new instance to `static sd_card_t sd_cards[]`
* Edit `ff14a/source/ffconf.h`. In particular, `FF_VOLUMES`:
```
#define FF_VOLUMES		2
```

## Appendix B: Operation of `no-OS-FatFS/example`:
* Connect a terminal. [PuTTY](https://www.putty.org/) or `tio` work OK. For example:
  * `tio -m ODELBS /dev/ttyACM0`
* Press Enter to start the CLI. You should see a prompt like:
```
    > 
```    
* The `help` command describes the available commands:
```    
setrtc <DD> <MM> <YY> <hh> <mm> <ss>:
  Set Real Time Clock
  Parameters: new date (DD MM YY) new time in 24-hour format (hh mm ss)
        e.g.:setrtc 16 3 21 0 4 0

date:
 Print current date and time

lliot <drive#>:
 !DESTRUCTIVE! Low Level I/O Driver Test
        e.g.: lliot 1

format [<drive#:>]:
  Creates an FAT/exFAT volume on the logical drive.
        e.g.: format 0:

mount [<drive#:>]:
  Register the work area of the volume
        e.g.: mount 0:

unmount <drive#:>:
  Unregister the work area of the volume

chdrive <drive#:>:
  Changes the current directory of the logical drive.
  <path> Specifies the directory to be set as current directory.
        e.g.: chdrive 1:

getfree [<drive#:>]:
  Print the free space on drive

cd <path>:
  Changes the current directory of the logical drive.
  <path> Specifies the directory to be set as current directory.
        e.g.: cd /dir1

mkdir <path>:
  Make a new directory.
  <path> Specifies the name of the directory to be created.
        e.g.: mkdir /dir1

del_node <path>:
  Remove directory and all of its contents.
  <path> Specifies the name of the directory to be deleted.
        e.g.: del_node /dir1

ls:
  List directory

cat <filename>:
  Type file contents

simple:
  Run simple FS tests

bench [<drive#:>]:
  A simple binary write/read benchmark

big_file_test <pathname> <size in MiB> <seed>:
 Writes random data to file <pathname>.
 Specify <size in MiB> in units of mebibytes (2^20, or 1024*1024 bytes)
        e.g.: big_file_test bf 1 1
        or: big_file_test big3G-3 3072 3

cdef:
  Create Disk and Example Files
  Expects card to be already formatted and mounted

swcwdt:
 Stdio With CWD Test
Expects card to be already formatted and mounted.
Note: run cdef first!

loop_swcwdt:
 Run Create Disk and Example Files and Stdio With CWD Test in a loop.
Expects card to be already formatted and mounted.
Note: Type any key to quit.

start_logger:
  Start Data Log Demo

stop_logger:
  Stop Data Log Demo

help:
  Shows this command help.

```
## Appendix C: Performance Tuning Tips
Obviously, set the baud rate as high as you can.

The modern SD card is a block device, meaning that the smallest addressable unit is a a block (or sector) of 512 bytes. So, it helps performance if your write size is a multiple of 512. If it isn't, partial block writes involve reading the existing block, modifying it in memory, and writing it back out. With all the space in SD cards these days, it can be well worth it to pad a record length to a multiple of 512.

There is a controller in each SD card running all kinds of internal processes. Generally, flash memory has to be erased before it can be written, and the minimum erase size is the erase block or segment. The size of an erase block varies between devices, but is typically around 256kB or 512kB. When a smaller amount of data is to be written the whole erase block is read, modified in memory, and then written again. SD cards use various strategies to speed this up. Most implement a translation layer. For any I/O operation, a translation from virtual to physical address is carried out by the controller. If data inside a segment is to be overwritten, the translation layer remaps the virtual address of the segment to another erased physical address. The old physical segment is marked dirty and queued for an erase. Later, when it is erased, it can be reused. Usually, SD cards have a cache of one or more segments for increasing the performance of read and write operations. The SD card is a "black box'": Most of this is invisible to the user, except for performance. So, the write times are far from deterministic. It might be helpful to have your write size be some factor of the erase block size. The `info` command in [examples/command_line](https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main/examples/command_line) reports the erase block size. It gets it from the Card-Specific Data register (CSR) in the SD card.

There are more variables at the file system level. The allocation unit, also known as cluster, is a unit of "disk" space allocation for files. When the size of the allocation unit is 32768 bytes, a file with 100 bytes in size occupies 32768 bytes of disk space. The space efficiency of disk usage gets worse with increasing size of allocation unit, but, on the other hand, the read/write performance increases. Therefore the size of allocation unit is a trade-off between space efficiency and performance. This is something you can change by formatting the SD card. See [f_mkfs](http://elm-chan.org/fsw/ff/doc/mkfs.html).

File fragmentation can lead to long access times. One commonly used trick is to use [f_lseek](http://elm-chan.org/fsw/ff/doc/lseek.html) to pre-allocate a file to its ultimate size before beginning to write to it. Even better, you can pre-allocate a contiguous file using [f_expand](http://elm-chan.org/fsw/ff/doc/expand.html).

## Appendix D: Troubleshooting
* The first thing to try is lowering the SPI baud rate (see hw_config.c). This will also make it easier to use things like logic analyzers.
   * For SDIO, you can increase the clock divider in `sd_sdio_begin` in `sd_driver/SDIO/sd_card_sdio.c`.
   ```
    // Increase to 25 MHz clock rate
    rp2040_sdio_init(sd_card_p, 1);
    ```
* Make sure the SD card(s) are getting enough power. Try an external supply. Try adding a decoupling capacitor between Vcc and GND. 
  * Hint: check voltage while formatting card. It must be 2.7 to 3.6 volts. 
  * Hint: If you are powering a Pico with a PicoProbe, try adding a USB cable to a wall charger to the Pico under test.
* Try another brand of SD card. Some handle the SPI interface better than others. (Most consumer devices like cameras or PCs use the SDIO interface.) I have had good luck with SanDisk, PNY, and  Silicon Power.
* Tracing: Most of the source files have a couple of lines near the top of the file like:
```
#define TRACE_PRINTF(fmt, args...) // Disable tracing
//#define TRACE_PRINTF printf // Trace with printf
```
You can swap the commenting to enable tracing of what's happening in that file.
* Logic analyzer: for less than ten bucks, something like this [Comidox 1Set USB Logic Analyzer Device Set USB Cable 24MHz 8CH 24MHz 8 Channel UART IIC SPI Debug for Arduino ARM FPGA M100 Hot](https://smile.amazon.com/gp/product/B07KW445DJ/) and [PulseView - sigrok](https://sigrok.org/) make a nice combination for looking at SPI, as long as you don't run the baud rate too high. 
* Get yourself a protoboard and solder everything. So much more reliable than solderless breadboard!
* Better yet, go to somwhere like [JLCPCB](https://jlcpcb.com/) and get a printed circuit board!

[^1]: as of [Pull Request #12 Dynamic configuration](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/pull/12) (in response to [Issue #11 Configurable GPIO pins](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/issues/11)), Sep 11, 2021
[^2]: as of [Pull Request #5 Bug in ff_getcwd when FF_VOLUMES < 2](https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/pull/5), Aug 13, 2021

[^3]: In my experience, the Card Detect switch on these doesn't work worth a damn. This might not be such a big deal, because according to [Physical Layer Simplified Specification](https://www.sdcard.org/downloads/pls/) the Chip Select (CS) line can be used for Card Detection: "At power up this line has a 50KOhm pull up enabled in the card... For Card detection, the host detects that the line is pulled high." 
However, the Adafruit card has it's own 47 kΩ pull up on CS - Card Detect / Data Line [Bit 3], rendering it useless for Card Detection.
[^4]: [Physical Layer Simplified Specification](https://www.sdcard.org/downloads/pls/)
[^5]: Rationale: Instances of `sd_spi_if_t` or `sd_sdio_if_t` are separate objects instead of being embedded in `sd_card_t` objects because `sd_sdio_if_t` carries a lot of state information with it (including things like data buffers). The union of the two types has the size of the largest type, which would result in a lot of wasted space in instances of `sd_spi_if_t`. I had another solution using `malloc`, but some people are frightened of `malloc` in embedded systems.
