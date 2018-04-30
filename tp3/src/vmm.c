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

struct acces {
  unsigned int numPage;
  unsigned int numFrame;
};

static unsigned int read_count = 0;
static unsigned int write_count = 0;
static FILE* vmm_log;
//static struct acces sequenceAcces2[NUM_PAGES];
static unsigned int sequenceAcces[NUM_FRAMES] = {0};

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

/*
struct acces getLRU(){
  return sequenceAcces[NUM_PAGES -1];
}

void accesFrame( struct acces frame){

  int position_frame;
  struct acces frameSpecifie;
  for (position_frame = 0; position_frame < NUM_PAGES; ++position_frame)
  {
    if (sequenceAcces[position_frame].numFrame == frame.numFrame){
          frameSpecifie = sequenceAcces[position_frame];
          break;
    }    
  }
  //decale les entrées jusqu'a l'ancienne position du frame_number
  for (int i = 0; i < position_frame; ++i)
  {
    sequenceAcces[i+1] = sequenceAcces[i];
  }
  // le premier frame est le plus recemment acceder 
  sequenceAcces[0] = frameSpecifie;

}

void pageFault(unsigned int pageNumber, struct acces ancienFrame){
  
  unsigned int frameNumber = ancienFrame.numFrame;
  //sauvegarde l'ancien frame dans le backing store et enleve de la page table
  pm_backup_page(frameNumber,ancienFrame.numPage);
  pt_unset_entry(ancienFrame.numPage);
  //met contenu de la page dans le frame puis modifie entrée dans la pt
  pm_download_page(pageNumber, frameNumber);
  pt_set_entry(pageNumber, frameNumber);
  //indique que j'ai acceder au frame
  struct acces frameAcceder = {.numFrame = frameNumber, .numPage = pageNumber};
  accesFrame(frameAcceder);
}
*/
int getFrameLRU(){
	int frameVictime = 0;
	for (int i = 0; i< NUM_FRAMES; i++){
		if (sequenceAcces[i]<sequenceAcces[frameVictime]){
			frameVictime = i;
		}
	}
	return frameVictime;
}

void applyLRU(int frameNumber){
	for (int i = 0; i< NUM_FRAMES; i++){
		if (i == frameNumber){
			  sequenceAcces[i] += 0x40000000;
		}else{
			  sequenceAcces[i] =  sequenceAcces[i] >> 1;
		}
	}
}

int getFrame(int pageNumber){
	int frameNumber = getFrameLRU();
	
	pm_backup_page(frameNumber,pageNumber);
	pt_unset_entry(pageNumber);
	pm_download_page(pageNumber,frameNumber);
	pt_set_entry(pageNumber,frameNumber);
	return frameNumber;
}

int trouverFrame(int pageNumber){
    int frameNumber = tlb_lookup (pageNumber,false);
  if (frameNumber < 0){
	//TLB Miss
  	    frameNumber = pt_lookup(pageNumber);
		if (frameNumber < 0){
			//Page Fault
			frameNumber = getFrame(pageNumber);
		  //struct acces frameAcces = getLRU();
		  //frameNumber = frameAcces.numFrame;
			//frameNumber = 10; //Changer ceci, comment trouve une frame dispo?
		  //pageFault(pageNumber,frameAcces);
			
		}
	tlb_add_entry(pageNumber,frameNumber,pt_readonly_p(pageNumber));
  }
  applyLRU(frameNumber);
  
  return frameNumber;
}

int trouverFrameWrite(int pageNumber){
	int frameNumber;
	if (pt_readonly_p(pageNumber))
	{
		frameNumber = getFrame(pageNumber);
		tlb_add_entry(pageNumber,frameNumber,false);
	}

	else{
		frameNumber = trouverFrame(pageNumber);
	}

	return frameNumber;
}
/* Effectue une lecture à l'adresse logique `laddress`.  */
char vmm_read (unsigned int laddress)
{
  char c = '!';
  read_count++;
  
  //256 bits / page --> 2^8 offset, 2^8 page number
  //Shift pour ne prendre que les chiffres de gauche
  unsigned int pageNumber = laddress >> 8;

  int frameNumber = trouverFrame (pageNumber);
  
  //Concaténation à 0000000011111111
  int offset = laddress & 255;
  int addressPhysique = (frameNumber * PAGE_FRAME_SIZE) + offset;
  c = pm_read (addressPhysique);
  vmm_log_command (stdout, "READING", laddress, pageNumber, frameNumber, offset, addressPhysique, c);
  return c;
}

/* Effectue une écriture à l'adresse logique `laddress`.  */
void vmm_write (unsigned int laddress, char c)
{
  write_count++;
  unsigned int pageNumber = laddress >> 8;
  
  int frameNumber = trouverFrameWrite(pageNumber);
  pt_set_dirty(pageNumber,true);
  int offset = laddress & 255;
  int addressPhysique = (frameNumber * PAGE_FRAME_SIZE) + offset;
  pm_write(addressPhysique, c);
  
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
