/*
 * rc522.h
 *
 * Driver for MFRC522 RFID module for STM32F4 (STM32F407VG Discovery board)
 * Provides register access, FIFO management and high-level MIFARE functions.
 *
 * This driver reuses platform SPI helpers (e.g. SPI1_Pins_Init, SPI1_Init,
 * spi1_transmit, spi1_receive, cs_enable, cs_disable) that exist in the
 * project (see `spi.c` / `spi.h`).
 *
 * API (selected):
 *  - RC522_GPIO_Init(void)
 *  - RC522_SPI_Init(void)
 *  - RC522_Init(void)
 *  - RC522_WriteReg(uint8_t addr, uint8_t data)
 *  - RC522_ReadReg(uint8_t addr)
 *  - RC522_WriteFIFO(uint8_t *data, uint8_t length)
 *  - RC522_ReadFIFO(uint8_t *data, uint8_t length)
 *  - RC522_Request(...), RC522_Anticoll(...), RC522_SelectTag(...)
 *  - RC522_Auth(...), RC522_Read(...), RC522_Write(...)
 *  - RC522_Halt(void)
 *
 * Minimal status codes are defined (MI_OK, MI_NOTAGERR, MI_ERR).
 */

#ifndef RC522_H_
#define RC522_H_

#include <stdint.h>
#include <stdbool.h>

/* Status codes */
#define MI_OK 0
#define MI_NOTAGERR 1
#define MI_ERR 2

/* MFRC522 Commands */
#define PCD_IDLE 0x00
#define PCD_AUTHENT 0x0E
#define PCD_RECEIVE 0x08
#define PCD_TRANSMIT 0x04
#define PCD_TRANSCEIVE 0x0C
#define PCD_RESETPHASE 0x0F
#define PCD_CALCCRC 0x03

/* Mifare commands */
#define PICC_REQIDL 0x26
#define PICC_REQALL 0x52
#define PICC_ANTICOLL 0x93
#define PICC_SELECTTAG 0x93
#define PICC_AUTHENT1A 0x60
#define PICC_AUTHENT1B 0x61
#define PICC_READ 0x30
#define PICC_WRITE 0xA0
#define PICC_DECREMENT 0xC0
#define PICC_INCREMENT 0xC1
#define PICC_RESTORE 0xC2
#define PICC_TRANSFER 0xB0
#define PICC_HALT 0x50

/* MFRC522 Registers (only ones used in driver) */
#define RC522_REG_COMMAND 0x01
#define RC522_REG_COMM_IE_N 0x02
#define RC522_REG_COMM_IRQ 0x04
#define RC522_REG_ERROR 0x06
#define RC522_REG_STATUS2 0x08
#define RC522_REG_FIFO_DATA 0x09
#define RC522_REG_FIFO_LEVEL 0x0A
#define RC522_REG_CONTROL 0x0C
#define RC522_REG_BIT_FRAMING 0x0D
#define RC522_REG_COLL 0x0E

#define RC522_REG_MODE 0x11
#define RC522_REG_TX_MODE 0x12
#define RC522_REG_TX_CONTROL 0x14
#define RC522_REG_TX_AUTO 0x15
#define RC522_REG_T_MODE 0x2A
#define RC522_REG_T_PRESCALER 0x2B
#define RC522_REG_T_RELOAD_H 0x2C
#define RC522_REG_T_RELOAD_L 0x2D
#define RC522_REG_CRC_RESULT_M 0x21
#define RC522_REG_CRC_RESULT_L 0x22
#define RC522_REG_DIV_IRQ 0x05

#define RC522_MAX_LEN 16

/* Initialization */
void RC522_GPIO_Init(void); // Configure CS/RST and SPI pins
void RC522_SPI_Init(void);  // Initialize SPI peripheral
void RC522_Init(void);      // Reset and configure the RC522

/* Register access */
void RC522_WriteReg(uint8_t addr, uint8_t data);
uint8_t RC522_ReadReg(uint8_t addr);

/* FIFO management */
void RC522_WriteFIFO(uint8_t *data, uint8_t length);
void RC522_ReadFIFO(uint8_t *data, uint8_t length);

/* High-level RFID functions */
uint8_t RC522_Request(uint8_t reqMode, uint8_t *TagType);
uint8_t RC522_Anticoll(uint8_t *serNum);
uint8_t RC522_SelectTag(uint8_t *serNum);
uint8_t RC522_Auth(uint8_t authMode, uint8_t BlockAddr, uint8_t *key, uint8_t *serNum);
uint8_t RC522_Read(uint8_t blockAddr, uint8_t *recvData);
uint8_t RC522_Write(uint8_t blockAddr, uint8_t *writeData);
void RC522_Halt(void);

/* Utility */
void RC522_CalculateCRC(uint8_t *data, uint8_t len, uint8_t *result);

#endif /* RC522_H_ */
