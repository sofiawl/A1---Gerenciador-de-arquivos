# Makefile - Projeto Vinac
# Sofia Wamser Lima 

CC      = gcc
CFLAGS  = -Wall -Wextra -g -std=c99
LDLIBS  = -lm
MAIN    = vinac
BIN_DIR = swl24
ENTREGA = swl24

# arquivos de cabeçalho
HDR = archive.h lz.h

# arquivos-objeto
OBJ = main.o archive.o lz.o

# regra padrão
all: $(BIN_DIR)/$(MAIN)

# gerar o executável na pasta swl24/
$(BIN_DIR)/$(MAIN): $(OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# dependências dos objetos
main.o: main.c $(HDR)
archive.o: archive.c archive.h 
lz.o: lz.c lz.h

# criar diretório do executável se não existir
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# executar
run: $(BIN_DIR)/$(MAIN)
	./$(BIN_DIR)/$(MAIN)

# gerar arquivo TGZ para entrega
tgz: clean
	-mkdir -p /tmp/$(USER)/$(ENTREGA)
	chmod 0700 /tmp/$(USER)/$(ENTREGA)
	cp *.c *.h makefile /tmp/$(USER)/$(ENTREGA)
	tar czvf $(ENTREGA).tgz -C /tmp/$(USER) $(ENTREGA)
	rm -rf /tmp/$(USER)
	@echo "Arquivo $(ENTREGA).tgz criado para entrega"

# limpar arquivos temporários
clean:
	rm -f *~ *.o $(BIN_DIR)/$(MAIN) $(ENTREGA).tgz
