/* On inclut l'interface publique */
#include "mem.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* Définition de l'alignement recherché
 * Avec gcc, on peut utiliser __BIGGEST_ALIGNMENT__
 * sinon, on utilise 16 qui conviendra aux plateformes qu'on cible
 */
#ifdef __BIGGEST_ALIGNMENT__
#define ALIGNMENT __BIGGEST_ALIGNMENT__
#else
#define ALIGNMENT 16
#endif

/* structure placée au début de la zone de l'allocateur

   Elle contient toutes les variables globales nécessaires au
   fonctionnement de l'allocateur

   Elle peut bien évidemment être complétée
*/
struct allocator_header {
        size_t memory_size;
	mem_fit_function_t *fit;
};

/* La seule variable globale autorisée
 * On trouve à cette adresse le début de la zone à gérer
 * (et une structure 'struct allocator_header)
 */
static void* memory_addr;

static inline void *get_system_memory_addr() {
	return memory_addr;
}

static inline struct allocator_header *get_header() {
	struct allocator_header *h;
	h = get_system_memory_addr();
	return h;
}

static inline size_t get_system_memory_size() {
	return get_header()->memory_size;
}


struct fb {
	size_t size;
	struct fb* next;
	/* ... */
};


void mem_init(void* mem, size_t taille)
{
        memory_addr = mem;
        *(size_t*)memory_addr = taille;
	/* On vérifie qu'on a bien enregistré les infos et qu'on
	 * sera capable de les récupérer par la suite
	 */
	assert(mem == get_system_memory_addr());
	assert(taille == get_system_memory_size());
	/* ... */
	struct fb* fb_new = get_system_memory_addr() + sizeof(struct allocator_header); //On créé notre zone libre positionné après l'header
	fb_new->size = taille - sizeof(struct allocator_header); //init à la taille global - le header d'avant
	fb_new->next = NULL; //Pas d'autre zones
	/* ... */
	mem_fit(&mem_fit_first);
}

void mem_show(void (*print)(void *, size_t, int)) {
	/* ... */
	struct fb* fb_all = get_system_memory_addr() + sizeof(struct allocator_header);
	struct fb* fb_next = get_system_memory_addr() + sizeof(struct allocator_header);
	int free;

	while ((void *)fb_all< (get_system_memory_addr()+get_system_memory_size())) { //Tant qu'on as pas parcouru toute notre zone

		if(fb_all<fb_next){	//Si notre next est plus grand que la ou on est
			free = 0;		//Alors zone occupé
		}
		else {	
			free = 1;		//Sinon libre et on next
			if(fb_next->next == NULL)
				fb_next= (get_system_memory_addr()+get_system_memory_size());
			else fb_next = fb_next->next;
		}
		print(fb_all, fb_all->size, free);

		//On passe à la zone suivante
		fb_all = (struct fb*)((size_t)(fb_all) + fb_all->size + (size_t)(sizeof(struct fb)));
	}
}

void mem_fit(mem_fit_function_t *f) {
	get_header()->fit = f;
}

void *mem_alloc(size_t taille) {
	__attribute__((unused)) /* juste pour que gcc compile ce squelette avec -Werror */

	struct fb *header = get_system_memory_addr() + sizeof(struct allocator_header); //Adresse à la première zone
	struct fb *zone_alloc = get_header()->fit(header, taille) ;//mem_fit_first(header, taille); //Notre potentiel zone libre

	//Si on a une zone libre valide
	if(zone_alloc!=NULL){
		//Si (taille+sonHeader est supérieur à la zone libre ET que la taille+sonHeader est inférieur à la zoneLibre+sonHeader) 
		/*
			On fait ceci pour éviter de perdre petit à petit de la mémoir
		*/
		
		//OU (si les 2 zones libres on la même taille)
		if((taille + sizeof(struct fb) > zone_alloc->size && taille + sizeof(struct fb) < zone_alloc->size + sizeof(struct fb)) || (taille + sizeof(struct fb) == zone_alloc->size)){
			//On récupère la zone libre avant notre allocation
			if(header->next != NULL)
				while(header->next<zone_alloc)
					header = header->next;

			//On a actuellement header->next = zone_alloc
			//On veu donc que heander->next = zone_alloc->next donc heander->next = header->next->next
			if(header->next->next!=NULL)
				header->next = header->next->next;

			zone_alloc->next = NULL;
			//(La size ne change pas dans ce cas)
			return zone_alloc;
		}else{
			zone_alloc->size -=(taille+(size_t)(sizeof(struct fb)));								//Mon modifie la taille de la zone libre
			struct fb *zone_occupy = (struct fb*)((size_t)(sizeof(struct fb) + zone_alloc->size + (size_t)(zone_alloc)));//On crée un nouveau header
			zone_occupy->size = taille;								//Le header à une size de taille
			return zone_occupy;
		}
	}else return NULL;
}

 	
void mem_free(void* mem) {
	struct fb *header = get_system_memory_addr() + sizeof(struct allocator_header); //Adresse à la première zone
	struct fb *occupe = mem;

	//Positionnement sur la zone libre avant notre zone occupé
	if(header->next != NULL)
		while(header->next<occupe)
			header = header->next;

	//Si zone libre avant la zone occupé alors fuuuuusion
	if((size_t)(header) + header->size + (size_t)(sizeof(struct fb)) == (size_t)occupe){
		header->size += occupe->size + sizeof(struct fb);		//nouvelle taille zone libre = sa taille + occupé + header occupé
		//Si zone occupé entre 2 zones libres alors fuuuuusion
		if((size_t)(header) + header->size + (size_t)(sizeof(struct fb)) == (size_t)(header->next)){
			occupe = header->next;
			header->size += occupe->size + sizeof(struct fb);
			header->next = occupe->next;
		} 
	}else if((size_t)(occupe) + occupe->size + (size_t)(sizeof(struct fb)) == (size_t)header->next){ //Si seulement zone libre après occupé
		struct fb *save = header->next;	//Zone libre après occupé
		header->next = occupe;			//La zone suivante de la zone précédente pointe maintenant sur occupe
		occupe->next = save->next;		//La zone libre suivante deviens notre zone d'après
		occupe->size += save->size + sizeof(struct fb);

	}else{ //Si pas de zone libre autours de occupé
		struct fb *save = header->next;
		header->next = occupe;
		occupe->next = save;
	}
	

}


struct fb* mem_fit_first(struct fb *list, size_t size) {
	bool first = true;

	while(list!=NULL){			//Tant qu'on est pas à la fin des zones libres

		/*
		On a décidé de garder notre premier herder libre peut importe se qu'il se passe, même si 
		sa taille doit passer à null.
		donc on gère le cas ou :
		*/
		//Si première zone libre > à la taille voulue plus la place pour mettre son header alors return
		if(list->size>size+sizeof(struct fb) && first){
			return list;
		}
		else if(list->size>=size && !first)	//Sino, on accepte d'ecraser le header
			return list;

		list = list->next;		//Sinon, on passe à la zone suivante
		if(first == true) first = false;
	} return NULL;				//Si pas de zone valide alors NULL
}

/* Fonction à faire dans un second temps
 * - utilisée par realloc() dans malloc_stub.c
 * - nécessaire pour remplacer l'allocateur de la libc
 * - donc nécessaire pour 'make test_ls'
 * Lire malloc_stub.c pour comprendre son utilisation
 * (ou en discuter avec l'enseignant)
 */
size_t mem_get_size(void *zone) {
	/* zone est une adresse qui a été retournée par mem_alloc() */

	/* la valeur retournée doit être la taille maximale que
	 * l'utilisateur peut utiliser dans cette zone */
	struct fb* get = zone;
	return get->size;
}

/* Fonctions facultatives
 * autres stratégies d'allocation
 */
struct fb* mem_fit_best(struct fb *list, size_t size) {
	bool first = true;
	struct fb* min = get_system_memory_addr() + sizeof(struct allocator_header);

	while(list!=NULL){
		if(list->size>size+sizeof(struct fb) && first){
			if(min->size >list->size)
				min = list;
		}
		else if(list->size>=size && !first)	//Sino, on accepte d'ecraser le header
			if(min->size > list->size)
				min = list;

		list = list->next;		//Sinon, on passe à la zone suivante
		if(first == true) first = false;
	} 
	return min;		
}

struct fb* mem_fit_worst(struct fb *list, size_t size) {
	bool first = true;
	struct fb* max =  get_system_memory_addr() + sizeof(struct allocator_header);

	while(list!=NULL){
		if(list->size>size+sizeof(struct fb) && first){
			if(max->size <list->size)
				max = list;
		}
		else if(list->size>=size && !first)	//Sino, on accepte d'ecraser le header
			if(max->size < list->size)
				max = list;

		list = list->next;		//Sinon, on passe à la zone suivante
		if(first == true) first = false;
	} 
	return max;
}
 