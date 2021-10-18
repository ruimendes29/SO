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

int terminado = 1;
int jaLeu = 0;
int escreveuStock = 0;
int escreveuVenda = 0;
int escreveuPosicaoFinalVendas = 0;
int escreveuLinhaFinalVendas = 0;

int fd_errors;

typedef enum {
    posicaoFinalVendas,
    linhaFinalVendas,
    posicaoUltimaAgregacao,
    linhaUltimaAgregacao
} GLOBAL;

int fd_server2ma;

ArtCache cache[100];
int ultimaPos = 0;

LCliente lc;
int fif_server;
int fif_cliente;

int lookForCodigo(unsigned long int codigo) {
    for (int i = 0; i < ultimaPos; i++)
        if (cache[i].codigo == codigo) return i;
    return -1;
}

unsigned long int getGlobalVariavel(GLOBAL g) {
    unsigned long int zero = 0UL;
    unsigned long int ret;
    size_t tamanhoULI = sizeof(unsigned long int);
    int fpos = open(FILE_POSICOES, O_RDWR, 0666);
    if (fpos == -1) {
        fpos = open(FILE_POSICOES, O_CREAT | O_RDWR, 0666);
        for (int i = 0; i < 4; i++) write(fpos, &zero, tamanhoULI);
    }
    lseek(fpos, tamanhoULI * g, SEEK_SET);
    read(fpos, &ret, tamanhoULI);
    close(fpos);
    return ret;
}

void setGlobalVariavel(GLOBAL g, unsigned long int newValue) {
    size_t tamanhoULI = sizeof(unsigned long int);
    int fpos = open(FILE_POSICOES, O_WRONLY, 0666);
    lseek(fpos, tamanhoULI * g, SEEK_SET);
    write(fpos, &newValue, tamanhoULI);
    close(fpos);
}

void insereCache(unsigned long int codigo) {
    int fa = open(FILE_ARTIGOS, O_RDONLY);
    if (fa == -1) {
        write(fd_errors, "ERROR: Não existe ficheiro de artigos.\n", 40);
        return;
    }
    Artigo a;
    lseek(fa,
          (codigo - 1) * sizeof(struct artigo) + sizeof(unsigned long int) * 2,
          SEEK_SET);
    read(fa, &a, sizeof(struct artigo));
    for (int i = ultimaPos; i > 0; i--) cache[i] = cache[i - 1];
    if (ultimaPos < 99) ultimaPos++;
    ArtCache novoNaCache;
    novoNaCache.codigo = codigo;
    novoNaCache.preco = a.preco;
    cache[0] = novoNaCache;
    close(fa);
}

void atualizaCache(int posicao) {
    ArtCache temp;
    for (int i = posicao; i > 0; i--) {
        temp = cache[i];
        cache[i] = cache[i - 1];
        cache[i - 1] = temp;
    }
}

int fazConsulta(unsigned long int codigo, char *cliente) {
    int fq = open(FILE_STOCK, O_RDONLY);
    int posicaoCache = lookForCodigo(codigo);
    if (posicaoCache == -1)
        insereCache(codigo);
    else
        atualizaCache(posicaoCache);
    LinhaConsulta lc;
    unsigned long int stock;
    size_t tamanhoULI = sizeof(unsigned long int);
    if (fq == -1) {
        write(fd_errors, "ERROR: Não existe ficheiro de stocks.\n", 39);
        return -1;
    }
    lseek(fq, (codigo - 1) * sizeof(unsigned long int), SEEK_SET);
    read(fq, &stock, tamanhoULI);
    lc.preco = cache[0].preco;
    lc.stock = stock;
    int fif_cliente = open(cliente, O_WRONLY, 0666);
    write(fif_cliente, &lc, sizeof(LinhaConsulta));
    close(fif_cliente);
    close(fq);
    return 0;
}

int atualizaStock(unsigned long int codigo, long int quantidade,
                  char *cliente) {
    int fq = open(FILE_STOCK, O_RDWR, 0666);
    int fv = open(FILE_VENDAS, O_RDWR | O_APPEND, 0666);
    LinhaAtualiza la;
    char *buff = malloc(sizeof(char) * (BUFSIZ));
    unsigned long int stock;
    unsigned long int tempVar;
    int stringSize;
    int posicaoCache = lookForCodigo(codigo);
    if (posicaoCache == -1)
        insereCache(codigo);
    else
        atualizaCache(posicaoCache);
    size_t tamanhoULI = sizeof(unsigned long int);
    if (fq == -1) {
        write(fd_errors, "ERROR: Não existe ficheiro de stocks.\n", 39);
        return -1;
    }
    if (fv == -1) fv = open(FILE_VENDAS, O_CREAT | O_RDWR | O_APPEND, 0666);
    lseek(fq, (codigo - 1) * sizeof(unsigned long int), SEEK_SET);
    read(fq, &stock, tamanhoULI);
    if (escreveuStock == 0) {
        if ((long int)stock + quantidade < 0)
            stock = 0;
        else
            stock += quantidade;
        lseek(fq, (codigo - 1) * sizeof(unsigned long int), SEEK_SET);
        write(fq, &stock, tamanhoULI);
    }
    escreveuStock = 1;

    stringSize = sprintf(buff, "%lu %ld %f\n", codigo, quantidade,
                         cache[0].preco * quantidade * -1);
    tempVar = getGlobalVariavel(posicaoFinalVendas);
    tempVar += stringSize;
    if (escreveuPosicaoFinalVendas == 0)
        setGlobalVariavel(posicaoFinalVendas, tempVar);
    escreveuPosicaoFinalVendas = 1;
    tempVar = getGlobalVariavel(linhaFinalVendas);
    tempVar++;
    if (escreveuLinhaFinalVendas == 0)
        setGlobalVariavel(linhaFinalVendas, tempVar);
    escreveuLinhaFinalVendas = 1;
    if (escreveuVenda == 0) write(fv, buff, stringSize);
    escreveuVenda = 1;
    la.codigo = codigo;
    la.stock = stock;
    fif_cliente = open(cliente, O_WRONLY);
    write(fif_cliente, &la, sizeof(LinhaAtualiza));
    close(fif_cliente);
    close(fq);
    close(fv);
    free(buff);
    return 0;
}

void calculaOffsets(unsigned long int offsets[], unsigned long int N) {
    int fv = open(FILE_VENDAS, O_RDONLY, 0666);
    const unsigned long int pua = getGlobalVariavel(posicaoUltimaAgregacao);
    const unsigned long int pf = getGlobalVariavel(posicaoFinalVendas);
    lseek(fv, pua, SEEK_SET);
    unsigned long int defaultOffSet = (pf - pua) / N;
    int r;
    int s;
    int flag = 1;
    char c;
    char buff[defaultOffSet * 2];
    for (unsigned long int i = 0; i < N; i++)
        offsets[i] = defaultOffSet * i + pua;
    for (unsigned long int i = 1; i < N && flag; i++) {
        r = read(fv, &buff, defaultOffSet);
        if (r == 0) {
            flag = 0;
        } else {
            if (buff[r - 1] != '\n') {
                do {
                    s = read(fv, &c, sizeof(char));
                    if (s == 0) {
                        flag = 0;
                    } else {
                        for (unsigned long int k = i; k < N; k++) {
                            offsets[k]++;
                        }
                    }
                } while (c != '\n' && flag);
            }
        }
    }
    close(fv);
}

void handleChangePrice(int signal) {
    ArtCache a;
    int fifo_manutencao = open("ma2server", O_RDONLY);
    read(fifo_manutencao, &a, sizeof(ArtCache));
    int posicao = lookForCodigo(a.codigo);
    if (posicao != -1) cache[posicao].preco = a.preco;
    close(fifo_manutencao);
}

void callAgregador(int signal) {
    char timer[128];
    time_t rawtime;
    time(&rawtime);
    struct tm *my_time = localtime(&rawtime);
    if (!strftime(timer, 128, "out/%FT%T", my_time)) puts("strftime failed");
    int fo = open(timer, O_CREAT, 0666);
    int ft = open(TMP_AGREGADOR, O_CREAT | O_TRUNC, 0666);
    close(ft);
    close(fo);
    const unsigned long int lf = getGlobalVariavel(linhaFinalVendas);
    const unsigned long int lua = getGlobalVariavel(linhaUltimaAgregacao);
    int nAgs = ((lf - lua) / 500);  // 500 linhas por agregador
    if (nAgs == 0) nAgs = 1;
    if (lf - lua > 0) {
        int pids[nAgs];
        unsigned long int offsets[nAgs];
        calculaOffsets(offsets, nAgs);
        int pipesLeitura[nAgs][2];
        const unsigned long int pf = getGlobalVariavel(posicaoFinalVendas);
        for (int i = 0; i < nAgs; i++) {
            if ((pids[i] = fork()) == 0) {
                pipe(pipesLeitura[i]);
                if (fork() == 0) {
                    int ft = open(TMP_AGREGADOR, O_WRONLY, 0666);
                    lseek(ft, offsets[i] * 10, SEEK_SET);
                    dup2(ft, 1);
                    close(ft);
                    close(pipesLeitura[i][1]);
                    dup2(pipesLeitura[i][0], 0);
                    close(pipesLeitura[i][0]);
                    execlp("./bin/ag", "./bin/ag", NULL);
                    _exit(0);
                } else {
                    Buffer buffer;
                    char buff[BUFSIZ];
                    int flag = 1;
                    unsigned long int total = offsets[i];
                    size_t r;
                    int fv = open(FILE_VENDAS, O_RDONLY, 0666);
                    lseek(fv, total, SEEK_SET);
                    create_buffer(fv, &buffer, MAX_BUFFER);

                    close(pipesLeitura[i][0]);
                    if (i < nAgs - 1) {
                        while (total < offsets[i + 1] && flag) {
                            r = readln(&buffer, buff, BUFSIZ);
                            if (r == 0)
                                flag = 0;
                            else {
                                write(pipesLeitura[i][1], buff, r);
                                total += r;
                            }
                        }
                        flag = 1;
                    } else {
                        while (total < pf && flag) {
                            r = readln(&buffer, buff, BUFSIZ);
                            if (r == 0)
                                flag = 0;
                            else {
                                write(pipesLeitura[i][1], buff, r);
                                total += r;
                            }
                            flag = 1;
                        }
                    }
                    close(pipesLeitura[i][1]);
                    close(fv);
                    wait(NULL);
                    destroy_buffer(&buffer);
                }
                _exit(0);
            }
        }
        for (int i = 0; i < nAgs; i++) {
            wait(NULL);
        }
        setGlobalVariavel(linhaUltimaAgregacao, lf);
        setGlobalVariavel(posicaoUltimaAgregacao, pf);
        if (fork() == 0) {
            int ft = open(TMP_AGREGADOR, O_RDONLY, 0666);
            int fo = open(timer, O_WRONLY, 0666);
            lseek(ft, 0, SEEK_SET);
            lseek(fo, 0, SEEK_SET);
            dup2(ft, 0);
            close(ft);
            dup2(fo, 1);
            close(fo);
            execlp("./bin/ag", "./bin/ag", NULL);
            _exit(0);
        } else
            wait(NULL);
    }
    unlink(TMP_AGREGADOR);
}

void handleExit(int signal) {
    int tempf = open("tempFile", O_CREAT | O_TRUNC | O_RDWR, 0666);
    int flag = 0;
    LCliente lcliente;
    if (terminado == 0) {
        if (jaLeu == 1) {
            if (lc.operacao == 'c')
                fazConsulta(lc.codigo, lc.nomeCliente);
            else if (lc.operacao == 'a')
                atualizaStock(lc.codigo, lc.quantidade, lc.nomeCliente);
            escreveuStock = 0;
            escreveuVenda = 0;
            escreveuLinhaFinalVendas = 0;
            escreveuPosicaoFinalVendas = 0;
        }
        sleep(1);
        while (read(fif_server, &lc, sizeof(LCliente)) > 0) {
            write(tempf, &lc, sizeof(LCliente));
            flag = 1;
        }
        close(fif_server);
        if (flag == 1) {
            lseek(tempf, 0, SEEK_SET);
            while (read(tempf, &lcliente, sizeof(LCliente))) {
                if (lcliente.operacao == 'c')
                    fazConsulta(lcliente.codigo, lcliente.nomeCliente);
                else if (lcliente.operacao == 'a')
                    atualizaStock(lcliente.codigo, lcliente.quantidade,
                                  lcliente.nomeCliente);
                escreveuStock = 0;
                escreveuVenda = 0;
                escreveuLinhaFinalVendas = 0;
                escreveuPosicaoFinalVendas = 0;
            }
        }

    } else
        close(fif_server);
    close(tempf);
    close(fd_server2ma);
    remove("out/server2ma");
    remove("tempFile");
    // unlink("server");
    exit(0);
}

/**
 * Importante
 * @param argc quantos argumentos
 * */
int main(int argc, char **argv) {
    char *fifo_server = "server";
    char *server_ma = "out/server2ma";
    fd_server2ma = open(server_ma, O_CREAT | O_WRONLY, 0666);

    fd_errors = open(LOG_SERVER, O_CREAT | O_TRUNC | O_WRONLY, 0666);

    pid_t mypid = getpid();
    write(fd_server2ma, &mypid, sizeof(pid_t));
    signal(SIGUSR1, callAgregador);
    signal(SIGTERM, handleExit);
    signal(SIGUSR2, handleChangePrice);
    mkfifo(fifo_server, 0666);

    if ((fif_server = open(fifo_server, O_RDONLY)) == -1) return -1;
    while (1) {
        terminado = 0;

        while (read(fif_server, &lc, sizeof(LCliente)) > 0) {
            jaLeu = 1;
            if (lc.operacao == 'c')
                fazConsulta(lc.codigo, lc.nomeCliente);
            else if (lc.operacao == 'a')
                atualizaStock(lc.codigo, lc.quantidade, lc.nomeCliente);
            escreveuStock = 0;
            escreveuVenda = 0;
            escreveuLinhaFinalVendas = 0;
            escreveuPosicaoFinalVendas = 0;
            jaLeu = 0;
        }

        terminado = 1;
    }
    close(fif_server);
    close(fd_server2ma);
    remove(server_ma);
}
