#include <pthread.h> 
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define QUANTIDADE_FAZENDEIROS 1
#define QUANTIDADE_FEIRANTES 1
#define QUANTIDADE_CLIENTES 2
#define BANANAS 0
#define LARANJAS 1
#define MAÇÃS 2
#define LIMÕES 3

int cliente_sendo_atendido;

sem_t sem_fazendeiro;
sem_t sem_feirante;
sem_t sem_cliente;


struct Par {
    int primeiro;
    int segundo;
};



struct Fazendeiro {
    pthread_t thread_fazendeiro;
    int frutas[4];
};


struct Feirante {
    pthread_t thread_feirante;
    int frutas[4];
};


struct Cliente {
    pthread_t thread_cliente;
    int frutas[4];
};


struct Par pedidos[QUANTIDADE_CLIENTES];
struct Fazendeiro fazendeiros[QUANTIDADE_FAZENDEIROS];
struct Feirante feirantes[QUANTIDADE_FEIRANTES];
struct Cliente clientes[QUANTIDADE_CLIENTES];


void * fazendeiro(void * id);
void * feirante(void * id);
void * cliente(void * id);


int main() {
    int i;
    int *id;

    srand48(time(NULL));
    sem_init(&sem_fazendeiro, 0, 0);
    sem_init(&sem_feirante, 0, 0);
    sem_init(&sem_cliente, 0, QUANTIDADE_CLIENTES);
    
    for (i = 0; i < QUANTIDADE_CLIENTES; i++) {
        id = (int *) malloc(sizeof(int));
        *id = i;

        if (pthread_create(&clientes[i].thread_cliente, NULL, cliente, (void *) id)) {
            printf("Erro na criação da thread.");
            exit(1);
        }
    }

        for (i = 0; i < QUANTIDADE_FEIRANTES; i++) {
        id = (int *) malloc(sizeof(int));
        *id = i;

        if (pthread_create(&feirantes[i].thread_feirante, NULL, feirante, (void *) id)) {
            printf("Erro na criação da thread.");
            exit(1);
        }
    }


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
        printf("Fazendeiro %d: Plantando sementes...\n", id);
        fazendeiros[id].frutas[BANANAS] = (rand()%4)+7;
        fazendeiros[id].frutas[LARANJAS] = (rand()%3)+5;
        fazendeiros[id].frutas[MAÇÃS] = (rand()%3)+3;
        fazendeiros[id].frutas[LIMÕES] = (rand()%3)+3;
        sleep(2);
        printf("Fazendeiro %d: Colheu: %d bananas, %d laranjas, %d maçãs e %d limões.\n",
                id, fazendeiros[id].frutas[BANANAS], fazendeiros[id].frutas[LARANJAS],
                fazendeiros[id].frutas[MAÇÃS], fazendeiros[id].frutas[LIMÕES]);
        sleep(3);
        sem_wait(&sem_fazendeiro);
            printf("Fazendeiro %d: Vendendo as frutas para os feirantes...\n", id);
            for (int i = 0; i < QUANTIDADE_FEIRANTES; i++) {
                feirantes[i].frutas[BANANAS] += fazendeiros[id].frutas[BANANAS];
                feirantes[i].frutas[LARANJAS] += fazendeiros[id].frutas[LARANJAS];
                feirantes[i].frutas[MAÇÃS] += fazendeiros[id].frutas[MAÇÃS];
                feirantes[i].frutas[LIMÕES] += fazendeiros[id].frutas[LIMÕES];
            }  
            sleep(2);
            printf("Fazendeiro %d: Terminou de vender as frutas para os feirantes.\n", id);
        sem_post(&sem_feirante);
    }

    pthread_exit(0);
}


void * feirante(void* pi) {
    int id = *(int *) pi;
    printf("Feirante %d: Acabei de abrir minha barraca!\n", id);
    while (1) {
        sleep(3);
        sem_wait(&sem_feirante);
            if (pedidos[cliente_sendo_atendido].segundo > feirantes[id].frutas[pedidos[cliente_sendo_atendido].primeiro]) {
                printf("Feirante %d: Não possui frutas o suficiente para o cliente %d.\n", id, cliente_sendo_atendido);
                while (pedidos[cliente_sendo_atendido].segundo > feirantes[id].frutas[pedidos[cliente_sendo_atendido].primeiro]) {
                    sem_post(&sem_fazendeiro);
                } 
            }
            printf("Feirante %d: Vendendo %d frutas para o cliente %d.\n", id, pedidos[cliente_sendo_atendido].segundo, cliente_sendo_atendido);
            feirantes[id].frutas[pedidos[cliente_sendo_atendido].primeiro] -= clientes[cliente_sendo_atendido].frutas[pedidos[cliente_sendo_atendido].primeiro];
            sleep(2);
            printf("Feirante %d: Quantidade de frutas restantes: %d.\n", id, feirantes[id].frutas[pedidos[cliente_sendo_atendido].primeiro]);
        sem_post(&sem_cliente);
    }

    pthread_exit(0);
}


void * cliente(void* pi) {
    int id = *(int *) pi;
    printf("Cliente %d: Acabei de chegar na feira!\n", id);
    while (1) {
        sleep(3);
        sem_wait(&sem_cliente);
        // gera um numero aleatorio de frutas
        clientes[id].frutas[BANANAS] = (rand()%10)+1;
        clientes[id].frutas[LARANJAS] = (rand()%10)+1;
        clientes[id].frutas[MAÇÃS] = (rand()%10)+1;
        clientes[id].frutas[LIMÕES] = (rand()%10)+1;
        int fruta = rand()%4;
        struct Par pedido;
        pedido.primeiro = fruta;
        pedido.segundo = clientes[id].frutas[fruta];
        pedidos[id] = pedido;
        cliente_sendo_atendido = id;
        
        if (fruta == BANANAS)
            printf("Cliente %d: Quero comprar %d %s.\n", id, clientes[id].frutas[BANANAS], "bananas");
        else if (fruta == LARANJAS)
            printf("Cliente %d: Quero comprar %d %s.\n", id, clientes[id].frutas[LARANJAS], "laranjas");
        else if (fruta == MAÇÃS)
            printf("Cliente %d: Quero comprar %d %s.\n", id, clientes[id].frutas[MAÇÃS], "maçãs");
        else
            printf("Cliente %d: Quero comprar %d %s.\n", id, clientes[id].frutas[LIMÕES], "limões");

        // chama o feirante
        sem_post(&sem_feirante);
        // espera
        printf("Cliente %d: Esperando o feirante pegar as frutas...\n", id);
    }

    pthread_exit(0);
}
