#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "conf.h"
#include "common.h"
#include "vmm.h"
#include "tlb.h"
#include "pt.h"
#include "pm.h"

static unsigned int read_count = 0;
static unsigned int write_count = 0;
static FILE* vmm_log;

void vmm_init (FILE *log)
{
  // Initialise le fichier de journal.
  vmm_log = log;
}


// NE PAS MODIFIER CETTE FONCTION
static void vmm_log_command (FILE *out, const char *command,
                             unsigned int laddress, /* Logical address. */
		             unsigned int page,
                             unsigned int frame,
                             unsigned int offset,
                             unsigned int paddress, /* Physical address.  */
		             char c) /* Caractère lu ou écrit.  */
{
  if (out)
    fprintf (out, "%s[%c]@%05d: p=%d, o=%d, f=%d pa=%d\n", command, c, laddress,
	     page, offset, frame, paddress);
}

/* Effectue une lecture à l'adresse logique `laddress`.  */
char vmm_read (unsigned int laddress)
{
  char c = '!';
  read_count++;
  /* ¡ TODO: COMPLÉTER ! */
  //va devoir trouver le numero de page -> divise l'adresse logique par 256 et 
  //la partie entière est le numero de page et le offset est addresse modulo 256
  //va ensuite chercher dans tlb si la page n'est pas dans le tbl alors on va cherher 
  //dans la table des pages et si la page a un frame associer on swap la page 
  //avec une autre page dans le tlb. S'il n'y a pas de page 

  // TODO: Fournir les arguments manquants.
  vmm_log_command (stdout, "READING", laddress, 0, 0, 0, 0, c);
  return c;
}

/* Effectue une écriture à l'adresse logique `laddress`.  */
void vmm_write (unsigned int laddress, char c)
{
  write_count++;

  //pas sure de ça puisque soit on ecrit sur le frame puis on quand on fait un 
  //swap de frame si on a ecrit sur le frame on va recopier sur le disque sinon pas besoin??
  //ou soit on ecrit directement sur le disque (less likely)
  /* ¡ TODO: COMPLÉTER ! */

  // TODO: Fournir les arguments manquants.
  vmm_log_command (stdout, "WRITING", laddress, 0, 0, 0, 0, c);
}


// NE PAS MODIFIER CETTE FONCTION
void vmm_clean (void)
{
  fprintf (stdout, "VM reads : %4u\n", read_count);
  fprintf (stdout, "VM writes: %4u\n", write_count);
}
