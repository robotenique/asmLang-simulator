---
title:  Orientações -projeto de MAC0216-
author: "Juliano Garcia de Oliveira e Enzo Hideki Nakamura e Allan Rocha"
geometry: margin=3cm
date: "11 de Dezembro, 2016"
---

----------------------------------------------------------------------------------------------

###Utilização

Para rodar o programa:

No terminal, usar **$ make asm**, e depois rodar com  **$ ./parser 'nomeArquivo'**.


Usar **$ make clean** para remover os arquivos binários / objetos.

Para rodar o programa macas:

No terminal, usar **$ make macas**, e depois rodar com  **$ ./macas 'nomeArquivo'**.
Isso faz com que ele gere um arquivo.maco.

Para rodar o programa maclk:

No terminal, usar **$ ./maclk out main.maco arq1.maco arq2.maco ...**, onde "out" é o arquivo de saída do programa.

###Comentários

Os arquivos principais implementados são o *parser.c*, o *parserTest.c* e o *defaultops.c*. Para melhor leitura do código, é recomendado um bom editor como o **Atom**, que permite executar *code folding* rapidamente. Por exemplo, selecionar todo o código e depois usar *ctrl + alt + shift + [* para dar *folding* em todo o arquivo, e depois ir de função em função.

O parser implementado verifica, além dos erros usuais, o erro quando a *label* do **EXTERN** não é definida no programa, como o **./macas** também faz. O parser que implementamos também detecta erros que o **./macas** não detectava, como por exemplo a linha **SUB $1,$1, 12 222 3 3 23**, que claramente está errada mas o **./macas** ignora.

A tabela de símbolos foi trocada para uma implementação usando vetores (a mesma enviada anteriormente).

O vinculador funciona restrito a seguinte especificação: o primeiro e o segundo argumento devem ser, respectivamente, o arquivo de saída e o arquivo onde se encontra a main.

###GRUPO ANTIGO

O integrante Allan Rocha fazia parte de um grupo com o Igor Fratel e o Yuji Murata. Devido a desmoronamento do grupo, o Allan Rocha se integrou ao grupo do Juliano Garcia e Enzo Hideki.

----------------------------------------------------------------------------------------------
