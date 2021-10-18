#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "artigo.h"
#include "constants.h"
#include "io.h"

int fd_errors;

LCliente getLCliente(char *fifo_cliente, char *buff, int *r) {
    LCliente lc;
    *r = -1;
    char s[2] = " ";
    if (contaEspacos(buff) == 0) {
        lc.operacao = 'c';
        strcpy(lc.nomeCliente, fifo_cliente);
        lc.codigo = ((unsigned long int)atol(buff));
        *r = 0;
    } else if (contaEspacos(buff) == 1) {
        int i = 0;
        char *token;
        lc.operacao = 'a';
        strcpy(lc.nomeCliente, fifo_cliente);
        token = strtok(buff, s);
        while (token != NULL && i < 2) {
            if (i == 0) {
                lc.codigo = ((unsigned long int)atol(token));
                i++;
            } else if (i == 1) {
                lc.quantidade = (atol(token));
                i++;
            }
            token = strtok(NULL, s);
        }
        *r = 0;
    }
    return lc;
}

int main(int argc, char *argv[]) {
    char *fifo_server = "server";
    char fifo_cliente[128];
    sprintf(fifo_cliente, "cliente%d", (int)getpid());
    char buff[BUFSIZ];

    fd_errors = open(LOG_CLIENTE, O_CREAT | O_APPEND | O_WRONLY, 0666);

    if (mkfifo(fifo_cliente, 0666) == -1) {
        write(fd_errors, "ERROR: Já existe fifo\n", 24);
    }

    int fif_cliente;
    int fif_server;
    int size;
    Buffer buffer;
    create_buffer(0, &buffer, MAX_BUFFER);
    int r;
    while ((size = readln(&buffer, buff, BUFSIZ)) > 0) {
        LCliente lc = getLCliente(fifo_cliente, buff, &r);
        LinhaAtualiza la;
        LinhaConsulta lcons;
        if (r == 0) {
            if ((fif_server = open(fifo_server, O_WRONLY)) == -1) return -1;
            write(fif_server, &lc, sizeof(LCliente));
            close(fif_server);
            if ((fif_cliente = open(fifo_cliente, O_RDONLY)) == -1) return -1;
            if (lc.operacao == 'a') {
                while (read(fif_cliente, &la, sizeof(LinhaAtualiza))) {
                }
                size = sprintf(buff, "%lu %lu\n", la.codigo, la.stock);
            } else if (lc.operacao == 'c') {
                while (read(fif_cliente, &lcons, sizeof(LinhaConsulta))) {
                }
                size = sprintf(buff, "%lu %f\n", lcons.stock, lcons.preco);
            }
            close(fif_cliente);
            write(1, buff, size);
        }
    }
    destroy_buffer(&buffer);
    unlink(fifo_cliente);
    return 0;
}