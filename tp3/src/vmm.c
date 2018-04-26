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
  
  //256 bits / page --> 2^8 offset, 2^8 page number
  //Shift pour ne prendre que les chiffres de gauche
  int pageNumber = laddress >> 8;
  
  //TODO: Créer le tlb
  int frameNumber = tlb_lookup (tlb, pageNumber);
  
  if (frameNumber < 0){
	//TLB Miss
	frameNumber = pt_lookup(pageNumber);
	if (frameNumber < 0){
		//Page Fault
		frameNumber = 10; //Changer ceci, comment trouve une frame dispo?
		pm_download_page(pageNumber, frameNumber);
		pt_set_entry(pageNumber, frameNumber);
	}
  }else{
	 
  }
  //Concaténation à 0000000011111111
  int offset = laddress && 255;
  
  /* ¡ TODO: COMPLÉTER ! */
  //dans la table des pages et si la page a un frame associer on swap la page 
  //avec une autre page dans le tlb. S'il n'y a pas de page 
  
  int addressPhysique = (frameNumber * PAGE_FRAME_SIZE) + offset;
  c = pm_read (addressPhysique);
  vmm_log_command (stdout, "READING", laddress, pageNumber, frameNumber, offset, addressPhysique, c);
  return c;
}

/* Effectue une écriture à l'adresse logique `laddress`.  */
void vmm_write (unsigned int laddress, char c)
{
  write_count++;
  int pageNumber = laddress >> 8;
  
  //TODO: Créer le tlb
  int frameNumber = tlb_lookup (tlb, pageNumber);
  if (frameNumber < 0){
	//TLB Miss
	frameNumber = pt_lookup(pageNumber);
	if (frameNumber < 0){
		//Page Fault
		frameNumber = 10; //Changer ceci, comment trouve une frame dispo?
		pm_download_page(pageNumber, frameNumber);
		pt_set_entry(pageNumber, frameNumber);
	}
  }else{
	 
  }

  int offset = laddress && 255;
  int addressPhysique = (frameNumber * PAGE_FRAME_SIZE) + offset;
  pm_write(paddress, c);
  //pas sure de ça puisque soit on ecrit sur le frame puis on quand on fait un 
  //swap de frame si on a ecrit sur le frame on va recopier sur le disque sinon pas besoin??
  //ou soit on ecrit directement sur le disque (less likely)

  vmm_log_command (stdout, "WRITING", laddress, pageNumber, frameNumber, offset, addressPhysique, c);
}


// NE PAS MODIFIER CETTE FONCTION
void vmm_clean (void)
{
  fprintf (stdout, "VM reads : %4u\n", read_count);
  fprintf (stdout, "VM writes: %4u\n", write_count);
}
