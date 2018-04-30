
#include <stdint.h>
#include <stdio.h>

#include "tlb.h"

#include "conf.h"

struct tlb_entry
{
  unsigned int page_number;
  int frame_number;             /* Invalide si négatif.  */
  bool readonly : 1;
};

static FILE *tlb_log = NULL;
//declare le tlb comme tlbentries 
static struct tlb_entry tlb_entries[TLB_NUM_ENTRIES]; 

//https://stackoverflow.com/questions/23509348/how-to-set-all-elements-of-an-array-to-zero-or-any-same-value
static unsigned int sequenceAcces[TLB_NUM_ENTRIES] = {0};

static unsigned int tlb_hit_count = 0;
static unsigned int tlb_miss_count = 0;
static unsigned int tlb_mod_count = 0;


/* Initialise le TLB, et indique où envoyer le log des accès.  */
void tlb_init (FILE *log)
{
  for (int i = 0; i < TLB_NUM_ENTRIES; i++)
    tlb_entries[i].frame_number = -1;
  tlb_log = log;
}

/******************** ¡ NE RIEN CHANGER CI-DESSUS !  ******************/

/* Recherche dans le TLB.
 * Renvoie le `frame_number`, si trouvé, ou un nombre négatif sinon.  */
static int tlb__lookup (unsigned int page_number, bool write)
{
  int frame_number = -1;
  for (int i = 0; i< TLB_NUM_ENTRIES; i++){
  //Puisque commence tous avec -1, causait problème
  	if (tlb_entries[i].page_number == page_number && tlb_entries[i].frame_number != -1){
		  frame_number = tlb_entries[i].frame_number;
		  if (write && tlb_entries[i].readonly){
			 printf("Cas géré par COPY ON WRITE");
		  }
		  sequenceAcces[i] = sequenceAcces[i] >> 1;
		  sequenceAcces[i] += 0x80000000;
	}else{
		  sequenceAcces[i] = sequenceAcces[i] >> 1;
	}
  }
  return frame_number;
 
}

/* Ajoute dans le TLB une entrée qui associe `frame_number` à
 * `page_number`.  */
static void tlb__add_entry (unsigned int page_number,
                            unsigned int frame_number, bool readonly)
{
    //int pageVictime = sequenceAcces[TLB_NUM_ENTRIES - 1];
	int pageVictime = 0;
	int i = 0;
	for ( i = 0; i < TLB_NUM_ENTRIES; i++){
    //cherche dans le tlb la page victime 
    //if (tlb_entries[i].page_number == pageVictime){
		if (sequenceAcces[i] < sequenceAcces[pageVictime]){
			pageVictime = i ;
		}
	}
    //accesPageRemplace(page_number,sequenceAcces[TLB_NUM_ENTRIES - 1]);
    tlb_entries[pageVictime].page_number = page_number;
    tlb_entries[pageVictime].frame_number = frame_number;
    tlb_entries[pageVictime].readonly = readonly;
	sequenceAcces[pageVictime] = sequenceAcces[pageVictime] >> 1;
	sequenceAcces[pageVictime] += 0x80000000;
	//printf("ADDENTRY LRU :PAGE NUMBER %d FRAME NUMBER %d \n",page_number,frame_number);
	//printBinaryValue2(sequenceAcces[pageVictime]);
	//printf("\n");
}

/******************** ¡ NE RIEN CHANGER CI-DESSOUS !  ******************/

void tlb_add_entry (unsigned int page_number,
                    unsigned int frame_number, bool readonly)
{
  tlb_mod_count++;
  tlb__add_entry (page_number, frame_number, readonly);
}

int tlb_lookup (unsigned int page_number, bool write)
{
  int fn = tlb__lookup (page_number, write);
  (*(fn < 0 ? &tlb_miss_count : &tlb_hit_count))++;
  return fn;
}

/* Imprime un sommaires des accès.  */
void tlb_clean (void)
{
  fprintf (stdout, "TLB misses   : %3u\n", tlb_miss_count);
  fprintf (stdout, "TLB hits     : %3u\n", tlb_hit_count);
  fprintf (stdout, "TLB changes  : %3u\n", tlb_mod_count);
  fprintf (stdout, "TLB miss rate: %.1f%%\n",
           100 * tlb_miss_count
           /* Ajoute 0.01 pour éviter la division par 0.  */
           / (0.01 + tlb_hit_count + tlb_miss_count));
}
