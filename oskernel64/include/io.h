#ifndef IO_H
#define IO_H
char in_byte(int port);
short in_word(int port);

void out_byte(int port, int v);
void out_word(int port, int v);
#endif //IO_H
