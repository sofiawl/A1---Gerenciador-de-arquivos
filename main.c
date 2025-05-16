#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "archive.h"


int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <opção> <archive> [membro1 membro2 ...]\n", argv[0]);
        return 1;
    }

    const char *cmd = NULL;
    const char *arc = NULL;
    int estado = 0;

    for (int i = 1; i < argc; i++) {
        switch (estado) {
            case 0:
                cmd = argv[i];
                if (cmd[0] != '-') {
                    fprintf(stderr, "Erro: a opção deve começar com '-'.\n");
                    return 1;
                }
                estado = 1;
                break;

            case 1:
                arc = argv[i];
                {
                    size_t len = strlen(arc);
                    if (len < 4 || strcmp(&arc[len - 3], ".vc") != 0) {
                        fprintf(stderr, "Erro: o nome do archive deve terminar com '.vc'.\n");
                        return 1;
                    }
                }
                estado = 2;

                if (strcmp(cmd, "-c") == 0) {
                    return print_headers(arc);
                }

                if (strcmp(cmd, "-x") == 0 && argc == 3) {
                    extract_file(arc, NULL);
                    return 0;
                }

                break;

            case 2:
                if (strcmp(cmd, "-ip") == 0 || strcmp(cmd, "-p") == 0) {
                    for (int j = i; j < argc; j++) {
                        add_file(arc, argv[j], -1);
                    }
                    return 0;

                } else if (strcmp(cmd, "-ic") == 0 || strcmp(cmd, "-i") == 0) {
                    for (int j = i; j < argc; j++) {
                        add_compress_file(arc, argv[j]);
                    }
                    return 0;

                } else if (strcmp(cmd, "-x") == 0) {
                    for (int j = 3; j < argc; j++) 
                        if (extract_file(arc, argv[j]) != 0)  // extrai os listados
                            return 1;

                    return 0;

                } else if (strcmp(cmd, "-r") == 0) {
                    for (int j = i; j < argc; j++) {
                        remove_file(arc, argv[j]);
                    }
                    return 0;

                } else if (strcmp(cmd, "-m") == 0) {
                    if (argc - i != 2) {
                        fprintf(stderr, "Uso: %s -m <archive> <membro> <target>\n", argv[0]);
                        return 1;
                    }

                    return move_file(arc, argv[i], argv[i+1]);
                } else {
                    fprintf(stderr, "Opção inválida: %s\n", cmd);
                    return 1;
                }

                break;
        }
    }

    return 0;
}
