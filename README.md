# RC522 RFID Driver (STM32F4, bare metal)

Bare-metal C driver and demo for the MFRC522 RFID module on STM32F407VG (Discovery). The project includes SPI register access, FIFO handling, and common MIFARE commands with a simple UID read loop.

## Features

- MFRC522 init, register read/write, FIFO access
- Card request, anti-collision (UID), select, auth, read, write, halt
- SPI1 driver (PA5/PA6/PA7) and chip select on PB0
- UART2 output (PA2/PA3) at 115200 for UID bytes
- SysTick-based delays

## Hardware

Tested with STM32F407VG (Discovery) and RC522 module (3.3V).

### Connections

- RC522 SCK -> PA5 (SPI1_SCK)
- RC522 MISO -> PA6 (SPI1_MISO)
- RC522 MOSI -> PA7 (SPI1_MOSI)
- RC522 SDA/SS -> PB0 (CS)
- RC522 RST -> PA8
- RC522 3.3V -> 3V3
- RC522 GND -> GND

Optional LEDs (demo): PD12 and PD14 are toggled in main.c based on UID match.

## Build and Flash

1. Open the project file: `SPI-RC522.uvprojx` in Keil uVision.
2. Select the target and build.
3. Flash to the board using your usual Keil workflow.

## Usage

- `main.c` continuously scans for a card, reads the UID, and sends the 4 UID bytes over UART2.
- Update the UID checks in `main.c` to match your own tags.
- If you want readable output, you can enable `printf` redirection using `__io_putchar` in `uart.c`.

## Project Files

- `rc522.c/h`: MFRC522 driver
- `spi.c/h`: SPI1 driver
- `uart.c/h`: UART2 driver and printf retarget
- `delay.c/h`: SysTick delay helpers
- `main.c`: Example application

## Notes

- The RC522 module is 3.3V only. Do not use 5V logic.
- If your wiring differs, update the pin mappings in `spi.c` and `rc522.c`.
