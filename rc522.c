#include <rc522.h>
#include <spi.h>
#include <stm32f4xx.h>
#include <string.h>

/* Internal helper: chip select wrappers mapped to existing cs_enable/cs_disable */
static inline void rc_cs_low(void) { cs_enable(); }
static inline void rc_cs_high(void) { cs_disable(); }

/* Write a single register */
void RC522_WriteReg(uint8_t addr, uint8_t data)
{
    uint8_t tx[2];
    /* Address format: 0xxxxxx0 for write (addr<<1 & 0x7E) */
    tx[0] = (addr << 1) & 0x7E;
    tx[1] = data;
    rc_cs_low();
    spi1_transmit(tx, 2);
    rc_cs_high();
}

/* Read a single register */
uint8_t RC522_ReadReg(uint8_t addr)
{
    uint8_t address = ((addr << 1) & 0x7E) | 0x80;
    uint8_t value = 0;
    rc_cs_low();
    spi1_transmit(&address, 1);
    spi1_receive(&value, 1);
    rc_cs_high();
    return value;
}

/* Write multiple bytes into FIFO */
void RC522_WriteFIFO(uint8_t *data, uint8_t length)
{
    for (uint8_t i = 0; i < length; i++)
    {
        RC522_WriteReg(RC522_REG_FIFO_DATA, data[i]);
    }
}

/* Read multiple bytes from FIFO (reads 'length' bytes into data) */
void RC522_ReadFIFO(uint8_t *data, uint8_t length)
{
    for (uint8_t i = 0; i < length; i++)
    {
        data[i] = RC522_ReadReg(RC522_REG_FIFO_DATA);
    }
}

/* CRC calculation using the RC522 internal CRC engine */
void RC522_CalculateCRC(uint8_t *data, uint8_t len, uint8_t *result)
{
    uint8_t i;
    RC522_WriteReg(RC522_REG_DIV_IRQ, RC522_ReadReg(RC522_REG_DIV_IRQ) & (~0x04));
    RC522_WriteReg(RC522_REG_FIFO_LEVEL, 0x80); // clear FIFO
    for (i = 0; i < len; i++)
    {
        RC522_WriteReg(RC522_REG_FIFO_DATA, data[i]);
    }
    RC522_WriteReg(RC522_REG_COMMAND, PCD_CALCCRC);
    uint16_t timeout = 0xFF;
    uint8_t n;
    do
    {
        n = RC522_ReadReg(RC522_REG_DIV_IRQ);
        timeout--;
    } while ((timeout != 0) && !(n & 0x04));
    result[0] = RC522_ReadReg(RC522_REG_CRC_RESULT_L);
    result[1] = RC522_ReadReg(RC522_REG_CRC_RESULT_M);
}

/* Core communication with the card */
static uint8_t RC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint16_t *backLen)
{
    uint8_t status = MI_ERR;
    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;
    uint8_t lastBits;
    uint8_t n = 0;
    uint16_t i;

    if (command == PCD_AUTHENT)
    {
        irqEn = 0x12;
        waitIRq = 0x10;
    }
    else if (command == PCD_TRANSCEIVE)
    {
        irqEn = 0x77;
        waitIRq = 0x30;
    }

    RC522_WriteReg(RC522_REG_COMM_IE_N, irqEn | 0x80);
    RC522_WriteReg(RC522_REG_COMM_IRQ, 0x7F);   // clear
    RC522_WriteReg(RC522_REG_FIFO_LEVEL, 0x80); // flush
    RC522_WriteReg(RC522_REG_COMMAND, PCD_IDLE);

    /* Write data to FIFO */
    for (i = 0; i < sendLen; i++)
    {
        RC522_WriteReg(RC522_REG_FIFO_DATA, sendData[i]);
    }

    RC522_WriteReg(RC522_REG_COMMAND, command);
    if (command == PCD_TRANSCEIVE)
    {
        RC522_WriteReg(RC522_REG_BIT_FRAMING, RC522_ReadReg(RC522_REG_BIT_FRAMING) | 0x80);
    }

    /* Wait for completion */
    i = 2000; // timeout
    do
    {
        n = RC522_ReadReg(RC522_REG_COMM_IRQ);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

    RC522_WriteReg(RC522_REG_BIT_FRAMING, RC522_ReadReg(RC522_REG_BIT_FRAMING) & (~0x80));

    if (i != 0)
    {
        if (!(RC522_ReadReg(RC522_REG_ERROR) & 0x1B))
        {
            status = MI_OK;
            if (n & irqEn & 0x01)
            {
                status = MI_ERR;
            }
            if (command == PCD_TRANSCEIVE)
            {
                n = RC522_ReadReg(RC522_REG_FIFO_LEVEL);
                uint8_t l = n;
                lastBits = RC522_ReadReg(RC522_REG_CONTROL) & 0x07;
                if (lastBits)
                {
                    *backLen = (n - 1) * 8 + lastBits;
                }
                else
                {
                    *backLen = n * 8;
                }

                if (n == 0)
                    n = 1;
                if (n > RC522_MAX_LEN)
                    n = RC522_MAX_LEN;

                for (i = 0; i < n; i++)
                {
                    backData[i] = RC522_ReadReg(RC522_REG_FIFO_DATA);
                }
            }
        }
        else
        {
            status = MI_ERR;
        }
    }

    return status;
}

/* Request (card detection) */
uint8_t RC522_Request(uint8_t reqMode, uint8_t *TagType)
{
    uint8_t status;
    uint16_t backBits = 0;
    RC522_WriteReg(RC522_REG_BIT_FRAMING, 0x07);
    TagType[0] = reqMode;
    status = RC522_ToCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);
    if ((status != MI_OK) || (backBits != 0x10))
    {
        return MI_ERR;
    }
    return MI_OK;
}

/* Anti-collision to get UID */
uint8_t RC522_Anticoll(uint8_t *serNum)
{
    uint8_t status;
    uint8_t i;
    uint8_t serCheck = 0;
    uint16_t unLen;

    RC522_WriteReg(RC522_REG_BIT_FRAMING, 0x00);
    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;
    status = RC522_ToCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);
    if (status == MI_OK)
    {
        for (i = 0; i < 4; i++)
            serCheck ^= serNum[i];
        if (serCheck != serNum[4])
        {
            status = MI_ERR;
        }
    }
    return status;
}

/* Select tag by UID. Returns MI_OK on success. */
uint8_t RC522_SelectTag(uint8_t *serNum)
{
    uint8_t buf[9];
    uint8_t i;
    uint16_t recvBits;

    buf[0] = PICC_SELECTTAG;
    buf[1] = 0x70;
    for (i = 0; i < 5; i++)
        buf[2 + i] = serNum[i];
    RC522_CalculateCRC(buf, 7, &buf[7]);

    if (RC522_ToCard(PCD_TRANSCEIVE, buf, 9, buf, &recvBits) != MI_OK)
        return MI_ERR;
    /* When success, return SAK (one byte) in buf[0] */
    return MI_OK;
}

/* Authentication: authMode = PICC_AUTHENT1A or PICC_AUTHENT1B */
uint8_t RC522_Auth(uint8_t authMode, uint8_t BlockAddr, uint8_t *key, uint8_t *serNum)
{
    uint8_t buff[12];
    uint16_t recvBits;
    buff[0] = authMode;
    buff[1] = BlockAddr;
    memcpy(&buff[2], key, 6);
    memcpy(&buff[8], serNum, 4);

    if (RC522_ToCard(PCD_AUTHENT, buff, 12, buff, &recvBits) != MI_OK)
        return MI_ERR;
    /* Check if authenticated by reading Status2Reg bit 3 (MIFARE OK) */
    if (!(RC522_ReadReg(RC522_REG_STATUS2) & 0x08))
        return MI_ERR;
    return MI_OK;
}

/* Read 16 bytes from blockAddr into recvData (expects buffer of at least 16 bytes) */
uint8_t RC522_Read(uint8_t blockAddr, uint8_t *recvData)
{
    uint8_t cmd[4];
    uint16_t backLen;

    cmd[0] = PICC_READ;
    cmd[1] = blockAddr;
    RC522_CalculateCRC(cmd, 2, &cmd[2]);

    if (RC522_ToCard(PCD_TRANSCEIVE, cmd, 4, recvData, &backLen) != MI_OK)
        return MI_ERR;
    if (backLen != 16 * 8)
        return MI_ERR; // expect 16 bytes
    return MI_OK;
}

/* Write 16 bytes to blockAddr from writeData (16 bytes) */
uint8_t RC522_Write(uint8_t blockAddr, uint8_t *writeData)
{
    uint8_t cmd[4];
    uint8_t ackBuf[4];
    uint16_t backLen;

    cmd[0] = PICC_WRITE;
    cmd[1] = blockAddr;
    RC522_CalculateCRC(cmd, 2, &cmd[2]);

    if (RC522_ToCard(PCD_TRANSCEIVE, cmd, 4, ackBuf, &backLen) != MI_OK)
        return MI_ERR;
    /* Check for ACK: ackBuf should have specific bits, many libraries check a short response of 4 bits */
    /* Send the 16 bytes + CRC */
    uint8_t buff[18];
    memcpy(buff, writeData, 16);
    RC522_CalculateCRC(buff, 16, &buff[16]);
    if (RC522_ToCard(PCD_TRANSCEIVE, buff, 18, ackBuf, &backLen) != MI_OK)
        return MI_ERR;
    return MI_OK;
}

/* Halt the card */
void RC522_Halt(void)
{
    uint8_t buff[4];
    uint16_t unLen;
    buff[0] = PICC_HALT;
    buff[1] = 0;
    RC522_CalculateCRC(buff, 2, &buff[2]);
    RC522_ToCard(PCD_TRANSCEIVE, buff, 4, buff, &unLen);
}

/* GPIO init: config CS (PB0) and RST (PA8) and SPI pins via existing helper */
void RC522_GPIO_Init(void)
{
    /* SPI pins */
    SPI1_Pins_Init();

    /* Configure RST PA8 as output */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER |= GPIO_MODER_MODE8_0;
    GPIOA->MODER &= ~GPIO_MODER_MODE8_1;

    /* Configure CS PB0 as output (PB0) */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    GPIOB->MODER |= GPIO_MODER_MODE0_0;
    GPIOB->MODER &= ~GPIO_MODER_MODE0_1;

    /* Drive RST high (not reset) */
    GPIOA->BSRR = GPIO_BSRR_BS8;
    /* De-select CS (high) */
    GPIOB->BSRR = GPIO_BSRR_BS0;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    GPIOD->MODER |= (GPIO_MODER_MODER12_0 | GPIO_MODER_MODER13_0 | GPIO_MODER_MODER14_0 | GPIO_MODER_MODER15_0);
}

/* SPI init wrapper */
void RC522_SPI_Init(void)
{
    SPI1_Init();
}

/* Full RC522 init: reset and configure common registers */
void RC522_Init(void)
{
    RC522_GPIO_Init();
    RC522_SPI_Init();

    /* Reset sequence: toggle RST (PA8 low then high) */
    GPIOA->BSRR = GPIO_BSRR_BR8; // reset low
    for (volatile int i = 0; i < 100000; i++)
        ;
    GPIOA->BSRR = GPIO_BSRR_BS8; // reset high
    for (volatile int i = 0; i < 100000; i++)
        ;

    RC522_WriteReg(RC522_REG_COMMAND, PCD_RESETPHASE);

    RC522_WriteReg(RC522_REG_T_MODE, 0x80);
    RC522_WriteReg(RC522_REG_T_PRESCALER, 0xA9);
    RC522_WriteReg(RC522_REG_T_RELOAD_L, 0xE8);
    RC522_WriteReg(RC522_REG_T_RELOAD_H, 0x03);
    RC522_WriteReg(RC522_REG_TX_AUTO, 0x40);
    RC522_WriteReg(RC522_REG_MODE, 0x3D);

    /* Antenna ON */
    uint8_t temp = RC522_ReadReg(RC522_REG_TX_CONTROL);
    if (!(temp & 0x03))
    {
        RC522_WriteReg(RC522_REG_TX_CONTROL, temp | 0x03);
    }
}
