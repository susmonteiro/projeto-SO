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

#### Etapa Final (não avaliada)
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

- Tarefa inicial: responsavel por inicializar o *socket* e aceitar pedidos de ligacao
- nova ligacao de um cliente: uma nova tarefa escrava fica encarregue de executar e responder aos pedidos desse cliente
- **Sessao** - periodo em que a ligacao esta ativa 
- tabela de ficheiros abertos: criada pela tarefa escrava e usada durante a sessao; **vetor com 5 entradas** - podemos escolher o conteudo de cada entrada deste vetor
- final da ligacao: tarefa escrava liberta a tabela e termina

#### Terminacao do servidor

O processo servidor termina ordeiramente quando recebe o sinal ***SIGINT*** (Ctrl+c), da seguinte forma:
- tratado pela tarefa inicial (a que esta a aceitar novos pedidos de ligacao)
- a partir do momento em que recebe este sinal, deixa de receber novos pedidos de ligacao
- o servidor so termina quando todas as sessoes ativas tiverem terminado
- o tempo deve ser impresso e o conteudo final escrito num ficheiro de saida (tal como ja esta implementado)

#### API cliente do Tecnicofs

>Interface de programacao (API): nao alterar .h e codigo de erros fornecidos

Funcoes da API:
- *int mount(char *address)*: estabelece uma ligacao
- *int unmount()*: termina uma ligacao

Durante uma sessao ativa (devolvem erro se forem chamadas sem uma sessao ativa):
- *int create(char *filename, int ownerPermissions, int othersPermissions)*
- *int delete(char *filename)*
- *int rename(char *filenameOld, char *filenameNew)*
- *int close(int fd)*
- *int read (int fd, char *buffer, int len)*
- *int write (int fd, char *buffer, int len)*

>nem todos os erros que e suposto as funcoes devolverem esta especificados, podemos ter que adicionar outros

#### Protocolo de pedido-resposta
- as mensagens de pedidos sao *strings* 
- os varios campos das mensagens sao separados por ' '
- o processo recetor deve ler do *socket stream* ate alcancar '\0'
- um processo cliente so envia um pedido apos ter recebido resposta para o pedido anterior

#### Notas
- nao e suposto tornar a tabela de *i-nodes* mais eficientes
- nao se consideram permissoes do grupo
- deixa de existir uma pool de tarefas escravas
- para o sinal ***SIGINT*** usar a funcao *pthread_sigmask*
- sistema em que corre com semantica BSD

## to-do
* tabela de descritores de ficheiro: estrutura que da acesso direto ao ficheiro atraves do iNumber e mantem a permissao de abertura do ficheiro
* interromper o accept: usar ***sigaction***; verificar o erro EINT (sair ordeiramente)
* as tarefas ignoram o sinal, so o programa principal e que interrompe o accept :)
* desativar os sinais durante o pthread_create, voltar a permitir logo a seguir

---
## Perguntitas
* tamanho maximo  do comando lido pela api?? ou e' infinito??
* preferencia sobre read/write a recv/send?
  
---
### Solucao wait infinito produtor-consumidor

---CHECK---
---
* Quando acabar a leitura do ficheiro (no final)
* Acrescentar um comando de exit das threads
* Uma thread ira ler este comando
  * Vai voltar a escrever o comando (atencao a exclusao mutua)
  * exit_thread


---
## Bibliografia


- http://retis.sssup.it/~lipari/courses/OS_CP/sockets.pdf
- https://stackoverflow.com/questions/3719462/sockets-and-threads-using-c
---
# Changes

- inicialmente tentei perceber porque e' que o fd estava a mudar a cada iteracao do while
- e apos uns prints bem localizados cheguei a conclusao que a intrucao nao estava a ser bem recebida da segunda vez
- entao percebi que ambos os problemas advinham do fdopen (ele e que fica a mandar neste fd e nao o podiamos fechar sem fechar o socket e ao mesmo tempo estavamos sempre a abrir mais um cada vez que a funcao commandCreate era chamada) **TLDR- fdopen gerava um novo fd sempre que chamado**
- mas se nao podiamos usar o fdopen, como raio iamos usar o getdilem(necessita FILE*).... Entao foi quando tentei o recv para buffer e depois strcpy.
- passado um tempo cheguei a conclusao que os erros do valgrind era precisamente porque estavamos a copiar para um buffer nao inicializado.... inicializei tudo a '\0' por tentativa. Os erros desapareceram.
- no entanto no momento da segunda chamada a func create o commando parecia nao ser enviado corretamente (ideia minha) pois davamos print e simplesmente nao aparecia nada, como se estivesse so a dar print do buffer com '\0'. Mas na execucao do strtok ele mostrava o nome do ficheiro a criar e as permissoes.... Algo nao estava certo... entao estavamos ou nao a receber alguma coisa?? tinhamos que estar nao e'? **recv bloqueante**
- Possivelmente a solucao do strcpy para copiar ate ao \0 nao fosse a melhor... Acabamos por tentar fazer o nosso proprio reader ate ao \0... (daqui vieram alguns seg faults estupidos do meu lado, a tentar fazer print do \0 para ver se chegava la ... *facepalm*)
- depois de testado apenas com dois creates (com um getchar() do lado do cliente para nao dar exit), o servidor ja nao crasha e o feedback esta certo! **Tudo continua a correr como suposto**

- O primeiro dos erros do valgrind era isto.... (demasiado tempo ate perceber... se calhar voltava a iaed....)
- malloc(sizeof(char)*strlen(string))....malloc(sizeof(char)*(strlen(string)+**1**))
