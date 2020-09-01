#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "batracios.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>

#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
// La union ya est� definida en sys/sem.h
#else
// Tenemos que definir la union
union semun 
{ 
	int val;
	struct semid_ds *buf;
	unsigned short int *array;
	struct seminfo *__buf;
};
#endif
int Id_Semaforo;
pid_t PIDPADRE;
int *contadorS, *contadorN, *contadorP;
char *Memoria = NULL;
int Id_Memoria;
void mover_troncos(void);
int  mover_rana(int *dx, int *dy, int *contador);
void SIGINT_padre(int s);

int main(int argc, char *argv[])
{
    struct sigaction ss;
	ss.sa_flags = 0;
    sigemptyset(&ss.sa_mask);
    ss.sa_handler = SIGINT_padre;
	sigaction(SIGINT, &ss, NULL);

	struct sigaction ss2;
	ss2.sa_flags = 0;
    sigemptyset(&ss2.sa_mask);
    ss2.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &ss2, NULL);

	char errores[100];
	int n_semaforos = 2;
    int ret = 0;
    int troncos[7] = {3,3,3,3,3,3,3};
    int laguas[7]  = {1,1,1,1,1,1,1};
    int dirs[7]    = {0,1,1,0,1,0,0};
    int tCriar     = 0;
	
    
	struct sembuf Operacion[1];
    
	//union semun arg;

	/* COMPROBACION DE PARAMETROS */
	if (argc != 3) {
		sprintf(errores, "USO: ./%s tics tcria", argv[0]);
		perror(errores);
		return 1;
	}
	ret = atoi(argv[1]);
	tCriar = atoi(argv[2]);

	if (ret < 0 || ret > 1000 || tCriar <= 0){
		sprintf(errores, "USO: PRIMER ARGUMENTO (0-1000) SEGUNDO ARGUMENTO > 0");
		perror(errores);
		return 1;
	}
	PIDPADRE = getpid();
	
    Id_Memoria = shmget (IPC_PRIVATE, 2048 + 8*3, 0777 | IPC_CREAT);
	if (Id_Memoria == -1)
	{
		exit (0);
	}

	Memoria = (char *)shmat (Id_Memoria,0, 0);
	if (Memoria == NULL)
	{
		exit (0);
	}

	contadorN = (int *)(Memoria + (2048));
	contadorS = (int *)(Memoria + (2048+8));
	contadorP = (int *)(Memoria + (2048+16));

    Id_Semaforo = semget (IPC_PRIVATE, 8, 0600 | IPC_CREAT);
	if (Id_Semaforo == -1)
	{
		exit (0);
	}


	//arg.val = 2;
	semctl (Id_Semaforo, 1, SETVAL, 20);
	semctl (Id_Semaforo, 2, SETVAL, 1);
	//semctl (Id_Semaforo, 3, SETVAL, 1); No se porque que da problemas.
	semctl (Id_Semaforo, 4, SETVAL, 1);
	semctl (Id_Semaforo, 5, SETVAL, 1);
	semctl (Id_Semaforo, 6, SETVAL, 1);
	semctl (Id_Semaforo, 7, SETVAL, 1);


    BATR_inicio(ret, Id_Semaforo, troncos, laguas, dirs, tCriar, Memoria);

	int dx, dy;
	int i = 0;
	for (i = 0; i < 4; i++) {
		switch (fork())
        {
        case -1: /* Gestión del error */
            perror("fork");
            return -1;
        case 0: /* Proceso hijo */
			
			for(;;) {
				int contador = 0;
				int flag = 0;
                Operacion[0].sem_num = 1;
                Operacion[0].sem_op  = -1; // wait
                Operacion[0].sem_flg = 0;

                semop(Id_Semaforo, Operacion, 1);
				
				/* SEMAFORO PARTO DE RANAS. */
				Operacion[0].sem_num = 1;
				Operacion[0].sem_op = -1;
				Operacion[0].sem_flg = 0;
				semop(Id_Semaforo, Operacion, i+4);

                BATR_descansar_criar();
				BATR_parto_ranas(i, &dx, &dy);
				(*contadorN)++;
				/* FIN SEMAFORO PARTO DE RANAS. */
				switch (fork()) {
					case -1: /* GESTION ERROR*/
						perror("fork");
						return -1;
					case 0: /* Proceso hijo*/
						
						for (;;) {
							
							
							/* SEMAFORO AVANCE DE RANAS */
							Operacion[0].sem_num 	= 1;
							Operacion[0].sem_op 	= -1;
							Operacion[0].sem_flg	= 0;
                			semop(Id_Semaforo, Operacion, 2);

							/* FIN SEMAFORO AVANCE DE RANAS*/

							if (dy == 11) {
								(*contadorS)++;
								Operacion[0].sem_num 	= 1;
								Operacion[0].sem_op 	= 1; //SIGNAL
								Operacion[0].sem_flg	= 0;
                				semop(Id_Semaforo, Operacion, 2);

								Operacion[0].sem_num = 1;
                				Operacion[0].sem_op  = 1; // SIGNAL
                				Operacion[0].sem_flg = 0;

                				semop(Id_Semaforo, Operacion, 1);
								kill(getpid(), SIGTERM);
								
							}

							if (mover_rana(&dx, &dy, &contador) != 0) {
								Operacion[0].sem_num 	= 1;
								Operacion[0].sem_op 	= 1; //SIGNAL
								Operacion[0].sem_flg	= 0;
                				semop(Id_Semaforo, Operacion, 2);

								Operacion[0].sem_num = 1;
                				Operacion[0].sem_op  = 1; // SIGNAL
                				Operacion[0].sem_flg = 0;
								(*contadorP)++;
								BATR_explotar(dx, dy);
                				semop(Id_Semaforo, Operacion, 1);
								kill(getpid(), SIGTERM);
							} 
							
							/* SEMAFORO AVANCE DE RANAS */
							Operacion[0].sem_num 	= 1;
							Operacion[0].sem_op 	= 1; //SIGNAL
							Operacion[0].sem_flg	= 0;
                			semop(Id_Semaforo, Operacion, 2);
							/* FIN SEMAFORO AVANCE DE RANAS */

							if (flag == 0) {
								/* SEMAFORO PARTO DE RANAS */
								Operacion[0].sem_num 	= 1;
								Operacion[0].sem_op 	= 1; // SIGNAL
								Operacion[0].sem_flg 	= 0;
								semop(Id_Semaforo, Operacion, i+4);
								flag=1;
								/* FIN SEMAFORO PARTO DE RANAS*/
							}
							

							
									
								
							
                        }
				}

			}
        default: /* Proceso padre */
			
			continue;
		}
	}
	for(;;) {

	}
	return 0;

}

void mover_troncos() {
	
	for (;;) {
        BATR_avance_troncos(0);
		BATR_pausita();
        BATR_avance_troncos(1);
		BATR_pausita();
        BATR_avance_troncos(2);
		BATR_pausita();
        BATR_avance_troncos(3);
		BATR_pausita();
        BATR_avance_troncos(4);
		BATR_pausita();
        BATR_avance_troncos(5);
		BATR_pausita();
        BATR_avance_troncos(6);
		BATR_pausita();
    }
	
}

int mover_rana(int *dx, int *dy, int *contador) {

	int aleatorio 	= rand() % 2;
	int aleatorio2 	= 0; 
	if (aleatorio != 1) {
		aleatorio2 = 1;
	}
	if (*contador == 7 || *contador == 8) return 1;
	if (!BATR_puedo_saltar(*dx,*dy, ARRIBA)){
		BATR_avance_rana_ini(*dx, *dy);
		BATR_avance_rana(dx, dy, ARRIBA);
		BATR_pausa();
		BATR_avance_rana_fin(*dx, *dy);
		*contador = 0;
	} else if (!BATR_puedo_saltar(*dx,*dy, aleatorio)) {
		BATR_avance_rana_ini(*dx, *dy);
		BATR_avance_rana(dx, dy, aleatorio);
		BATR_pausa();
		BATR_avance_rana_fin(*dx, *dy);
		*contador += 2;						
	} else if (!BATR_puedo_saltar(*dx, *dy, aleatorio2)){
        BATR_avance_rana_ini(*dx, *dy);
		BATR_avance_rana(dx, dy, aleatorio2);
		BATR_pausa();
		BATR_avance_rana_fin(*dx, *dy);	
		*contador += 3;						
	} else {
		return 1;
	}

	return 0;
}

void SIGINT_padre(int s)
{
	int status;
    char texto[100];
	pid_t proceso = getpid();
	if (PIDPADRE == proceso) {
		if (Id_Semaforo != -1) {
			if(semctl(Id_Semaforo, 0, IPC_RMID) == -1) {
				sprintf(texto, "Fallo al eliminar semaforos --> SIGINT\n");
				perror(texto);
			} else {
				sprintf(texto, "Semaforos liberados correctamente\n");
				write(1, texto, strlen(texto));
			}
		}
		

		sprintf(texto, "\nEl contador de salvadas es: %d\nEl contador Perdidas es: %d\nNacidas=%d\n", *contadorS, *contadorP, *contadorN);
		write(1, texto, strlen(texto));	

		if (shmdt(Memoria) != 0) {
			sprintf(texto, "\nFallo al eliminar memoria compartida --> SIGINT\n");
			perror(texto);
		}
		shmctl (Id_Memoria, IPC_RMID, 0); 
		
	}
	BATR_fin();
    exit(1);
}
