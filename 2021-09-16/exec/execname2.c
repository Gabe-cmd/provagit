/*
Esercizio 1: Linguaggio C (obbligatorio) 20 punti
Creare una directory chiamata exec. Scrivere un programma execname che se viene aggiunto un file
nela directory exec interpreti il nome del file come comando con parametri, lo esegua e cancelli il file.
es: sopo aver lanciato execname:
	execname exec
a seguito di questo comando:
	touch 'exec/echo ciao mare'
il programma stampa:
	ciao mare
(consiglio, usare inotify)
*/
//inotify monitora la directory e rileva eventi
//una read corretta restituisce -> struct inotify_event 
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define EVENT_SIZE		(sizeof( struct inotify_event))
#define EVENT_BUF_LEN	(1024 * (EVENT_SIZE + 16))

// Funzuone che conta spazi presenti in stringa
int spaces_counter(char* stringa){
	int spaces = 0;
	int i = 0;
	while (stringa[i] != '\0'){
		if (stringa[i] == ' ') spaces++;
		i++;
	}
	return spaces;
}

/* -------------------------------------- */

int main(int argc, char *argv[]){

	// define PATH_MAX 4096 -> chars in a path name including nul
	char dir[PATH_MAX];
	getcwd(dir, sizeof(dir));

  	// Viene utilizzato per salvare l'output di read
  	int numberOfEvents = 0;	

	// File descriptor di inotify_init
	int fd;

	// Watch descriptor
	int wd;

	// Buffer per leggere gli eventi che si verificano
	char buffer[EVENT_BUF_LEN];

	// Istanza di inotify + check errore
	fd = inotify_init();
	if (fd < 0)	perror("Errore inizializzazione inotify: ");

	// La dir da controllare è:
	wd = inotify_add_watch(fd, dir, IN_CREATE);

	// Read che rimane finchè non si verificano eventi
	numberOfEvents = read(fd, buffer, EVENT_BUF_LEN);

	// Controllo errori
	if(numberOfEvents < 0) perror("errore nella read: ");

	// DA QUI:
	// -> teniamo conto degli eventi avvenuti
	// -> il buffer contiene gli eventi

	int i = 0;
	while (i < numberOfEvents){
		struct inotify_event* event = (struct inotify_event*) &buffer[i];

		//Name contiene in filename del file la cui cartella è monitorata
		if (event->mask & IN_CREATE){
			if (event->mask & IN_ISDIR){
				printf("Directory added -- no further actions");
			}
			else{

				// Salviamo il nome del file
				char fileName[event->len];
				char filePath[100];
				strcpy(fileName, event->name);

				// Salviamo path del file
				strcpy(filePath, dir);
				strcat(filePath, "/");
				strcat(filePath, fileName);
				
				// Puntatore a primo carattere del nome del file
				char* p = &fileName[0];
				int spaces_in_string = spaces_counter(p) + 1;
				// Array che contiene lista di argomenti
				char* listaArgomenti[spaces_in_string];

				// Splitta stringa passata in più token puntati da pointer
				char* token = strtok(fileName, " ");
				char* command = token;
				int k = 0;

				// Per valutare tutte le parole della stringa nome
				while(token != NULL && k < spaces_in_string){
					listaArgomenti[k] = token;
					token = strtok(NULL, " ");
					k++;
				}

				// Lista passata ad execvp() deve essere NULL terminated
				listaArgomenti[k] = NULL;

				pid_t puteo;
				// Eseguo la parte del figlio solo se sono nel figlio (ossia se == 0)
				if ((puteo = fork()) == 0){
					
					//
					int fd2 = open(filePath, O_WRONLY | O_CREAT);
					dup2(fd2, 1);
					close(fd2);
					
					int controlloCorrettezza = execvp(command, listaArgomenti);
					if (controlloCorrettezza == -1) printf("Forking Failed");
				}
				wait(&puteo);
			}
		}
		
		// Mi muovo di size di un evento + minura variabile
		i += EVENT_SIZE + event->len;
	}

	// Rimuoviamo dalla watchlist
 	inotify_rm_watch(fd, wd);
  	close(fd);

	return 0;
}
