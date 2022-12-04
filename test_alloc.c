#include "mem.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void afficher_zone(void *adresse, size_t taille, int free)
{
  printf("Zone %s, Adresse : %lu, Taille : %lu\n", free?"libre":"occupee",
         adresse - get_memory_adr(), (unsigned long) taille);
}

void afficher_zone_libre(void *adresse, size_t taille, int free)
{
  if (free)
	  afficher_zone(adresse, taille, 1);
}

void afficher_zone_occupee(void *adresse, size_t taille, int free)
{
  if (!free)
	  afficher_zone(adresse, taille, 0);
}

int main(int argc, char *argv[]) {
	void* tab_adresse [9];
	int pos_adresse = 0;

	printf("\n\nVérification de l'allocateur mémoire\n");
	mem_init(get_memory_adr(), get_memory_size());
	for (int i=100; i<1000; i=i+100) {
		
		tab_adresse[pos_adresse] = mem_alloc(i);
		if(tab_adresse[pos_adresse]==NULL)
			printf("Error allocation\n");
		pos_adresse++;
	}

	printf("Libre : \n\n");
	mem_show(afficher_zone_libre);
	printf("occupee : \n\n");
	mem_show(afficher_zone_occupee);
	printf("\n");

	mem_free(tab_adresse[0]);
	printf("Memoire liberee : %lu\n",tab_adresse[0]- get_memory_adr());
	mem_free(tab_adresse[8]);
	printf("Memoire liberee : %lu\n",tab_adresse[8]- get_memory_adr());
	mem_free(tab_adresse[1]);
	printf("Memoire liberee : %lu\n",tab_adresse[1]- get_memory_adr());
	printf("Libre : \n\n");
	mem_show(afficher_zone_libre);
	printf("occupee : \n\n");
	mem_show(afficher_zone_occupee);
	printf("\n");

	mem_free(tab_adresse[2]);
	printf("Memoire liberee : %lu\n",tab_adresse[2]- get_memory_adr());
	
	mem_free(tab_adresse[7]);
	printf("Memoire liberee : %lu\n",tab_adresse[7]- get_memory_adr());

		printf("Libre : \n\n");
	mem_show(afficher_zone_libre);
	printf("occupee : \n\n");
		mem_show(afficher_zone_occupee);
		printf("\n");

	mem_free(tab_adresse[4]);
	printf("Memoire liberee : %lu\n",tab_adresse[4]- get_memory_adr());
	mem_free(tab_adresse[3]);
	printf("Memoire liberee : %lu\n",tab_adresse[3]- get_memory_adr());
	mem_free(tab_adresse[5]);
	printf("Memoire liberee : %lu\n",tab_adresse[5]- get_memory_adr());
	mem_free(tab_adresse[6]);
	printf("Memoire liberee : %lu\n",tab_adresse[6]- get_memory_adr());

	printf("Libre : \n\n");
	mem_show(afficher_zone_libre);
	printf("occupee : \n\n");
	mem_show(afficher_zone_occupee);
	printf("\n");

	// TEST OK
	return 0;
}
