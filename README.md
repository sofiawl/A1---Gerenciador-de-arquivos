# A1
Gerenciador de arquivos para a matéria Programação2.
O pacote de software gera o executável chamado
vina, que deve ser executado da seguinte forma:
vinac <opção> <archive> [membro1 membro2 ...]
Onde a opção pode ser:

-ip (ou -p) : insere/acrescenta um ou mais membros sem compressão ao
archive. Caso o membro já exista no archive, ele deve ser substituído.
Novos membros são inseridos respeitando a ordem da linha de comando, ao
final do archive;

-ic (ou -i) : insere/acrescenta um ou mais membros com compressão ao
archive. Caso o membro já exista no archive, ele deve ser substituído.
Novos membros são inseridos respeitando a ordem da linha de comando, ao
final do archive;

-m membro : move o membro indicado na linha de comando para imediatamente
depois do membro target existente em archive. A movimentação deve ocorrer
na seção de dados do archive; para mover para o início, o target deve ser
NULL;

-x : extrai os membros indicados de archive. Se os membros não forem
indicados, todos devem ser extraídos. A extração consiste em ler o membro
de archive e criar um arquivo correspondente, com conteúdo idêntico, em
disco;

-r : remove os membros indicados de archive;
