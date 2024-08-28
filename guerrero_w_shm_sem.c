#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Códigos de color ANSI
#define GREEN	    "\x1B[32m"
#define YELLOW	    "\x1B[33m"
#define RED	        "\x1B[31m"
#define BLUE	    "\x1B[34m"
#define GRNB        "\e[42m"
#define YELB        "\e[43m"
#define YELHB       "\e[0;103m"
#define REDB        "\e[41m"
#define RESET 	    "\x1B[0m"
#define TEXT_BOLD   "\e[1m"

// Segmentos compartidos (DEBEN CAMBIARSE AL MOMENTO DE LA ENTREGA)
#define SEM_USER        "/soyd_semaforo_user_alu_120056"
#define SEM_SERVER      "/soyd_semaforo_server_alu_120056"
#define SHARED_MEMORY   "/soyd_memoria_compartida_alu_120056"

// Ataques
#define SWORD 1
#define MACE  2
#define ARROW 3

// Nombre de cada ataque
#define SWORD_TXT   "espada"
#define MACE_TXT    "maza"
#define ARROW_TXT   "flecha"
#define FAIL_TXT    "nulo"

// Mensaje de cada ataque
#define SWORD_MSJ                   "Ataque con espada EXITOSO al monstruo. Le hice %i de daño y su salud se redujo a %i."
#define MACE_MSJ_S                  "Ataque con maza EXITOSO al monstruo. Le hice %i de daño y su salud se redujo a %i."
#define MACE_MSJ_F1                 "Ataque con maza FALLIDO! Energía insuficiente. Se desactiva el ataque temporalmente."
#define MACE_MSJ_F2                 "Ataque con maza FALLIDO! Poder desactivado. Debo esperar %i turno/s."
#define MACE_MSJ_ENERGY_RECHARGER   "Ataque con maza FALLIDO! Se activa el ataque y se recargan %i unidades de energía."
#define ARROW_MSJ_S                 "Ataque con flecha EXITOSO al monstruo. Le hice %i de daño y su salud se redujo a %i."
#define ARROW_MSJ_F                 "Ataque con flecha FALLIDO al monstruo. No saque el dado correcto."

// Dados
#define D5    5
#define D6    6
#define D20   20

// Dado exitoso ataque con flecha
#define DICE_ARROW_SUCCES   3
#define ARROW_SUCCES        10

// Configuraciones de jugador
#define PLAYER          7
#define PLAYER_ENERGY   25
#define PLAYER_HEALTH   100
#define PLAYER_INIT_NAME         "alu_120056"
#define PLAYER_NAME             "Franco L. "
#define PLAYER_NAME_GAME_OVER   "game over "
#define PLAYER_MSJ_GAME_OVER    "El monstruo acabó conmigo. Fue lindo mientras duró :(."

// Monster
#define MONSTER 0

// Bitacoras
#define BITACORA_LOCAL     "guerrero-alu_120056.csv"
#define BITACORA_SERVER    "/tmp/guerra-soyd.csv"

// Tamaño de buffer general para cadenas de caracteres
#define SIZE_BUFFER 512

// Control para el ataque con maza
#define MACE_LIMIT 3
int mace_attack_control = 4;

// struct compartida
struct jugador {
    char nombre[15];
    int salud;
    int energia;
    int ataco;
    char mensaje[100];
};

// struct para manejar solo los datos necesarios
struct dataGame {
    struct jugador * player;
    struct jugador * monster;
    sem_t * sem_user;
    sem_t * sem_server;
    int file_descriptor;
};

void game(struct dataGame * data_game);
void init_data_game(struct dataGame * data_game);
int show_menu();
void check_reset_game(struct jugador * player);
void show_stats(struct jugador * monster, struct jugador * player);
char * execute_attack(int attack, struct jugador * monster, struct jugador * player);
char * sword_attack(struct jugador * monster, struct jugador * player);
char * mace_attack(struct jugador * monster, struct jugador * player);
char * arrow_attack(struct jugador * monster, struct jugador * player);
int random_dice(int face);
void update_bitacoras(char * msj);
void game_over(struct dataGame * data_game);
void free_data_game(struct dataGame * data_game);

int main(void) {
    // Carga de semáforos y memroria compartida
    struct dataGame * data_game;
    data_game = malloc(sizeof(struct dataGame));
    init_data_game(data_game);

    // Limpio la pantalla
    system("clear");

    // Inicio el juego
    if(data_game != NULL)
        game(data_game);
    else {
        printf(RED TEXT_BOLD"\n   ERROR: datos de juego no inicializados.\n\n" RESET);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/**
 * @brief Inicializa el juego.
 * @param data_game estructura que almacena los datos del juego.
 */
void game(struct dataGame * data_game) {
    int attack;
    int control;
    char * msj;

    // Variable para controlar el bucle, en caso de que el monstruo mate al jugador este no va a poder realizar ataques
    control = 1;

    printf(BLUE TEXT_BOLD "\n   ¡¡INICIO JUEGO DE ROL SOyD!!\n\n" RESET);
    show_stats(data_game->monster, data_game->player);
    if(data_game->player->salud > 0) {
        attack = show_menu();

        while (attack != 4 && control == 1) {
            printf(RED TEXT_BOLD"ESPERANDO TURNO PARA EJECUTAR ATAQUE: %i\n" RESET, attack);

            sem_wait(data_game->sem_user);
            system("clear");
            // Inicio sección CRÍTICA
            if(data_game->player->salud > 0) {
                // Chequeo reset game
                check_reset_game(data_game->player);
                // Ejecuta el ataque seleccionado
                msj = execute_attack(attack, data_game->monster, data_game->player);
                update_bitacoras(msj);
                printf(BLUE TEXT_BOLD "\n   %s\n\n" RESET, data_game->player->mensaje);
                free(msj);
            } else {
                // En este momento player se entera que esta muerto.
                printf(RED TEXT_BOLD"\n   GAME OVER\n\n" RESET);
                game_over(data_game);
                control = 0;
            }
            show_stats(data_game->monster, data_game->player);
            // Fin sección CRÍTICA
            sem_post(data_game->sem_server);

            attack = show_menu();
        }
    }

    free_data_game(data_game);
    printf(YELLOW TEXT_BOLD"SALIENDO...\n" RESET);
}

/**
 * @brief Abre los semaforos y el segmento de memoria compartida para iniciar el juego.
 * @param data_game estructura para almacenar los jugadores, semaforos y memoria compartida.
 */
void init_data_game(struct dataGame * data_game) {
    sem_t * sem_user;
    sem_t * sem_server;
    struct jugador * jugadores;
    int file_descriptor;
    struct stat stats_memory_obj;

    // Apertura de semáforos.
    sem_user = sem_open(SEM_USER, 0);
    sem_server = sem_open(SEM_SERVER, 0);

    if(sem_user == SEM_FAILED || sem_server == SEM_FAILED) {
        perror("Error al abrir los semaforos.\n");
        exit(EXIT_FAILURE);
    }

    // Apertura de la memoria compartida.
    file_descriptor = shm_open(SHARED_MEMORY, O_RDWR, 0666);
    if(file_descriptor == -1) {
        perror("Error al abrir un objeto de memoria compartida.\n");
        exit(EXIT_FAILURE);
    }

    if(fstat(file_descriptor, &stats_memory_obj) == -1) {
        perror("Error al obtener las estructura stat que contiene informacion acerca del objeto de memoria compartida.\n");
        exit(EXIT_FAILURE);
    }

    // Mapeo del objeto de memoria compartida al puntero "jugadores" de la memoria local.
    jugadores = mmap(0, stats_memory_obj.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, 0);
    if(jugadores == MAP_FAILED) {
        perror("Error al mapear el espacio de memoria a la memoria local.\n");
        exit(EXIT_FAILURE);
    }

    data_game->player = &jugadores[PLAYER];
    data_game->monster = &jugadores[MONSTER];
    data_game->sem_user = sem_user;
    data_game->sem_server = sem_server;
    data_game->file_descriptor = file_descriptor;

    // Cambio de nombre solo cuando este puede jugar
    if(data_game->player->salud > 0)
        sprintf(data_game->player->nombre, PLAYER_NAME);
}

/**
    * @brief Menu de seleccion de ataque: \n
    * 1- Ataque con espada. \n
    * 2- Ataque con maza. \n
    * 3- Ataque con flecha. \n
    * 4- Salir. \n
    * En el caso de ingresar una cadena de caracteres, solamente trata al primero.
    * @return int opcion seleccionada.
 */
int show_menu() {
    char control[SIZE_BUFFER];
    printf(YELLOW TEXT_BOLD "============ Menu de selección de ataque ============\n" RESET);
    printf(GREEN TEXT_BOLD "1- " RESET "Ataque con espada.\n");
    printf(GREEN TEXT_BOLD "2- " RESET "Ataque con maza.\n");
    printf(GREEN TEXT_BOLD "3- " RESET "Ataque con flecha.\n");
    printf(GREEN TEXT_BOLD "4- " RESET "Salir.\n");
    printf("Introduzca opción (1-4): ");
    scanf(" %s", control);
    // Utilizo tabla ASCII para evitar bucles infintos ingresando caracteres, en el caso de que control sea int.
    while((int) control[0] < 49 || (int) control[0] > 52){
        printf(RED TEXT_BOLD "¡Opción incorrecta!\n" RESET);
        printf("Introduzca opción (1-4): ");
        scanf(" %s", control);
    }
    return control[0] - '0';
}

/**
 * @brief Chequea que el mosntruo no haya reiniciado el juego.
 * @param player datos del jugador.
 */
void check_reset_game(struct jugador * player) {
    // Si el nombre del jugador es igual al inicial, lo cambio por su nombre real.
    if(strcmp(player->nombre, PLAYER_INIT_NAME) == 0)
        sprintf(player->nombre, PLAYER_NAME);
}

/**
    * @brief Ejecuta un ataque al mounstro,
    * @param attack ataque (1-3)
    * @param monster jugador mounstro
    * @param player jugador principal
    * @return char * mensaje que retorna un ataque.
 */
char * execute_attack(int attack, struct jugador * monster, struct jugador * player) {
    char * toReturn;
    switch (attack)
    {
        case 1 : {
            toReturn = sword_attack(monster, player);
            break;
        }
        case 2 : {
            toReturn = mace_attack(monster, player);
            break;
        }
        case 3 : {
            toReturn = arrow_attack(monster, player);
            break;
        }
        default: {
            return NULL;
        }
    }
    return toReturn;
}

/**
    * @brief Ataque con espada.
    * @param monster jugador mounstro
    * @param player jugador principal
    * @return char * mensaje que retorna el ataque.
 */
char * sword_attack(struct jugador * monster, struct jugador * player) {
    int dice;
    char * msj;
    msj = (char *) malloc(sizeof(char) * SIZE_BUFFER);
    dice = random_dice(D6);
    monster->salud -= dice;
    player->ataco = SWORD;

    mace_attack_control++;
    if(mace_attack_control == MACE_LIMIT) {
        player->energia += PLAYER_ENERGY;
    }

    // Chequeo salud negativa del monstruo
    monster->salud = (monster->salud < 0) ? 0 : monster->salud;

    sprintf(player->mensaje, SWORD_MSJ, dice, monster->salud);
    sprintf(msj, "%s, %i, %i, " SWORD_TXT ", D6=%i, %s", player->nombre, player->salud, player->energia, dice, player->mensaje);
    return msj;
}

/**
    * @brief Ataque con maza.
    * @param monster jugador mounstro
    * @param player jugador principal
    * @return char * mensaje que retorna el ataque.
 */
char * mace_attack(struct jugador * monster, struct jugador * player){
    int dice;
    char * msj;
    msj = (char *) malloc(sizeof(char) * SIZE_BUFFER);
    dice = random_dice(D20);
    player->ataco = MACE;

    if(mace_attack_control >= MACE_LIMIT) {
        if((player->energia - dice) > 0) {
            monster->salud -= dice;
            player->energia -= dice;

            // Chequeo salud negativa dele monstruo
            monster->salud = (monster->salud < 0) ? 0 : monster->salud;

            sprintf(player->mensaje, MACE_MSJ_S, dice, monster->salud);
            sprintf(msj, "%s, %i, %i, " MACE_TXT ", D20=%i, %s", player->nombre, player->salud, player->energia, dice, player->mensaje);
        } else {
            player->energia = 0;
            mace_attack_control = 0;
            sprintf(player->mensaje, MACE_MSJ_F1);
            sprintf(msj, "%s, %i, %i, " FAIL_TXT ", D20=%i, %s", player->nombre, player->salud, player->energia, dice, player->mensaje);
        }
    } else {
        mace_attack_control++;
        if(mace_attack_control == MACE_LIMIT) {
            player->energia += PLAYER_ENERGY;
            sprintf(player->mensaje, MACE_MSJ_ENERGY_RECHARGER, PLAYER_ENERGY);
        } else {
            sprintf(player->mensaje, MACE_MSJ_F2, MACE_LIMIT - mace_attack_control);
        }
        sprintf(msj, "%s, %i, %i, " FAIL_TXT ", D20=%i, %s", player->nombre, player->salud, player->energia, dice, player->mensaje);
    }

    return msj;
}

/**
    * @brief Ataque con flecha.
    * @param monster jugador mounstro
    * @param player jugador principal
    * @return char * mensaje que retorna el ataque.
 */
char * arrow_attack(struct jugador * monster, struct jugador * player) {
    int dice;
    char * msj;
    msj = (char *) malloc(sizeof(char) * SIZE_BUFFER);
    dice = random_dice(D5);
    player->ataco = ARROW;

    if(dice == DICE_ARROW_SUCCES) {
        monster->salud -= ARROW_SUCCES;
        player->energia += ARROW_SUCCES;

        // Chequeo salud negativa del monstruo.
        monster->salud = (monster->salud < 0) ? 0 : monster->salud;

        sprintf(player->mensaje, ARROW_MSJ_S, ARROW_SUCCES, monster->salud);
        sprintf(msj, "%s, %i, %i, " ARROW_TXT ", D5=%i, %s", player->nombre, player->salud, player->energia, dice, player->mensaje);
    } else {
        sprintf(player->mensaje, ARROW_MSJ_F);
        sprintf(msj, "%s, %i, %i, " FAIL_TXT ", D5=%i, %s", player->nombre, player->salud, player->energia, dice, player->mensaje);
    }

    mace_attack_control++;
    if(mace_attack_control == MACE_LIMIT) {
        player->energia += PLAYER_ENERGY;
    }

    return msj;
}

/**
    * @brief Simula tirar un dado de "face" caras. Utiliza la hora como semilla.
    * @param face caras del dado
    * @return face cara seleccionada
 */
int random_dice(int face) {
    srand(time(NULL));
    return rand() % face + 1;
}

/**
 * @brief Actualiza las bitacoras correspondientes al jugador.
 * Las bitacoras se actualizan en tiempo real.
 * @param data_game
 */
void update_bitacoras(char * msj) {
    char * insert_server;
    char * insert_local;

    insert_server = malloc(sizeof(char) * SIZE_BUFFER);
    insert_local = malloc(sizeof(char) * SIZE_BUFFER);

    // Copia en bitacora del mensaje del jugador
    sprintf(insert_server, "echo '%s;' >> " BITACORA_SERVER, msj);
    sprintf(insert_local, "echo '%s;' >> " BITACORA_LOCAL, msj);
    system(insert_server);
    system(insert_local);

    free(insert_server);
    free(insert_local);
}

/**
    * @brief Muestra por consola las stats de un jugador.
    * @param monster jugador monstruo
    * @param player jugador
 */
void show_stats(struct jugador * monster, struct jugador * player) {
    char * color;
    // Con esto evito que la barra de salud sea 0
    int bar_size = (player->salud > 0) ? player->salud : 1;
    char bar[bar_size];
    color = (bar_size > 0.8 * PLAYER_HEALTH) ? GRNB :
            (bar_size > 0.5 * PLAYER_HEALTH) ? YELHB :
            (bar_size > 0.25 * PLAYER_HEALTH) ? YELB :
            REDB;

    for (int i = 0; i < bar_size; i++) {
        bar[i] = ' ';
    }
    // Elimino atributos extraños finalizando la cadena
    bar[bar_size - 1] = '\0';

    // Se hardcodea porque el monstruo puede dejarme con salud negativa
    player->salud = (player->salud < 0) ? 0 : player->salud;

    printf(YELLOW TEXT_BOLD "==== Estado del jugador ====\n" RESET);
    printf(GREEN TEXT_BOLD "Nombre: " RESET "%s\n", player->nombre);
    printf(GREEN TEXT_BOLD "Salud: " RESET "(%i) %s%s\n" RESET, player->salud, color, bar);
    printf(GREEN TEXT_BOLD "Energía: " RESET "%i\n", player->energia);
    printf(GREEN TEXT_BOLD "Último mensaje del monstruo: " RESET "%s\n", monster->mensaje);
}

/**
 * @brief Fin de juego para el player recibido.
 * @param data_game informacion de juego.
 */
void game_over(struct dataGame * data_game) {
    char * msj;
    msj = (char *) malloc(sizeof(char) * SIZE_BUFFER);

    data_game->player->salud = 0;
    data_game->player->energia = 0;
    data_game->player->ataco = 0;
    sprintf(data_game->player->mensaje, PLAYER_MSJ_GAME_OVER);

    //Actualizo la bitacora para indicar que el player esta fuera de juego
    sprintf(msj, "%s, %i, %i, " FAIL_TXT ", 0, %s", data_game->player->nombre, data_game->player->salud, data_game->player->energia, data_game->player->mensaje);
    update_bitacoras(msj);

    // En memoria compartido dejo el nombre respectivo para game over (fueradejuego)
    sprintf(data_game->player->nombre, PLAYER_NAME_GAME_OVER);
    free(msj);
}

/**
 * @brief Libera los recursos utilizados.
 * @param data_game informacion de juego.
 */
void free_data_game(struct dataGame * data_game) {
    sem_close(data_game->sem_user);
    sem_close(data_game->sem_server);

    close(data_game->file_descriptor);
    free(data_game);
}