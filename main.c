
#include <rc522.h>
#include <delay.h>
#include <uart.h>
#include <stdint.h>
#include <stdio.h>

/*__inline int fputc(int c, FILE *stream)
{
    ITM_SendChar(c);
    return (c);
}*/

uint8_t rfid_id[4];

int main(void)
{
    /* Initialize RC522 and small startup delay */
    RC522_Init();
    delay(50);
    uart2_rxtx_init();

    uint8_t tagType[2];
    uint8_t serNum[5]; /* 4 bytes UID + BCC */

    while (1)
    {
        /* Check for a card in the field */
        if (RC522_Request(PICC_REQIDL, tagType) == MI_OK)
        {
            /* Anti-collision, read UID */
            if (RC522_Anticoll(serNum) == MI_OK)
            {
                /* store first 4 UID bytes into rfid_id */
                for (int i = 0; i < 4; i++)
                    rfid_id[i] = serNum[i];

                /* Print UID using printf (output via SWO/ITM) */
                // printf("Card UID: %02X %02X %02X %02X\n", rfid_id[0], rfid_id[1], rfid_id[2], rfid_id[3]);

                uart2_write(rfid_id[0]);
                uart2_write(rfid_id[1]);
                uart2_write(rfid_id[2]);
                uart2_write(rfid_id[3]);

                /* Compare to target IDs and light corresponding LED */
                if (rfid_id[0] == 0xB1 && rfid_id[1] == 0xC2 && rfid_id[2] == 0x29 && rfid_id[3] == 0x03)
                {
                    /* PD14 ON, PD12 OFF */
                    GPIOD->ODR |= GPIO_ODR_OD14;
                    GPIOD->ODR &= ~GPIO_ODR_OD12;
                }
                else if (rfid_id[0] == 0xDE && rfid_id[1] == 0x60 && rfid_id[2] == 0x08 && rfid_id[3] == 0x02)
                {
                    /* PD12 ON, PD14 OFF */
                    GPIOD->ODR |= GPIO_ODR_OD12;
                    GPIOD->ODR &= ~GPIO_ODR_OD14;
                }
                else
                {
                    /* No match: both off */
                    GPIOD->ODR &= ~(GPIO_ODR_OD12 | GPIO_ODR_OD14);
                }

                if (RC522_SelectTag(serNum) != MI_OK)
                {
                    // printf("SelectTag failed\n");
                    RC522_Halt();
                    continue;
                }
            }
        }
        delay(100);
    }

    /* never reached */
    return 0;
}
