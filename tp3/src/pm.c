#include <stdio.h>
#include <string.h>


#include "conf.h"
#include "pm.h"
#include "pt.h"

static FILE *pm_backing_store;
static FILE *pm_log;
static char pm_memory[PHYSICAL_MEMORY_SIZE];
static unsigned int download_count = 0;
static unsigned int backup_count = 0;
static unsigned int read_count = 0;
static unsigned int write_count = 0;

// Initialise la mémoire physique
void pm_init (FILE *backing_store, FILE *log)
{
  pm_backing_store = backing_store;
  pm_log = log;
  memset (pm_memory, '\0', sizeof (pm_memory));
}

// Charge la page demandée du backing store
void pm_download_page (unsigned int page_number, unsigned int frame_number)
{
  download_count++;
  
  //Met le pointeur au début de la page
  if (fseek(pm_backing_store, page_number * PAGE_FRAME_SIZE, SEEK_SET)){
        printf("download - fseek a retourné une erreur");
        return;
  }
  
  char buffer[PAGE_FRAME_SIZE + 1];  //Char de fin
  memset(buffer, '0', PAGE_FRAME_SIZE + 1);
  
  //Lit les données et les met dans buffer
  if (fread(buffer, PAGE_FRAME_SIZE, 1, pm_backing_store) < 1){
        printf("fread a retourné une erreur");
        return;
  }
  
  //Écrit dans la mémoire physique le contenu de notre buffer
  //Pour pouvoir y accéder plus tard
  unsigned int emplacement = frame_number * PAGE_FRAME_SIZE;
  strncpy(&pm_memory[emplacement],buffer,PAGE_FRAME_SIZE);
}

// Sauvegarde la frame spécifiée dans la page du backing store
void pm_backup_page (unsigned int frame_number, unsigned int page_number)
{  
  if (pt_dirty_p(page_number)){//si la page est dirty on la backup
		backup_count++;

	  //Met le pointeur au début de la page
	    if (fseek(pm_backing_store, page_number * PAGE_FRAME_SIZE, SEEK_SET)){
	        printf("backup - fseek a retourné une erreur");
	        return;
	  	}
	  
	  //Besoin +1? voir valgrind
	  char buffer[PAGE_FRAME_SIZE + 1];
	  memset(buffer, '0', PAGE_FRAME_SIZE + 1);
	  
	  // Va chercher le contenu de la mémoire et le met dans buffer
	  // Pour pouvoir y accéder plus tard
	  unsigned int emplacement = frame_number * PAGE_FRAME_SIZE;
	  strncpy(buffer,&pm_memory[emplacement],PAGE_FRAME_SIZE);
	  
	  //Écrit les données de buffer dans backing store
	  if (fwrite(buffer, PAGE_FRAME_SIZE, 1, pm_backing_store) < 1){
	        printf("backup - fwrite a retourné une erreur");
	        return;
	  }
	  pt_set_dirty(page_number,false);
  }
}

char pm_read (unsigned int physical_address)
{
  //Unsigned peut être négatif ou en dehors de l'interval accepté
  if (physical_address < 0){
	  printf("lecture - Ladresse physique est négative");
  }
  else if (physical_address >= PHYSICAL_MEMORY_SIZE){
	  printf("lecture - Ladresse physique est out of bound");
  }else{
	  read_count++;
	  return pm_memory[physical_address];
  }
  return '!';
}

void pm_write (unsigned int physical_address, char c)
{
  //Unsigned peut être négatif ou en dehors de l'interval accepté
  if (physical_address < 0){
	  printf("écriture - Ladresse physique est négative");
  }  
  else if (physical_address >= PHYSICAL_MEMORY_SIZE){
	  printf("écriture - Ladresse physique est out of bound");
  }else{
      write_count++;
	  pm_memory[physical_address] = c;
  }
}


void pm_clean (void)
{
  // Enregistre l'état de la mémoire physique.
  if (pm_log)
    {
      for (unsigned int i = 0; i < PHYSICAL_MEMORY_SIZE; i++)
	{
	  if (i % 80 == 0)
	    fprintf (pm_log, "%c\n", pm_memory[i]);
	  else
	    fprintf (pm_log, "%c", pm_memory[i]);
	}
    }
  fprintf (stdout, "Page downloads: %2u\n", download_count);
  fprintf (stdout, "Page backups  : %2u\n", backup_count);
  fprintf (stdout, "PM reads : %4u\n", read_count);
  fprintf (stdout, "PM writes: %4u\n", write_count);
}
