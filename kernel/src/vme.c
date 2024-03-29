#include "klib.h"
#include "vme.h"
#include "proc.h"

static TSS32 tss;

typedef union free_page {
  union free_page *next;
  char buf[PGSIZE];
} page_t;

page_t *free_page_list;

void init_gdt() {
  static SegDesc gdt[NR_SEG];
  gdt[SEG_KCODE] = SEG32(STA_X | STA_R,   0,     0xffffffff, DPL_KERN);
  gdt[SEG_KDATA] = SEG32(STA_W,           0,     0xffffffff, DPL_KERN);
  gdt[SEG_UCODE] = SEG32(STA_X | STA_R,   0,     0xffffffff, DPL_USER);
  gdt[SEG_UDATA] = SEG32(STA_W,           0,     0xffffffff, DPL_USER);
  gdt[SEG_TSS]   = SEG16(STS_T32A,     &tss,  sizeof(tss)-1, DPL_KERN);
  set_gdt(gdt, sizeof(gdt[0]) * NR_SEG);
  set_tr(KSEL(SEG_TSS));
}

void set_tss(uint32_t ss0, uint32_t esp0) {
  tss.ss0 = ss0;
  tss.esp0 = esp0;
}

static PD kpd;
static PT kpt[PHY_MEM / PT_SIZE] __attribute__((used));

void init_page() {
  extern char end;
  panic_on((size_t)(&end) >= KER_MEM - PGSIZE, "Kernel too big (MLE)");
  static_assert(sizeof(PTE) == 4, "PTE must be 4 bytes");
  static_assert(sizeof(PDE) == 4, "PDE must be 4 bytes");
  static_assert(sizeof(PT) == PGSIZE, "PT must be one page");
  static_assert(sizeof(PD) == PGSIZE, "PD must be one page");
  // Lab1-4: init kpd and kpt, identity mapping of [0 (or 4096), PHY_MEM)
  //TODO();
  //printf("start init kpd and kpt\n");
  memset(&kpd, 0, sizeof(kpd));
  for(int i = 0; i < PHY_MEM / PT_SIZE; i++){
    kpd.pde[i].val = MAKE_PDE(kpt + i, 7);
    for(int j = 0; j < NR_PTE; j++){
      kpt[i].pte[j].val = MAKE_PTE((i << DIR_SHIFT) | (j << TBL_SHIFT), 7);
    }
  }
  //printf("end init kpd and kpt\n");

  kpt[0].pte[0].val = 0;
  set_cr3(&kpd);
  set_cr0(get_cr0() | CR0_PG);
  // Lab1-4: init free memory at [KER_MEM, PHY_MEM), a heap for kernel
  //TODO();
  //printf("start init free memory at KER_MEM and PHY_MEM\n");
  free_page_list = (page_t *)PAGE_DOWN(KER_MEM);
  page_t *listPointer = free_page_list;
  int addrPointer = PAGE_DOWN(KER_MEM) + PGSIZE;
  while(addrPointer < PHY_MEM){
    // malloc a page_t ?
    //printf("in while start: addrPtr:0x%x\n", addrPointer);
    listPointer->next = (page_t *)addrPointer;
    addrPointer += PGSIZE;
    listPointer = listPointer->next;
    //printf("in while end\n");
  }
  listPointer->next = NULL; //end of the free_page_list
  //printf("end init free memory at KER_MEM and PHY_MEM\n");
}

void *kalloc() {
  // Lab1-4: alloc a page from kernel heap, abort when heap empty
  //TODO();
  //printf("kalloc\n");
  if(free_page_list == NULL){
    assert(0);
  }
  //printf("end kalloc\n");
  void *ret = (void *)(free_page_list);
  free_page_list = free_page_list->next;
  return ret;
}

void kfree(void *ptr) {
  // Lab1-4: free a page to kernel heap
  // you can just do nothing :)
  //TODO();
  //printf("kfree\n");
  if((int)ptr >= KER_MEM && (int)ptr < PHY_MEM){
    memset(ptr, 0, PGSIZE);
    page_t *temp = free_page_list;
    free_page_list = ptr;
    free_page_list->next = temp;
  }
}

PD *vm_alloc() {
  // Lab1-4: alloc a new pgdir, map memory under PHY_MEM identityly
  //TODO();
  //printf("vm_alloc\n");
  PD *pgdir = kalloc();
  // an unused pde is set all 0
  memset((void *)pgdir, 0, PGSIZE);
  for(int i = 0; i < PHY_MEM / PT_SIZE; i++){
    pgdir->pde[i].val = MAKE_PDE(kpt + i, 7);
    // for(int j = 0; j < NR_PTE; j++){
    //   kpt[i].pte[j].val = MAKE_PTE((i << DIR_SHIFT) | (j << TBL_SHIFT), 1);
    // }
  }
  //printf("end vm_alloc\n");
  return pgdir;
}

void vm_teardown(PD *pgdir) {
  // Lab1-4: free all pages mapping above PHY_MEM in pgdir, then free itself
  // you can just do nothing :)
  //TODO();
  for(int i = PHY_MEM / PT_SIZE; i < NR_PDE; i++){
    // an all 0 pde is unused so there's no need to kfree it
    if(pgdir->pde[i].present != 0){
      PDE *pde = &pgdir->pde[i];
      //printf("%d %d\n", pgdir->pde[i], (*pde));
      PT *pt = (PT *)PDE2PT(*pde); // pa of PT
      for(int j = 0; j < NR_PTE; j++){
        if(pt->pte[j].present == 0) continue;
        kfree(PTE2PG(pt->pte[j]));  //kfree all the PHY page
      }
      kfree((void *)pt); //kfree the PT of this PDE
    }
  }
  kfree(pgdir);
}

PD *vm_curr() {
  return (PD*)PAGE_DOWN(get_cr3());
}

PTE *vm_walkpte(PD *pgdir, size_t va, int prot) {
  // Lab1-4: return the pointer of PTE which match va
  // if not exist (PDE of va is empty) and prot&1, alloc PT and fill the PDE
  // if not exist (PDE of va is empty) and !(prot&1), return NULL
  // remember to let pde's prot |= prot, but not pte
  assert((prot & ~7) == 0);
  //TODO();
  int pd_index = ADDR2DIR(va);
  PDE *pde = &(pgdir->pde[pd_index]);
  if(pde->present != 0){
    PT *pt = PDE2PT(*pde);
    int pt_index = ADDR2TBL(va);
    PTE *pte = &(pt->pte[pt_index]);
    return pte;
  }else{
    if(prot != 0){
      //printf("vm_walkpte\n");
      PT *pt = kalloc();
      memset(pt, 0, PGSIZE);
      pde->val = MAKE_PDE(pt, 7);
      int pt_index = ADDR2TBL(va);
      PTE *pte = &(pt->pte[pt_index]);
      return pte;
    }else{
      return NULL;
    }
  }
}

void *vm_walk(PD *pgdir, size_t va, int prot) {
  // Lab1-4: translate va to pa
  // if prot&1 and prot voilation ((pte->val & prot & 7) != prot), call vm_pgfault
  // if va is not mapped and !(prot&1), return NULL
  //TODO();
  PTE *pte = vm_walkpte(pgdir, va, prot);
  if(pte == NULL) return NULL;
  else{
    void *page = PTE2PG(*pte);
    void *pa = (void *)((uint32_t)page | ADDR2OFF(va));
    return pa;
  }
}

void vm_map(PD *pgdir, size_t va, size_t len, int prot) {
  // Lab1-4: map [PAGE_DOWN(va), PAGE_UP(va+len)) at pgdir, with prot
  // if have already mapped pages, just let pte->prot |= prot
  assert(prot & PTE_P);
  assert((prot & ~7) == 0);
  size_t start = PAGE_DOWN(va);
  size_t end = PAGE_UP(va + len);
  assert(start >= PHY_MEM);
  assert(end >= start);
  //TODO();
  //printf("vm_map\n");
  for(size_t addr = PAGE_DOWN(va); addr < PAGE_UP(va + len); addr += PGSIZE){
    PTE *pte = vm_walkpte(pgdir, addr, prot);
    if(pte->present == 1){
      pte->val = pte->val | prot;
    }else{
      page_t *pg = kalloc();
      pte->val = MAKE_PTE(pg, 7);
    }
  }
}

void vm_unmap(PD *pgdir, size_t va, size_t len) {
  // Lab1-4: unmap and free [va, va+len) at pgdir
  // you can just do nothing :)
  //assert(ADDR2OFF(va) == 0);
  //assert(ADDR2OFF(len) == 0);
  //TODO();
  for(size_t addr = PAGE_DOWN(va); addr < PAGE_UP(va + len); addr += PGSIZE){
    //find pte
    //PTE *pte = vm_walkpte(pgdir, addr, 7);
    
    int pd_index = ADDR2DIR(addr);
    PDE *pde = &(pgdir->pde[pd_index]);
    PT *pt = PDE2PT(*pde);
    int pt_index = ADDR2TBL(addr);
    PTE *pte = &(pt->pte[pt_index]);
    
    //unmap
    if(pte->present != 0){
      page_t *pg = PTE2PG(*pte);
      kfree(pg);
    }
  }
  if(pgdir == vm_curr()){
    set_cr3(vm_curr());
  }
}

void vm_copycurr(PD *pgdir) {
  // Lab2-2: copy memory mapped in curr pd to pgdir
  //TODO();
  PD* pgdir_curr = vm_curr();

  for(size_t addr = PAGE_DOWN(PHY_MEM); addr < PAGE_UP(USR_MEM); addr += PGSIZE){
    PTE* pte = vm_walkpte(pgdir_curr, addr, 7);
    if(pte != NULL && pte->present){
      vm_map(pgdir, addr, PGSIZE, 7);
      void * phyaddr1 = vm_walk(pgdir, addr, 7);
      void * phyaddr2 = vm_walk(pgdir_curr, addr, 7);
      memcpy(phyaddr1, phyaddr2, PGSIZE);
    }
  }
}

void vm_pgfault(size_t va, int errcode) {
  printf("pagefault @ 0x%p, errcode = %d\n", va, errcode);
  panic("pgfault");
}
