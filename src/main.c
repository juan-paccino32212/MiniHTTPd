#include <stdio.h>
#include "server.h"

int main(void) {
    printf("Iniciando servidor nimiHTTPd...\n");

    if (server_start() == -1) {
        fprintf(stderr, "Error al iniciar el servidor.\n");
        return 1;
    }
    return 0;
}
