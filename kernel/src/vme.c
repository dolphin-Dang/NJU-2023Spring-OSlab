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
  memset(&kpd, 0, sizeof(kpd));
  for(int i = 0; i < PHY_MEM / PT_SIZE; i++){
    kpd.pde[i].val = MAKE_PDE(kpt + i, 1);
    for(int j = 0; j < NR_PTE; j++){
      kpt[i].pte[j].val = MAKE_PTE((i << DIR_SHIFT) | (j << TBL_SHIFT), 1);
    }
  }

  kpt[0].pte[0].val = 0;
  set_cr3(&kpd);
  set_cr0(get_cr0() | CR0_PG);
  // Lab1-4: init free memory at [KER_MEM, PHY_MEM), a heap for kernel
  //TODO();
  int addrPointer = PAGE_DOWN(KER_MEM);
  page_t *listPointer = free_page_list;
  while(addrPointer < PHY_MEM){
    listPointer->next = addrPointer;
    addrPointer += PGSIZE;
    listPointer = listPointer->next;
  }
  listPointer->next = NULL; //end of the free_page_list
}

void *kalloc() {
  // Lab1-4: alloc a page from kernel heap, abort when heap empty
  //TODO();
  if(free_page_list->next == NULL) assert(0);
  int ret = free_page_list->next;
  free_page_list->next = free_page_list->next->next;
  return (void *)ret;
}

void kfree(void *ptr) {
  // Lab1-4: free a page to kernel heap
  // you can just do nothing :)
  //TODO();
  if((int)ptr >= KER_MEM && ptr < PHY_MEM){
    memset(ptr, 0, PGSIZE);
    page_t *temp = free_page_list->next;
    free_page_list->next = ptr;
    free_page_list->next->next = temp;
  }
}

PD *vm_alloc() {
  // Lab1-4: alloc a new pgdir, map memory under PHY_MEM identityly
  //TODO();
  PD *pgdir = kalloc();
  // an unused pde is set all 0
  memset((void *)pgdir, 0, PGSIZE);
  for(int i = 0; i < PHY_MEM / PT_SIZE; i++){
    pgdir->pde[i].val = MAKE_PDE(pgdir + i, 1);
    // for(int j = 0; j < NR_PTE; j++){
    //   kpt[i].pte[j].val = MAKE_PTE((i << DIR_SHIFT) | (j << TBL_SHIFT), 1);
    // }
  }
  return pgdir;
}

void vm_teardown(PD *pgdir) {
  // Lab1-4: free all pages mapping above PHY_MEM in pgdir, then free itself
  // you can just do nothing :)
  //TODO();
  for(int i = PHY_MEM / PT_SIZE;i < NR_PDE; i++){
    // an all 0 pde is unused so there's no need to kfree it
    if(pgdir->pde[i].val == 0) break;
    else{
      PDE *pde = (PDE *)pgdir->pde[i].val;
      PT *pt = (PT *)PDE2PT(*pde); // pa of PT
      for(int j = 0; j < NR_PTE; j++){
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
  TODO();
}

void *vm_walk(PD *pgdir, size_t va, int prot) {
  // Lab1-4: translate va to pa
  // if prot&1 and prot voilation ((pte->val & prot & 7) != prot), call vm_pgfault
  // if va is not mapped and !(prot&1), return NULL
  TODO();
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
  TODO();
}

void vm_unmap(PD *pgdir, size_t va, size_t len) {
  // Lab1-4: unmap and free [va, va+len) at pgdir
  // you can just do nothing :)
  //assert(ADDR2OFF(va) == 0);
  //assert(ADDR2OFF(len) == 0);
  //TODO();
}

void vm_copycurr(PD *pgdir) {
  // Lab2-2: copy memory mapped in curr pd to pgdir
  TODO();
}

void vm_pgfault(size_t va, int errcode) {
  printf("pagefault @ 0x%p, errcode = %d\n", va, errcode);
  panic("pgfault");
}
