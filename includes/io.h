#ifndef __IO__
#define __IO__

#define MAX_BUFFER 4096

typedef struct linhaCliente {
    char operacao;
    unsigned long int codigo;
    long int quantidade;
    char nomeCliente[128];
} LCliente;

typedef struct linhaAtualiza {
    unsigned long int codigo;
    unsigned long int stock;
} LinhaAtualiza;

typedef struct linhaConsulta {
    unsigned long int stock;
    float preco;
} LinhaConsulta;

typedef struct buffer_t {
    int fd;           // file descriptor
    int size;         // number of elements read
    int pt;           // pointer to next char to read
    size_t capacity;  // capacity of body
    char *body;
} Buffer;

void create_buffer(int fd, Buffer *buffer, size_t initial_capacity);

void destroy_buffer(Buffer *buffer);

ssize_t readln(Buffer *buffer, void *buf, size_t nbyte);

int contaEspacos(char *string);

#endif
