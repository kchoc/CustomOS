#ifndef PMAP_H
#define PMAP_H

#include <machine/pmap.h>

#include <vm/types.h>

extern pmap_t kernel_pmap_store;
#define kernel_pmap (&kernel_pmap_store)

int     pmap_init();
pmap_t* pmap_create();
void    pmap_debug(pmap_t* pmap);
void    pmap_destroy(pmap_t* pmap);
void    pmap_activate(pmap_t* pmap);

int     pmap_enter(pmap_t* pmap, vaddr_t virt, paddr_t phys, vm_prot_t prot, pmap_flags_t flags);
void    pmap_remove(pmap_t* pmap, vaddr_t sva, vaddr_t eva);
void    pmap_protect(pmap_t* pmap, vaddr_t sva, vaddr_t eva, vm_prot_t prot);
paddr_t pmap_extract(pmap_t* pmap, vaddr_t virt);

#endif // X86_PMAP_H
