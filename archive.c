#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include "lz.h"
#include "archive.h"
 
/*Recalcula offsets*/
void computer_offsets(int total_infos, file_info *infos){
	// Início da área de dados
    long current_offset = sizeof(int) + total_infos * sizeof(file_info); 

	// Atualiza offsets na struct
    for (int i = 0; i < total_infos; i++) {
		infos[i].place = i;
        infos[i].offset = current_offset;
        current_offset += infos[i].size_disk;
    }
}

/*Reescreve dados no archiver*/ 
int reset_archive(FILE *fp_out, FILE *fp_original, file_info *infos, int total_infos) {

	// Descobre o tamanho do maior membro
    long max_member_size = 0;
    for (int i = 0; i < total_infos; i++) {
        if (infos[i].size_disk > max_member_size)
            max_member_size = infos[i].size_disk;
    }

	// Aloca buffer com o tamanho do maior membro
    unsigned char *buffer = malloc(max_member_size);
    if (!buffer) {
        perror("Erro ao alocar buffer fixo");
        return -1;
    }

	rewind(fp_out);
	fwrite(&total_infos, sizeof(int), 1, fp_out);

	// Salva posição atual
	long after_header = ftell(fp_out);

	// Volta onde parou
	fseek(fp_out, after_header, SEEK_SET);

	// Agora sim, escreve o vetor infos corretamente
	fwrite(infos, sizeof(file_info), total_infos, fp_out);

	// 
    for (int i = 0; i < total_infos; i++) {
        FILE *source = NULL;

		// No caso de ler o novo arquivo
        if (infos[i].from_disk) {
            source = fopen(infos[i].name, "rb");
            if (!source) {
                perror("Erro ao abrir membro original");
                free(buffer);
                return -1;
            }
		// Nos caso de ler os outros arquivos
        } else {
            source = fp_original;
            if (fseek(source, infos[i].offset, SEEK_SET) != 0) {
                perror("Erro ao posicionar leitura no archive original");
                free(buffer);
                return -1;
            }
        }

		// Escreve os dados no archiver
        long total_read = 0;
        while (total_read < infos[i].size_disk) {
			// Até onde deve ler
            size_t to_read = infos[i].size_disk - total_read;
			// Lê até to_read de source e escreve em buffer, chunck recebe tamanho da info
            size_t chunk = fread(buffer, 1, to_read, source); 
            if (chunk == 0 && ferror(source)) {
                perror("Erro ao ler dados");
                if (infos[i].from_disk) fclose(source);
                free(buffer);
                return -1;
            }
			// Escreve o que está no buffer até chunck no arq de saída
            fwrite(buffer, 1, chunk, fp_out);
            total_read += chunk;
        }

        if (infos[i].from_disk)
            fclose(source);
    }

    free(buffer);

    return 0;
}

/*Lê o diretório*/
int read_header(FILE *fp, int *total_infos, file_info **infos) {

	rewind(fp);
	// Primeiro valor é a quantidade de arquivos 
	if (fread(total_infos, sizeof(int), 1, fp) != 1 || *total_infos < 0 || *total_infos > 10000) {
		fprintf(stderr, "Arquivo corrompido ou total_infos inválido: %d\n", *total_infos);
		return -1;
	}

	// Aloca um vetor para informações dos arquivos
	*infos = malloc(sizeof(file_info) * (*total_infos + 1));
	if (!*infos) {
		perror("Erro ao alocar vetor de diretórios");
		return -1;
	}

	// Adiciona informações no vetor
	for (int i = 0; i < *total_infos; i++) {
		(*infos)[i].from_disk = 0;
		if (fread(&(*infos)[i], sizeof(file_info), 1, fp) != 1) {
			fprintf(stderr, "Erro ao ler header do archive\n");
			free(*infos);
			return -1;
		}
	}

		return 0;
}

/*-ip : insere/acrescenta um ou mais membros sem compressão ao archive. Caso
o membro já exista no archive, ele deve ser substituído. Novos membros são
inseridos respeitando a ordem da linha de comando, ao final do archive;*/
int add_file(const char *archiver, const char *new_file, int old_size){
    FILE *fp_new = fopen(new_file, "rb");
    if (!fp_new){
        perror("Erro ao abrir o arquivo de entrada");
        return -1;
    }

	// Adquire informações sobre novo arquivo
    struct stat statbuf;
    if (stat(new_file, &statbuf) == -1){
        perror("Erro ao obter informações do arquivo de entrada");
        fclose(fp_new);
        return -1;
    }

	// Se o archiver não existir cria ele, senão abre ele
    int new_arc = 0;
    FILE *arc = fopen(archiver, "rb+");
    if (!arc){
        arc = fopen(archiver, "wb+");
        new_arc = -1;
        if (!archiver){
            perror("Erro ao abrir arquivo de saída");
            fclose(fp_new);
            return -1;
        }
    }

    int total_infos = 0;
    file_info *infos = NULL;

	// Se o archiver não foi criado agora, lê diretório
    if (!new_arc) {
        if (read_header(arc, &total_infos, &infos) != 0){
            fclose(fp_new);
            fclose(arc);
            return -1;
        }
    } else {
		// Se o archiver foi criado agora
        int zero = 0;
        fwrite(&zero, sizeof(int), 1, arc);
        infos = malloc(sizeof(file_info));
        if (!infos) {
            perror("Erro ao alocar infos");
            fclose(fp_new);
            fclose(arc);
            return -1;
        }
    }

	// Verifica se já não existe o mesmo arquivo
    int found_index = -1;
    for (int i = 0; i < total_infos; i++){ 
        if (strncmp(infos[i].name, new_file, sizeof(infos[i].name)) == 0){
            found_index = i;
            break;
        }
    }

	// Atualiza ou cria as informações do novo arquivo
    file_info new_info;
    memset(&new_info, 0, sizeof(new_info));
    strncpy(new_info.name, new_file, sizeof(new_info.name) - 1);
    new_info.uid = statbuf.st_uid;
    new_info.size_disk = statbuf.st_size;
    new_info.size_original = (old_size == -1) ? new_info.size_disk : old_size;
    new_info.mod_time = statbuf.st_mtime;
	new_info.from_disk = 1;

    if (found_index >= 0) {
		// Se já existe o arquivo, atualiza ele
        new_info.place = found_index;
    } else {
		// Se não existe, aumenta o tamanho do vetor de infos
        new_info.place = total_infos;
        file_info *tmp = realloc(infos, sizeof(file_info) * (total_infos + 1));
        if (!tmp) {
            perror("Erro ao realocar infos");
            fclose(fp_new);
            fclose(arc);
            free(infos);
            return -1;
        }
        infos = tmp;
        total_infos++;
    }

	// Adiciona informações atualizadas ao vetor
    infos[new_info.place] = new_info;

	computer_offsets(total_infos, infos);

	// Abre o archiver para escretiva
	fclose(fp_new);
    arc = freopen(archiver, "wb+", arc);
    if (!arc){
        perror("Erro ao reabrir archive");
        free(infos);
        return -1;
    }

	// Sobrescreve tudo em archiver
    if (reset_archive(arc, NULL, infos, total_infos) != 0){
        perror("Erro ao resetar archive");
        fclose(arc);
        free(infos);
        return -1;
    }

    fclose(arc);
    free(infos);

    printf("Arquivo \"%s\" %s no archive.\n", new_file, (found_index >= 0 ? "atualizado" : "adicionado"));

    return 0;
}

/*-ic : insere/acrescenta um ou mais membros com compressão ao archive. Caso
o membro já exista no archive, ele deve ser substituído. Novos membros são
inseridos respeitando a ordem da linha de comando, ao final do archive;
*/
int add_compress_file(const char *archiver, const char *new_file){
    FILE *fp_new = fopen(new_file, "rb");
    if (!fp_new) {
        perror("Erro ao abrir o arquivo de entrada");
        return -1;
    }

	// Descobre o tamanho do novo arquivo
    fseek(fp_new, 0, SEEK_END);
    int file_size = ftell(fp_new);
    fseek(fp_new, 0, SEEK_SET);

	// Buffer para guardar dados do novo arquivo
    unsigned char *in_buf = (unsigned char *)malloc(file_size + 1);
    if (!in_buf){
        perror("Erro ao alocar memória para o buffer de entrada");
        fclose(fp_new);
        return -1;
    }

	memset(in_buf, 0, file_size + 1); 

    size_t read_bytes = fread(in_buf, 1, file_size, fp_new);
    fclose(fp_new);

    if (read_bytes != (size_t)file_size) {
        fprintf(stderr, "Erro na leitura do arquivo: esperado %d bytes, lido %zu bytes\n", file_size, read_bytes);
        free(in_buf);
        return -1;
    }

	// Buffer para guardar dados comprimidos do novo arquivo
    unsigned char *comp_buf = (unsigned char *)malloc(file_size);
    if (!comp_buf){
        perror("Erro ao alocar memória para o buffer de compressão");
        free(in_buf);
        return -1;
    }

	// Comprimi arquivo e descobre tamanho da versão comprimida
    int comp_size = LZ_Compress(in_buf, comp_buf, file_size);
    if (comp_size <= 0){
        perror("Erro na compressão do arquivo");
        free(in_buf);
        free(comp_buf);
        return -1;
    }


    // Se arquivo comprimido não for menor que o original, adiciona original
    if (comp_size >= file_size){
        free(in_buf);
        free(comp_buf);
        add_file(archiver, new_file, -1);
        return 0;
    }

    // Sobrescreve o próprio arquivo com os dados comprimidos
    FILE *fp_overwrite = fopen(new_file, "wb");
    if (!fp_overwrite){
        perror("Erro ao sobrescrever o arquivo com versão comprimida");
        free(in_buf);
        free(comp_buf);
        return -1;
    }

    fwrite(comp_buf, 1, comp_size, fp_overwrite);
    fclose(fp_overwrite);

    // Adiciona ao arquivo de archive usando nome original
    if (add_file(archiver, new_file, file_size) != 0) {
        fprintf(stderr, "Erro ao adicionar arquivo comprimido\n");
        return -1;
    }

	// Sobrescreve dados descomprimidos no novo arquivo 
    fp_overwrite = fopen(new_file, "wb");
    if (!fp_overwrite){
        perror("Erro ao sobrescrever o arquivo com versão descomprimida");
        free(in_buf);
        free(comp_buf);
        return -1;
    }

    fwrite(in_buf, 1, file_size, fp_overwrite);
    fclose(fp_overwrite);


    free(in_buf);
    free(comp_buf);

    return 0;
}

/*-m membro : move o membro indicado na linha de comando para imediatamente
depois do membro target existente em archive. A movimentação deve ocorrer
na seção de dados do archive;*/
int move_file(const char *archiver, const char *membro, const char *target) {
    FILE *fp_original = fopen(archiver, "rb");
    if (!fp_original) {
        perror("Erro ao abrir archive para leitura");
        return -1;
    }

    int total_infos = 0;
    file_info *infos = NULL;
    if (read_header(fp_original, &total_infos, &infos) != 0) {
        fclose(fp_original);
        return -1;
    }

    // Encontra os índices
    int idx_membro = -1, idx_target = -1;
    for (int i = 0; i < total_infos; i++) {
        if (strcmp(infos[i].name, membro) == 0)
            idx_membro = i;
        if (strcmp(infos[i].name, target) == 0)
            idx_target = i;
    }

    if (idx_membro == -1 || idx_target == -1) {
        fprintf(stderr, "Erro: membro ou target não encontrados no archive.\n");
        free(infos);
        fclose(fp_original);
        return -1;
    }

    if (idx_membro == idx_target + 1) {
        printf("Membro já está na posição correta. Nenhuma alteração feita.\n");
        free(infos);
        fclose(fp_original);
        return 0;
    }

    // Reorganiza o vetor infos
    file_info moved = infos[idx_membro];
    if (idx_membro < idx_target) {
        for (int i = idx_membro; i < idx_target; i++)
            infos[i] = infos[i + 1];
        infos[idx_target] = moved;
    } else {
        for (int i = idx_membro; i > idx_target; i--)
            infos[i] = infos[i - 1];
        infos[idx_target + 1] = moved;
    }

    // Reabre o arquivo original para escrita em modo leitura-escrita
    FILE *fp_rw = fopen(archiver, "r+b");
    if (!fp_rw) {
        perror("Erro ao reabrir archive para sobrescrita");
        free(infos);
        fclose(fp_original);
        return -1;
    }

    // Recalcula offsets
    computer_offsets(total_infos, infos);

    // Regrava os dados com a nova ordem
    if (reset_archive(fp_rw, fp_original, infos, total_infos) != 0) {
        fprintf(stderr, "Erro ao sobrescrever o archive.\n");
        free(infos);
        fclose(fp_original);
        fclose(fp_rw);
        return -1;
    }

    printf("Membro \"%s\" movido com sucesso após \"%s\".\n", membro, target);

    free(infos);
    fclose(fp_original);
    fclose(fp_rw);
    return 0;
}


/*-x : extrai os membros indicados de archive. Se os membros não forem
indicados, todos devem ser extraídos. A extração consiste em ler o membro
de archive e criar um arquivo correspondente, com conteúdo idêntico, em
disco;*/
int extract_file(const char *archiver, const char *target_name) {
    FILE *arc = fopen(archiver, "rb");
    if (!arc) {
        perror("Erro ao abrir o archive");
        return -1;
    }

    int total_infos = 0;
    file_info *infos = NULL;

    if (read_header(arc, &total_infos, &infos) != 0) {
        fclose(arc);
        return -1;
    }

    int found = 0;
    for (int i = 0; i < total_infos; i++) {
        if (target_name && strncmp(infos[i].name, target_name, sizeof(infos[i].name)) != 0) {
            continue; // Pula se não for o membro desejado
        }

        found = 1;

        // Aloca buffer para ler os dados compactados ou não
        unsigned char *buffer = malloc(infos[i].size_disk);
        if (!buffer) {
            perror("Erro ao alocar buffer de leitura");
            free(infos);
            fclose(arc);
            return -1;
        }

        fseek(arc, infos[i].offset, SEEK_SET);
        int read_bytes = fread(buffer, 1, infos[i].size_disk, arc);
        if (read_bytes != infos[i].size_disk) {
            fprintf(stderr, "Erro: leitura incompleta de %s\n", infos[i].name);
            free(buffer);
            free(infos);
            fclose(arc);
            return -1;
        }

        // Cria o arquivo de saída
        FILE *out = fopen(infos[i].name, "wb");
        if (!out) {
            perror("Erro ao criar arquivo extraído");
            free(infos);
            fclose(arc);
            return -1;
        }

        fwrite(buffer, 1, infos[i].size_disk, out);
        fclose(out);


        free(buffer);
        // Se só queria um arquivo, loop acaba
        if (target_name != NULL) break;
    }

    free(infos);
    fclose(arc);

    if (!found) {
        fprintf(stderr, "Arquivo '%s' não encontrado no archive\n", target_name);
        return -1;
    }

    printf("Extração concluída com sucesso.\n");
    return 0;
}

/*-r : remove os membros indicados de archive;*/
int remove_file(const char *archiver, const char *member){
	FILE *arc = fopen(archiver, "rb");
	if (!arc){
		perror("Erro ao abrir archive");
		return -1;
	}

	// Lê diretório (lista de descrição de membros na ordem em que foram inseridos)
	int total_infos = 0;
	file_info *infos;
	if (read_header(arc, &total_infos, &infos) != 0){
		fclose(arc);
		return -1;
	}


	// Verifica se há o arquivo em archive e qual é a sua posição
	int found_index = -1;	
	for (int i = 0; i < total_infos; i++) //copy with limited size(n)
		if (strncmp(infos[i].name, member, sizeof(infos[i].name)) == 0)
			found_index = i;

	// Tratamento para arquivos não encontrados
	if (found_index == -1){
		fprintf(stderr, "%s não encontrado em archive\n", member);
		fclose(arc);
		return -1;
	}

	// Reorganiza vetor sem member	
	for (int i = found_index; i < total_infos - 1; i++)
		infos[i] = infos[i + 1];
	total_infos--;

	// Reabre archive para sobreescrever tudo(muda a mode)
	FILE *fp_rw = fopen(archiver, "w+b"); //redirects an archive for a FILE 
	if (!fp_rw){
		perror("Erro ao reabrir arquivo para sobrescrita");
		return -1;
	}

	computer_offsets(total_infos, infos);

	rewind(arc);

	if (reset_archive(fp_rw, arc, infos, total_infos) != 0){
		perror("Erro ao resetar archive");
		fclose(fp_rw);
		return -1;
	}

	fclose(arc);
	fclose(fp_rw);
	free(infos);
	printf("Arquivo \"%s\" foi removido do archive.\n", member);
	
	return 0;
}

/*-c : lista o conteúdo de archive em ordem, incluindo as propriedades de
cada membro (nome, UID, tamanho original, tamanho em disco e data de
modificação) e sua ordem no arquivo.*/
int print_headers(const char *archiver){
	FILE *arc = fopen(archiver, "rb");
	if (!arc){
		perror("Erro ao abrir archive");
		return -1;
	}

	// Lê diretório (lista de descrição de membros na ordem em que foram inseridos)
	int total_infos = 0;
	file_info *infos;
	if (read_header(arc, &total_infos, &infos) != 0){
		perror("Não há membros no archiver");
		fclose(arc);
		return -1;
	} 

	printf("\n==== LISTA DE ARQUIVOS NO ARCHIVE %s (n de arquivos, %d)====\n\n", archiver, total_infos);
	printf("%-20s %-6s %-10s %-10s %-20s %-10s %-6s\n",
       "NOME", "UID", "ORIGINAL", "DISCO", "MODIFICADO", "OFFSET", "ORDEM");
	printf("%-20s %-6s %-10s %-10s %-20s %-10s %-6s\n",
       "--------------------", "------", "----------", "----------", "--------------------", "----------", "------");

	for (int i = 0; i < total_infos; i++) {
    	char time_str[20];
    	time_t mod_time = infos[i].mod_time;
    	struct tm *tm_info = localtime(&mod_time);
    	strftime(time_str, sizeof(time_str), "%Y/%m/%d %H:%M:%S", tm_info);

    	printf("%-20s %-6d %-10d %-10d %-20s %-10ld %-6d\n",
           infos[i].name,
           infos[i].uid,
           infos[i].size_original,
           infos[i].size_disk,
           time_str,
           infos[i].offset,
           infos[i].place);
	}

	free(infos);
	fclose(arc);
	return 0;
}
