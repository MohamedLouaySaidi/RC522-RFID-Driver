#ifndef SPI_H_
#define SPI_H_

#include <stdint.h>

void SPI1_Pins_Init();
void SPI1_Init();
void spi1_transmit(uint8_t *data, uint32_t size);
void spi1_receive(uint8_t *data, uint32_t size);
void cs_enable(void);
void cs_disable(void);

#endif /* SPI_H_ */