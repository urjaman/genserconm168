void getline(unsigned char *buf, unsigned char len);
void sendstr_P(PGM_P str);
void sendstr(const unsigned char * str);
unsigned char* scanfor_notspace(unsigned char *buf);
unsigned char* scanfor_space(unsigned char *buf);
void tokenize(unsigned char *rcvbuf,unsigned char** ptrs, unsigned char* tkcntptr);
#define RECEIVE() uart_recv()
#define SEND(n) uart_send(n)
