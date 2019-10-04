## Visão Global do Projeto
Desenvolver um sistema de ficheiros chamado TecnicoFS
Funções básicas:
- criar
- pesquisar
- remover ficheiro
A diretoria é mantida numa BST (*Binary Search Tree*), em que cada nó representa um ficheiro:
- chave do nó: nome do ficheiro (tipo *String*)
- valor mantido no nó: i-number (tipo *int*) - gerados sequencialmente, havendo um **contador de i-numbers**
---
## Etapas de Desenvolvimento
#### [Exercício 1](#exercício-1)
Desenvolver o servidor do TecnicoFS, composto por uma pool de tarefas escravas. Simplificações:
 - restringe-se o paralelismo efetivo na sincronização dos acessos à diretoria partilhada 
 - as chamadas das funções do TecnicoFS são simuladas pelo carregamento de um ficheiro com a sequência de comandos (e não são feitas por parte do utilizador)

#### [Exercício 2](#exercício-2)
 - maior paralelismo efetivo
 - permitir que as operações se executem em paralelo com o carregamento inicial do ficheiro

 #### [Exercício 3](#exercício-3)
 - operações feitas pelo utilizador
 - os processos cliente fazem chamadas ao TecnicoFS enviando mensagens através de um socket
 - nova funcionalidade: salvaguardar o conteudo da diretoria do novo FS num FS externo

#### Etapa Final (não avaliada)
Adaptação do projeto para que as chamadas sejam feitas através do api (*Application Program Interface*) e passem pelo Gestor de FS do Sistema Operativo

---
## Exercício 1
>"Para efeitos de depuração, quando o servidor terminar, este deve também apresentar o tempo total
de execução no stdout e escrever o conteúdo final da diretoria num ficheiro de saída"

O programa do servidor deve chamar-se ***tecnicofs*** e receber os seguintes argumentos de linha de comando:

    tecnicofs inputfile outputfile numthreads

#### Carregamento do ficheiro de entrada
>**`inputfile`** (primeiro argumento): *pathname* de um ficheiro de entrada, que contem uma sequência de comandos

Comandos:
 - **comando 'c'**: adiciona à diretoria uma nova entrada, cuja chave é o nome indicado em argumento
 - **comando 'l'**: pesquisa a diretoria por uma entrada com o nome indicado em argumento
 -**comando 'd'**: apaga da diretoria a entrada com o nome indicado em argumento

>Linhas começadas com **'#'** são comentários, a ser ingorados pelo programa

<details><summary>Exemplo do ficheiro de entrada</summary>

    # isto é um exemplo
    c f.txt
    c s.exe
    l f.txt
    d s.exe
</details>

Funcionamento do programa:
1. uma vez iniciado, o servidor deve criar a diretoria, vazia
2. abre o ficheiro de entrada e carrega os comandos para um vetor em memória - **tamanho máximo 150mil entradas** (definido numa constante)
3. o carregamento termina quando é atingido o *end of file* ou o vetor seja totalmente preenchido

#### Paralelização do servidor
`numthreads` (3º argumento) define o número de tarefas escravas. O servidor deverá ser capaz de executar operações em paralelo. 

Cada tarefa escrava deve aceder ao vetor de chamadas, retirar o próximo elemento que esteja
pendente e executá-lo sobre a diretoria partilhada. O vetor de chamadas, o contador de i-numbers e
a diretoria são partilhados, pelo que devem ser **sincronizados** adequadamente.

>"Se duas chamadas de tipo ‘c’
surgem no vetor numa dada ordem, os i-numbers gerados para cada entrada devem também refletir
a ordem no vetor."

Em relação à sincronização dos acessos à diretoria, 2 estratégias a implementar (escolhidas aquando da compilação)
1. *pthread_mutex* (*-DMUTEX* no compilador)
2. *pthread_rw_lock* (*-DRWLOCK* no compilador)

Caso não seja passada nenhuma das flags, a sincronização é desativada e nesse caso *numthreads* = 1.

#### Terminação do servidor
1. imprimir no *stdout* o tempo total de execução (desde que a *pool* de tarefas foi inicializada):
    ```
    TecnicoFS completed in [duration] seconds.
    ```
    em que o tempo é medido em segundos com 4 casas decimais, por exemplo:
    ```
    TecnicoFS completed in 2.5897 seconds.
    ```
2. o conteúdo final da diretoria deve ser exportado para um ficheiro, indicado em `outputfile` (2º argumento). Caso o ficheiro já exista, o seu conteúdo deve ser truncado.

#### Entrega e avaliação
 - submeter o ficheiro no formato *zip* com a respetiva *Makefile* 
    - o comando *make* deve gerar 3 executáveis: `tecnicofs-nosync`, `tecnicofs-mutex` e `tecnicofs-rwlock`, correspondentes a cada estratégia de sincronização)
    - o comando *make clean* deve limpar todos os ficheiros resultantes da compilação do projeto




---
## Exercício 2

---
## Exercício 3
