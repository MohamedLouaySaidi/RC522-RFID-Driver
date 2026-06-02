#ifndef UART_H_
#define UART_H_

#include <stdint.h>

void uart2_rxtx_init(void);
void uart2_write(int ch);
int __io_putchar(int ch); /* or int fputc(int c, FILE *stream) if you retarget differently */

#endif /* UART_H_ */