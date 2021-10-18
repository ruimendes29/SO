#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "artigo.h"
#include "constants.h"
#include "io.h"

unsigned long int posicaoFicheiro = sizeof(unsigned long int);
unsigned long int proximoCodigo = 1;
unsigned long int obsoleteSpace = 0;

unsigned long int compactadorStringsFile() {
    int fa = open(FILE_ARTIGOS, O_RDWR, 0666);
    int fs = open(FILE_STRINGS, O_RDONLY, 0666);
    int ft = open(TMP_STRINGS, O_CREAT | O_WRONLY | O_APPEND, 0666);
    Artigo a;
    size_t tamanhoULI = sizeof(unsigned long int);
    size_t lido;
    unsigned long int zero = 0UL;
    write(ft, &zero, tamanhoULI);
    unsigned long int novasPosicoes = tamanhoULI;
    int size;
    char *buffer;
    lseek(fa, 2 * tamanhoULI, SEEK_SET);
    while ((lido = (read(fa, &a, sizeof(struct artigo)))) != 0) {
        lseek(fs, a.posicaoString, SEEK_SET);
        read(fs, &size, sizeof(int));
        buffer = malloc(sizeof(char) * size + 1);
        read(fs, buffer, size);
        a.posicaoString = novasPosicoes;
        novasPosicoes += write(ft, &size, sizeof(int));
        novasPosicoes += write(ft, buffer, size);
        lseek(fa, -lido, SEEK_CUR);
        write(fa, &a, sizeof(struct artigo));
        free(buffer);
    }
    if (remove(FILE_STRINGS) == 0)
        rename(TMP_STRINGS, FILE_STRINGS);
    else
        return -1;
    return novasPosicoes;
}

int fd_errors;

unsigned long int insereArtigo(char *nome, char *preco) {
    int fa = open(FILE_ARTIGOS, O_RDWR, 0666);
    int fs = open(FILE_STRINGS, O_WRONLY | O_APPEND,
                  0666);  // Tentar fazer sem o append???
    int fq = open(FILE_STOCK, O_WRONLY, 0666);
    int length = strlen(nome);
    unsigned long int zero = 0UL;  // 0 unsigned long int
    size_t tamanhoULI = sizeof(unsigned long int);
    Artigo a;
    if (fa == -1) {
        fa = open(FILE_ARTIGOS, O_CREAT | O_RDWR, 0666);
        write(fa, &proximoCodigo, tamanhoULI);
        write(fa, &posicaoFicheiro, tamanhoULI);
        lseek(fa, 0, SEEK_SET);
    }
    if (fs == -1) {
        fs = open(FILE_STRINGS, O_CREAT | O_WRONLY | O_APPEND, 0666);
        write(fs, &obsoleteSpace, tamanhoULI);
    }
    if (fq == -1) fq = open(FILE_STOCK, O_CREAT | O_WRONLY, 0666);
    read(fa, &proximoCodigo, tamanhoULI);  // Ler o codigo do artigo introduzido
    a.preco = atof(preco);                 // Preço do artigo
    read(fa, &posicaoFicheiro,
         tamanhoULI);  // Posição no ficheiro strings do nome do artigo
    a.posicaoString = posicaoFicheiro;
    lseek(fa,
          (proximoCodigo - 1) * sizeof(struct artigo) +
              sizeof(unsigned long int) *
                  2,  // 2 uli que estao presentes no ficheiro artigos
          SEEK_SET);  // encontrar a posiçao onde vai ser escrito o artigo
    write(fa, &a, sizeof(struct artigo));  // escrever o artigo
    write(fs, &length, sizeof(int));
    write(
        fs, nome,
        (ssize_t)length);  // escrever o nome do artigo no ficheiro de strings;
    lseek(fq, (proximoCodigo - 1) * sizeof(unsigned long int), SEEK_SET);
    write(fq, &zero, tamanhoULI);
    posicaoFicheiro += length + sizeof(int);
    proximoCodigo++;
    lseek(fa, 0, SEEK_SET);
    write(fa, &proximoCodigo, tamanhoULI);
    write(fa, &posicaoFicheiro, tamanhoULI);
    lseek(fa, 0, SEEK_SET);
    close(fa);
    lseek(fs, 0, SEEK_SET);
    close(fs);
    lseek(fq, 0, SEEK_SET);
    close(fq);
    return (proximoCodigo - 1);
}

int alteraNome(unsigned long int codigo, char *nome) {
    int fa = open(FILE_ARTIGOS, O_RDWR, 0666);
    int fs = open(FILE_STRINGS, O_RDWR, 0666);
    int length = strlen(nome);
    int obsoleteLength;
    Artigo a;
    if (fa == -1) {
        write(fd_errors, "ERROR: Não existe ficheiro de artigos.\n", 40);
        return -1;
    }
    if (fs == -1) {
        write(fd_errors, "ERROR: Não existe ficheiro de strings.\n", 40);
        return -1;
    }
    lseek(fa, sizeof(unsigned long int), SEEK_SET);
    read(fa, &posicaoFicheiro,
         sizeof(unsigned long int));  // Posição no ficheiro strings do nome do
                                      // artigo
    lseek(fa,
          (codigo - 1) * sizeof(struct artigo) + sizeof(unsigned long int) * 2,
          SEEK_SET);
    if (read(fa, &a, sizeof(struct artigo)) == 0) {
        write(fd_errors, "ERROR: O artigo procurado não existe.\n", 39);
        return -1;
    }
    lseek(fs, 0, SEEK_SET);
    read(fs, &obsoleteSpace, sizeof(unsigned long int));
    lseek(fs, a.posicaoString, SEEK_SET);
    read(fs, &obsoleteLength, sizeof(int));
    obsoleteSpace += obsoleteLength + sizeof(int);
    lseek(fs, 0, SEEK_SET);
    write(fs, &obsoleteSpace, sizeof(unsigned long int));
    a.posicaoString = posicaoFicheiro;
    lseek(fa,
          (codigo - 1) * sizeof(struct artigo) + sizeof(unsigned long int) * 2,
          SEEK_SET);
    write(fa, &a, sizeof(struct artigo));
    lseek(fs, posicaoFicheiro, SEEK_SET);
    write(fs, &length, sizeof(int));
    write(
        fs, nome,
        (ssize_t)length);  // escrever o nome do artigo no ficheiro de strings;
    posicaoFicheiro += length + sizeof(int);
    if ((float)obsoleteSpace / (float)posicaoFicheiro > 0.2)
        posicaoFicheiro = compactadorStringsFile();
    lseek(fa, sizeof(unsigned long int), SEEK_SET);
    write(fa, &posicaoFicheiro, sizeof(unsigned long int));
    close(fa);
    close(fs);
    return 0;
}

int talkToServer(int sinal) {
    int fifo_server = open("out/server2ma", O_RDONLY);
    pid_t pidServer;
    if (fifo_server == -1) {
        write(fd_errors, "ERROR: O servidor não está aberto.\n", 37);
        return -1;
    } else {
        read(fifo_server, &pidServer, sizeof(pid_t));
        kill(pidServer, sinal);
        close(fifo_server);
        return 0;
    }
}

int alteraPreco(unsigned long int codigo, float preco) {
    int fa = open(FILE_ARTIGOS, O_RDWR, 0666);
    char *fifo_ma = "ma2server";
    mkfifo(fifo_ma, 0666);
    ArtCache ac;
    ac.codigo = codigo;
    ac.preco = preco;
    if (talkToServer(SIGUSR2) == 0) {
        int fd_fifoma = open(fifo_ma, O_WRONLY, 0666);
        write(fd_fifoma, &ac, sizeof(ArtCache));
        close(fd_fifoma);
    }
    Artigo a;
    if (fa == -1) {
        write(fd_errors, "ERROR: Não existe ficheiro de artigos.\n", 40);
        return -1;
    }
    lseek(fa,
          (codigo - 1) * sizeof(struct artigo) + sizeof(unsigned long int) * 2,
          SEEK_SET);
    if (read(fa, &a, sizeof(struct artigo)) == 0) {
        write(fd_errors, "ERROR: O artigo procurado não existe.\n", 39);
        return -1;
    }
    a.preco = preco;
    lseek(fa,
          (codigo - 1) * sizeof(struct artigo) + sizeof(unsigned long int) * 2,
          SEEK_SET);
    write(fa, &a, sizeof(struct artigo));
    close(fa);
    unlink(fifo_ma);
    return 0;
}

int quantosDigs(int x) {
    int r = 1;
    while (x > 10) {
        x /= 10;
        r++;
    }
    return r;
}

int main(int argc, char *argv[]) {
    char buff[BUFSIZ];
    char *token;
    char *args[3];
    char *stringCodigo;

    fd_errors = open(LOG_MANUTENCAO, O_CREAT | O_TRUNC | O_WRONLY, 0666);

    int i = 0;
    unsigned long int codigo;
    const char s[2] = " ";
    Buffer buffer;
    create_buffer(0, &buffer, MAX_BUFFER);
    while (readln(&buffer, buff, BUFSIZ)) {
        token = strtok(buff, s);
        switch (buff[0]) {
            case 'i':
                while (token != NULL && i < 3) {
                    args[i++] = strdup(token);
                    token = strtok(NULL, s);
                }
                codigo = insereArtigo(args[1], args[2]);
                stringCodigo =
                    malloc(sizeof(char) * (quantosDigs(codigo) + 13));
                sprintf(stringCodigo, "%lu\n", codigo);
                write(1, stringCodigo, strlen(stringCodigo));
                free(stringCodigo);
                i = 0;
                break;
            case 'n':
                while (token != NULL && i < 3) {
                    args[i++] = strdup(token);
                    token = strtok(NULL, s);
                }
                alteraNome(atol(args[1]), args[2]);
                i = 0;
                break;
            case 'p':
                while (token != NULL && i < 3) {
                    args[i++] = strdup(token);
                    token = strtok(NULL, s);
                }
                alteraPreco(atol(args[1]), atof(args[2]));
                i = 0;
                break;
            case 'a':
                talkToServer(SIGUSR1);
                break;
            case 'c':
                compactadorStringsFile();
                break;
            default:
                break;
        }
    }
}
