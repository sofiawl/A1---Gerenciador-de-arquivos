#ifndef archive
#define archive

typedef struct __attribute__((packed))  {
    char name[100];         // Nome do arquivo (não-constante, modificado dinamicamente)
    int uid;                // UID do dono do arquivo
    int size_original;   // Tamanho original do arquivo
    int size_disk;       // Tamanho do arquivo no archive
    int mod_time;        // Data de modificação 
    long int offset;             // Offset no arquivo archive
    int place;              // Posição na ordem de inserção
    unsigned int from_disk; // Se veio do disco (1) ou do archive (0)
} file_info;


/*-ip : insere/acrescenta um ou mais membros sem compressão ao archive. Caso
o membro já exista no archive, ele deve ser substituído. Novos membros são
inseridos respeitando a ordem da linha de comando, ao final do archive;*/
int add_file(const char *arc, const char *new_file, int old_size);

/*-ic : insere/acrescenta um ou mais membros com compressão ao archive. Caso
o membro já exista no archive, ele deve ser substituído. Novos membros são
inseridos respeitando a ordem da linha de comando, ao final do archive;
*/
int add_compress_file(const char *arc, const char *new_file);

/*-m membro : move o membro indicado na linha de comando para imediatamente
depois do membro target existente em archive. A movimentação deve ocorrer
na seção de dados do archive;*/
int move_file(const char *member, const char *target, const char *arc);

/*-x : extrai os membros indicados de archive. Se os membros não forem
indicados, todos devem ser extraídos. A extração consiste em ler o membro
de archive e criar um arquivo correspondente, com conteúdo idêntico, em
disco;*/
int extract_file(const char *arc, const char *target_name);

/*-r : remove os membros indicados de archive;*/
int remove_file(const char *arc, const char *member);

/*-c : lista o conteúdo de archive em ordem, incluindo as propriedades de
cada membro (nome, UID, tamanho original, tamanho em disco e data de
modificação) e sua ordem no arquivo.*/
int print_headers(const char *arc);

#endif
