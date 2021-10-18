#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arraylist.h"
#include "artigo.h"
#include "constants.h"
#include "io.h"

int fd_tmp;
ArrayList codigos_usados;

typedef struct linha_venda {
    unsigned long int codigo;
    long int quantidade;
    float montante;
} LinhaVenda;

int agrega(unsigned long int codigo, long int quantidade, float montante) {
    LinhaVenda lv;

    lseek(fd_tmp, (codigo - 1) * sizeof(LinhaVenda), SEEK_SET);

    if (read(fd_tmp, &lv, sizeof(LinhaVenda)) == 0) {
        // Se não conseguir ler, então é porque esse código ainda não está no
        // ArrayList de codigos_usados.
        lv.codigo = codigo;
        lv.quantidade = quantidade;
        lv.montante = montante;
        arraylist_add(codigos_usados, codigo);
    } else {
        if (lv.codigo == 0) {
            // às vezes consegue ler, mas os campos estão todos vazios
            // por exemplo, fomos até 3 e depois foi adicionado o código 1
            arraylist_add(codigos_usados, codigo);
            lv.codigo = codigo;
        }
        lv.quantidade += quantidade;
        lv.montante += montante;
    }

    lseek(fd_tmp, (codigo - 1) * sizeof(LinhaVenda), SEEK_SET);
    write(fd_tmp, &lv, sizeof(LinhaVenda));

    return 0;
}

void show_vendas_agregadas() {
    char linha[1024];
    int x;
    LinhaVenda lv;

    for (unsigned int i = 0; i < codigos_usados->size; i++) {
        lseek(fd_tmp, (codigos_usados->values[i] - 1) * sizeof(LinhaVenda),
              SEEK_SET);
        read(fd_tmp, &lv, sizeof(LinhaVenda));
        x = sprintf(linha, "%lu %ld %f\n", lv.codigo, lv.quantidade,
                    lv.montante);
        write(1, linha, x);
    }
}

int main(int argc, char **argv) {
    char buff[BUFSIZ];
    char *token;
    char *args[3];
    const char s[2] = " ";
    int i = 0;
    int flag = 1;
    char tmp_file[BUFSIZ];
    sprintf(tmp_file, "out/tmp%d", getpid());
    fd_tmp = open(tmp_file, O_CREAT | O_TRUNC | O_RDWR, 0666);

    codigos_usados = arraylist_create();

    Buffer buffer;
    create_buffer(0, &buffer, MAX_BUFFER);

    while (readln(&buffer, buff, BUFSIZ) > 0) {
        token = strtok(buff, s);
        if (token == NULL) flag = 0;
        while (token != NULL && i < 3) {
            args[i++] = strdup(token);
            token = strtok(NULL, s);
        }
        if (flag)
            agrega((unsigned long)atol(args[0]), atol(args[1]), atof(args[2]));
        flag = 1;
        i = 0;
    }

    show_vendas_agregadas();
    close(fd_tmp);
    unlink(tmp_file);
}
