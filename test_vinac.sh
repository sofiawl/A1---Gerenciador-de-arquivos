#!/bin/bash
set -e

# Corrige: nomes reais dos arquivos adicionados ao archive devem ser extraídos corretamente

echo "[1] Criando arquivos de teste..."
echo "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" > a.txt
echo "BB" > b.txt
echo "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC" > c.txt
echo "D" > d.txt

echo "[2] Limpando archive antigo..."
rm -f test.vc

echo "[3] Inserindo arquivos com -ip..."
./swl24/vinac -ip test.vc a.txt b.txt

echo "[4] Listando com -c..."
./swl24/vinac -c test.vc

echo "[5] Inserindo c.txt e d.txt com compressão -ic..."
./swl24/vinac -ic test.vc c.txt d.txt

echo "[6] Listando novamente..."
./swl24/vinac -c test.vc

echo "[7] Extraindo apenas b.txt e c.txt..."
rm -f b.txt c.txt
./swl24/vinac -x test.vc b.txt c.txt
cat b.txt 
cat c.txt

echo "[8] Removendo a.txt..."
./swl24/vinac -r test.vc a.txt
./swl24/vinac -c test.vc

echo "[9] Movendo d.txt para depois de b.txt..."
./swl24/vinac -m test.vc d.txt b.txt
./swl24/vinac -c test.vc

echo "[10] Extraindo tudo..."
rm -f *.txt 
./swl24/vinac -x test.vc
ls -l *.txt 

echo "Todos os testes passaram com sucesso."
