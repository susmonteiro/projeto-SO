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
#### [Exercício n1](#exercício-1)
Desenvolver o servidor do TecnicoFS, composto por uma pool de tarefas escravas. Simplificações:
 - restringe-se o paralelismo efetivo na sincronização dos acessos à diretoria partilhada 
 - as chamadas das funções do TecnicoFS são simuladas pelo carregamento de um ficheiro com a sequência de comandos (e não são feitas por parte do utilizador)

#### [Exercício n2](#exercício-2)
 - maior paralelismo efetivo
 - permitir que as operações se executem em paralelo com o carregamento inicial do ficheiro

 #### [Exercício n3](#exercício-3)
 - operações feitas pelo utilizador
 - os processos cliente fazem chamadas ao TecnicoFS enviando mensagens através de um socket
 - nova funcionalidade: salvaguardar o conteudo da diretoria do novo FS num FS externo

#### Etapa Final (não a
valiada)
Adaptação do projeto para que as chamadas sejam feitas através do api (*Application Program Interface*) e passem pelo Gestor de FS do Sistema Operativo

---
## Exercício 1
>"Para efeitos de depuração, quando o servidor terminar, este deve também apresentar o tempo total
de execução no stdout e escrever o conteúdo final da diretoria num ficheiro de saída"

O programa do servidor deve chamar-se ***tecnicofs*** e receber os seguintes argumentos de linha de comando:

    tecnicofs inputfile outputfile numthreads

#### Carregamento do ficheiro de entrada stdout
>**`inputfile`** (primeiro argumento): *pathname* de um ficheiro de entrada, que contem uma sequência de comandos

insertCommand
Comandos:
 - **comando 'c'**: adiciona à diretoria uma nova entrada, cuja chave é o nome indicado em argumento
 - **comando 'l'**: pesquisa a diretoria por uma entrada com o nome indicado em argumento
 -**comando 'd'**: apaga da diretoria a entrada com o nome indicado em argumento

>Linhas começadas com **'#'** são comentários, a ser ignorados pelo programa

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
2. o conteúdo final da diretoria deve ser exportado para um ficheiro, stdout indicado em `outputfile` (2º argumento). Caso o ficheiro já exista, o seu conteúdo deve ser truncado.

#### Entrega e avaliação
 - submeter o ficheiro no formato *zip* com a respetiva *Makefile* 
    - o comando *make* deve gerar 3 executáveis: `tecnicofs-nosync`, `tecnicofs-mutex` e `tecnicofs-rwlock`, correspondentes a cada estratégia de sincronização)
    - o comando *make clean* deve limpar todos os ficheiros resultantes da compilação do projeto




---
## Exercício 2
Argumentos da linha de comandos 

        tecnicofs inputfile outputfile numthreads numbuckets

#### Execução incremental de comandos
- carregamento dos comandos em paralelo com a sua execução (em vez de ser feito em duas fases)
- caso uma tarefa escrava esteja livre e o vetor de comandos não tenha nenhum comando,esta deve aguardar até que surja novo comando no vetor ou até que o final do ficheiro de entrada seja alcançado
- na nova solução, quando o vetor de comandos estiver totalmente preenchido, a tarefa que o preenche deve esperar por que novas posições sejam libertadas 
- a dimensão do vetor de comandos deve passar a ser 10
- o argumento de linha de comandos ​numthreads determina o número de tarefas escravas, excluindo a tarefa que carrega o vetor de comandos
- o tempo deve passar a ser medido desde o momento em que o vetor começa a ser carregado 

#### Nova operação: renomear ficheiro
Esta operação recebe dois argumentos: *nome atual* e *novo nome*. Como tal, é necessário "apagar" o ficheiro da diretoria e voltar a inseri-la (o *inumber* mantém-se). 
Para tal, é necessário verificar se o ficheiro com *nome atual* existe e se não existe já nenhum ficheiro com o nome *novo nome* no fs (em caso de erro, cancelar a operação e não devolver erro).
> temos que verificar se o *novo nome* já existe antes de remover o *nome atual* da diretoria

Esta operação é suportada pelo comando 'r'

        r f1 f2 

Sendo *f1* o *nome atual* e *f2* o *novo nome*

#### Shell script

Desenvolver um *shell script* chamado *runTests.sh* para avaliar o desempenho do TecnicoFS.

---
## Exercício 3

#### *I-nodes* e conteudos dos ficheiros

Tecnicofs passa a manter uma tabela de *i-nodes*

Cada ficheiro passa a ter um dono (UID) e a cada ficheiro estao associadas permissoes de leitura e escrita, sendo que sao definidas no momento de criacao do ficheiro e as **permissoes nao podem ser alteradas**

O ficheiro tem conteudo (string com '\0' no fim), sendo um ponteiro para este conteudo guardado no *i-node*

Todos estes elementos sao guardados no *i-node* do ficheiro

#### Comunicacao com processos clientes

O servidor TecnicoFS deixa de carregar comandos a partir de ficheiro. Em vez disso, passa a ter um
socket Unix do tipo stream, através do qual recebe e aceita ligações solicitadas por outros processos,
que designamos de processos cliente.

Argumentos da linha de comandos:

        tecnicofs nomesocket outputfile numbuckets

**nomesocket** - nome que deve ser associado ao *socket* atraves do qual o servidor recebe pedidos de ligacao

Tarefa inicial: responsavel por inicializar o *socket* e aceitar pedidos de ligacao

---
## Perguntitas

* fazer numthreads = numthreads + 1 (thread produtora) ???? 
* vamos tornar o pwd global?? inputfile


* init mutex, rw e sem
* destroy mutex, rw e sem

a1 a2 mesma arvore
nao esquecer destroy e init sems
hashtab e' global, no apply commands nao pode estar fora
  
---
### Solucao wait infinito produtor-consumidor

---CHECK---
---
* Quando acabar a leitura do ficheiro (no final)
* Acrescentar um comando de exit das threads
* Uma thread ira ler este comando
  * Vai voltar a escrever o comando (atencao a exclusao mutua)
  * exit_thread
* 