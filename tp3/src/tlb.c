
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

static int sequenceAcces[TLB_NUM_ENTRIES];

static unsigned int tlb_hit_count = 0;
static unsigned int tlb_miss_count = 0;
static unsigned int tlb_mod_count = 0;


void accesPage(int page_number){

  int position_page;
  for (position_page = 0; position_page < TLB_NUM_ENTRIES; ++position_page)
  {
    if (sequenceAcces[position_page] == page_number){
          break;
    }    
  }
  //decale les entrées jusqu'a l'ancienne position du numero de la page
  for (int i = 0; i < position_page; ++i)
  {
    sequenceAcces[i+1] = sequenceAcces[i];
  }
  // la premiere page est la plus recemment acceder 
  sequenceAcces[0] = page_number;

}

void accesPageRemplace(int page_number_new,int page_number_old){
  accesPage(page_number_old);
  sequenceAcces[0] = page_number_new;
}

void algo_LRU(int page_number_new){
  accesPageRemplace(page_number_new,sequenceAcces[TLB_NUM_ENTRIES - 1 ]);
}

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
  for (int i = 0; i< TLB_NUM_ENTRIES; i++){
  	if (tlb_entries[i].page_number == page_number){
  		//TODO stuff
      accesPage(page_number);
  	}
  }
  return -1;
}

/* Ajoute dans le TLB une entrée qui associe `frame_number` à
 * `page_number`.  */
static void tlb__add_entry (unsigned int page_number,
                            unsigned int frame_number, bool readonly)
{
    int pageVictime = sequenceAcces[TLB_NUM_ENTRIES - 1];
	  int victim = 0;
	for ( victim = 0; victim < TLB_NUM_ENTRIES; victim++){
    if (tlb_entries[victim].page_number == pageVictime){
      break;
    }
		//TODO: stuff
	}
    tlb_entries[victim].page_number = page_number;
    tlb_entries[victim].frame_number = frame_number;
    tlb_entries[victim].readonly = readonly;
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
