/* UART MODULE HEADER */

unsigned char uart_isdata(void);
unsigned char uart_recv(void);
void uart_send(unsigned char val);
void uart_init(void);
void uart_wait_txdone(void);
#define BAUD 38400
