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
   int numFrame;
   bool vide;
   unsigned int indice;

};

static unsigned int read_count = 0;
static unsigned int write_count = 0;
static FILE* vmm_log;
static struct acces sequenceAcces[NUM_FRAMES] = {{.indice=0}, {.vide = true},{.numPage=0},{.numFrame=0}};
 static bool initialise = false;
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

void miseAjourSequenceAcces(int frameNumber,unsigned int pageNumber,bool vide){
	sequenceAcces[frameNumber].numPage = pageNumber;
	sequenceAcces[frameNumber].vide = vide;
}
struct acces getFrameLRU(){
	int frameVictime = 0;
	for (int i = 0; i< NUM_FRAMES; i++){
		if (sequenceAcces[i].indice<sequenceAcces[frameVictime].indice){
			frameVictime = i;
		}
	}
	return sequenceAcces[frameVictime];
}

void applyLRU(int frameNumber){
	if (!initialise){
		for (int i = 0; i < NUM_FRAMES; ++i)
		{
			sequenceAcces[i].numFrame  = i;
		}
		initialise = true;
	}
	for (int i = 0; i< NUM_FRAMES; i++){
		if (i == frameNumber){
			  sequenceAcces[i].indice =  sequenceAcces[i].indice >> 1;
			  sequenceAcces[i].indice += 0x80000000;
		}else{
			  sequenceAcces[i].indice =  sequenceAcces[i].indice >> 1;
		}
	}
}

int getFrame(unsigned int pageNumber, bool readonly){
	struct acces frame = getFrameLRU();
	applyLRU(frame.numFrame);
	int frameNumber = frame.numFrame;
	if (frame.vide != true){
		pm_backup_page(frameNumber,frame.numPage);
		pt_unset_entry(frame.numPage);
	}
	pm_download_page(pageNumber,frameNumber);
	pt_set_entry(pageNumber,frameNumber, readonly);
	miseAjourSequenceAcces(frameNumber,pageNumber,false);
	return frameNumber;
}

int trouverFrame(unsigned int pageNumber){
  int frameNumber = tlb_lookup (pageNumber,false);
  if (frameNumber < 0){
	//TLB Miss
  	    frameNumber = pt_lookup(pageNumber);
		if (frameNumber < 0){
			//Page Fault
			frameNumber = getFrame(pageNumber,false);
			
		}
	tlb_add_entry(pageNumber,frameNumber,pt_readonly_p(pageNumber));
  }
  applyLRU(frameNumber);
  
  return frameNumber;
}

int trouverFrameWrite(unsigned int pageNumber){
	int frameNumber;
	//Quand on veut écrire sur un readonly, on doit appliquer le COW
	if (pt_readonly_p(pageNumber))
	{
		frameNumber = getFrame(pageNumber, true);
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
  vmm_log_command (stdout, "WRITING", laddress, pageNumber, frameNumber, offset, addressPhysique, c);
}


// NE PAS MODIFIER CETTE FONCTION
void vmm_clean (void)
{
  fprintf (stdout, "VM reads : %4u\n", read_count);
  fprintf (stdout, "VM writes: %4u\n", write_count);
}
