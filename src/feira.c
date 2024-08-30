// Importação de bibliotecas.
#include <pthread.h> 
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


// Definição da quantidade de cada agente do programa.
#define QUANTIDADE_FAZENDEIROS 1
#define QUANTIDADE_FEIRANTES 2
#define QUANTIDADE_CLIENTES 2


// Índices correspondentes as frutas produzidas e vendidas.
#define BANANAS 0
#define LARANJAS 1
#define MAÇÃS 2
#define LIMÕES 3


// Definição de variáveis globais usadas para a comunicação entre cliente e feirante e
// feirante e fazendeiro, respectivamente.
int cliente_esperando;
int feirante_comprando_frutas;


// Definição dos semáforos utilizados por cada agente para executar sua tarefa.
sem_t sem_fazendeiro;
sem_t sem_feirante;
sem_t sem_cliente;


// Definição dos semáforos utilizados para indicar um estado de espera para os agentes.
sem_t sem_feirante_comprando_frutas;
sem_t sem_cliente_esperando_frutas[QUANTIDADE_CLIENTES];


// Definição dos locks para garantir acesso exclusivo a variáveis compartilhadas.
pthread_mutex_t mutex_clientes = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_feirantes = PTHREAD_MUTEX_INITIALIZER;


// Definição de uma estrutura de par de inteiros
struct Par {
    int primeiro;
    int segundo;
};


// Definição da estrutura de cada fazendeiro.
struct Fazendeiro {
    pthread_t thread_fazendeiro;
    int frutas[4];
};


// Definição da estrutura de cada feirante.
struct Feirante {
    pthread_t thread_feirante;
    int frutas[4];
};


// Definição da estrutura de cada cliente.
struct Cliente {
    pthread_t thread_cliente;
    int frutas[4];
};


// Vetor de pares com os pedidos feitos pelos clientes aos feirantes, 
// o primeiro elemento indica qual a fruta desejada, conforme o índice
// especificado acima, e o segundo indica a quantidade desejada.
struct Par pedidos[QUANTIDADE_CLIENTES];


// Vetores de estruturas dos agentes.
struct Fazendeiro fazendeiros[QUANTIDADE_FAZENDEIROS];
struct Feirante feirantes[QUANTIDADE_FEIRANTES];
struct Cliente clientes[QUANTIDADE_CLIENTES];


// Assinatura das funções utilizadas.
void * fazendeiro(void * id);
void * feirante(void * id);
void * cliente(void * id);


// Função principal.
int main() {
    int i;
    int *id;

    srand48(time(NULL));

    // Inicializando os semáforos.
    sem_init(&sem_fazendeiro, 0, 0);
    sem_init(&sem_feirante, 0, 0);
    sem_init(&sem_cliente, 0, QUANTIDADE_CLIENTES);

    // Criando as threads dos clientes.
    for (i = 0; i < QUANTIDADE_CLIENTES; i++) {
        sem_init(&sem_cliente_esperando_frutas[i], 0, 0);

        id = (int *) malloc(sizeof(int));
        *id = i;

        if (pthread_create(&clientes[i].thread_cliente, NULL, cliente, (void *) id)) {
            printf("Erro na criação da thread.");
            exit(1);
        }
    }

    // Criando as threads dos feirantes.
    for (i = 0; i < QUANTIDADE_FEIRANTES; i++) {
        id = (int *) malloc(sizeof(int));
        *id = i;

        if (pthread_create(&feirantes[i].thread_feirante, NULL, feirante, (void *) id)) {
            printf("Erro na criação da thread.");
            exit(1);
        }
    }

    // Criando as threads dos fazendeiros.
    for (i = 0; i < QUANTIDADE_FAZENDEIROS; i++) {
        id = (int *) malloc(sizeof(int));
        *id = i;

        if (pthread_create(&fazendeiros[i].thread_fazendeiro, NULL, fazendeiro, (void *) id)) {
            printf("Erro na criação da thread.");
            exit(1);
        }
    }

    pthread_join(clientes[0].thread_cliente, NULL);

    return 0;
}


void * fazendeiro(void* pi) {
    int id = *(int *) pi;
    while (1) {
        // Fazendeiro planta as sementes.
        printf("Fazendeiro %d: Plantando sementes...\n", id);
        fazendeiros[id].frutas[BANANAS] = (rand()%4)+7;
        fazendeiros[id].frutas[LARANJAS] = (rand()%3)+5;
        fazendeiros[id].frutas[MAÇÃS] = (rand()%3)+3;
        fazendeiros[id].frutas[LIMÕES] = (rand()%3)+3;
        sleep(2);
        printf("Fazendeiro %d: Colheu: %d bananas, %d laranjas, %d maçãs e %d limões.\n",
                id, fazendeiros[id].frutas[BANANAS], fazendeiros[id].frutas[LARANJAS],
                fazendeiros[id].frutas[MAÇÃS], fazendeiros[id].frutas[LIMÕES]);

        // Fazendeiro vende o que produziu para o feirante que o chama.
        sleep(3);
        sem_wait(&sem_fazendeiro);
            printf("Fazendeiro %d: Vendendo as frutas para o feirante %d\n", id, feirante_comprando_frutas);
            feirantes[feirante_comprando_frutas].frutas[BANANAS] += fazendeiros[id].frutas[BANANAS];
            feirantes[feirante_comprando_frutas].frutas[LARANJAS] += fazendeiros[id].frutas[LARANJAS];
            feirantes[feirante_comprando_frutas].frutas[MAÇÃS] += fazendeiros[id].frutas[MAÇÃS];
            feirantes[feirante_comprando_frutas].frutas[LIMÕES] += fazendeiros[id].frutas[LIMÕES];
            sleep(2);
            printf("Fazendeiro %d: Terminou de vender as frutas para o feirante %d.\n", id, feirante_comprando_frutas);
        sem_post(&sem_feirante_comprando_frutas);
    }

    pthread_exit(0);
}


void * feirante(void* pi) {
    int id = *(int *) pi;
    printf("Feirante %d: Acabei de abrir minha barraca!\n", id);
    while (1) {
        sleep(3);
        pthread_mutex_lock(&mutex_feirantes);
            sem_wait(&sem_feirante);
                // Caso o feirante não tenha a quantidade suficiente de frutas para atender o cliente, 
                // ele vai até o fazendeiro.
                if (pedidos[cliente_esperando].segundo > feirantes[id].frutas[pedidos[cliente_esperando].primeiro]) {
                    printf("Feirante %d: Não possui frutas o suficiente para o cliente %d.\n", id, cliente_esperando);
                    while (pedidos[cliente_esperando].segundo > feirantes[id].frutas[pedidos[cliente_esperando].primeiro]) {
                        feirante_comprando_frutas = id;
                        sem_post(&sem_fazendeiro);
                        sem_wait(&sem_feirante_comprando_frutas);
                    } 
                }

                // A partir daqui, é garantido que o feirante tem a quantidade suficiente de frutas para o cliente.
                printf("Feirante %d: Vendendo %d frutas para o cliente %d.\n", id, pedidos[cliente_esperando].segundo, cliente_esperando);
                feirantes[id].frutas[pedidos[cliente_esperando].primeiro] -= clientes[cliente_esperando].frutas[pedidos[cliente_esperando].primeiro];
                sleep(2);
                printf("Feirante %d: Quantidade de frutas restantes: %d.\n", id, feirantes[id].frutas[pedidos[cliente_esperando].primeiro]);
            sem_post(&sem_cliente_esperando_frutas[cliente_esperando]);
        pthread_mutex_unlock(&mutex_feirantes);
        
    }

    pthread_exit(0);
}


void * cliente(void* pi) {
    int id = *(int *) pi;
    printf("Cliente %d: Acabei de chegar na feira!\n", id);
    while (1) {
        sleep(3);
        sem_wait(&sem_cliente);
            pthread_mutex_lock(&mutex_clientes);
                // Cliente escolhe uma fruta para comprar.
                int fruta = rand()%4;
                struct Par pedido;
                pedido.primeiro = fruta;
                pedido.segundo = clientes[id].frutas[fruta];
                pedidos[id] = pedido;
                cliente_esperando = id;
                clientes[id].frutas[fruta] = (rand()%10)+1;
                
                if (fruta == BANANAS)
                    printf("Cliente %d: Quero comprar %d %s.\n", id, clientes[id].frutas[BANANAS], "bananas");
                else if (fruta == LARANJAS)
                    printf("Cliente %d: Quero comprar %d %s.\n", id, clientes[id].frutas[LARANJAS], "laranjas");
                else if (fruta == MAÇÃS)
                    printf("Cliente %d: Quero comprar %d %s.\n", id, clientes[id].frutas[MAÇÃS], "maçãs");
                else
                    printf("Cliente %d: Quero comprar %d %s.\n", id, clientes[id].frutas[LIMÕES], "limões");

                // Cliente chama o feirante para comprar as frutas.
                sem_post(&sem_feirante);
                //  Cliente espera o feirante pegar as frutas.
                printf("Cliente %d: Esperando o feirante pegar as frutas...\n", id);
                sem_wait(&sem_cliente_esperando_frutas[id]);
            pthread_mutex_unlock(&mutex_clientes);
        printf("Cliente %d: Pegou as frutas.\n", id);
        sem_post(&sem_cliente);
    }

    pthread_exit(0);
}