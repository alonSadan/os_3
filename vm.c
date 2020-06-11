#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"

#define IN_SWAP 1
#define IN_PHY 0
#define OCCUPIED 1
#define VACANT 0

#define BUF_SIZE (PGSIZE/4)

#define CLEAN_VA ((char *) (-1))

// #define NONE
// #define NFUA
// #define LAPA
// #define SCIFO
// #define AQ

//int DEBUG = 0;
static char buffer[PGSIZE];


extern char data[]; // defined by kernel.ld
pde_t *kpgdir;      // for use in scheduler()

//static uint shiftCounter(int bit, int pageNum);
//static uint countSetBits(uint n);
//static void updatePageInPriorityQueue(int pageNum);
uint getPageIndexDefault(int inSwapFile, int isOccupied, char *va);

// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void seginit(void)
{
  struct cpu *c;

  // Map "logical" addresses to virtual addresses using identity map.
  // Cannot share a CODE descriptor for both kernel and user
  // because it would have to have DPL_USR, but the CPU forbids
  // an interrupt from CPL=0 to DPL=3.
  c = &cpus[cpuid()];
  c->gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0, 0xffffffff, 0);
  c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
  c->gdt[SEG_UCODE] = SEG(STA_X | STA_R, 0, 0xffffffff, DPL_USER);
  c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
  lgdt(c->gdt, sizeof(c->gdt));
}

// Return the address of the PTE in page table pgdir
// that corresponds to virtual address va.  If alloc!=0,
// create any required page table pages.
static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
  pde_t *pde;
  pte_t *pgtab;

  pde = &pgdir[PDX(va)];
  if (*pde & PTE_P) //if page table entry is present
  {
    pgtab = (pte_t *)P2V(PTE_ADDR(*pde));
  }
  else
  {
    if (!alloc || (pgtab = (pte_t *)kalloc()) == 0)
      return 0;
    // Make sure all those PTE_P bits are zero.
    memset(pgtab, 0, PGSIZE);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table
    // entries, if necessary.
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  return &pgtab[PTX(va)];
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned.
static int
mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
{
  char *a, *last;
  pte_t *pte;

  a = (char *)PGROUNDDOWN((uint)va);
  last = (char *)PGROUNDDOWN(((uint)va) + size - 1);
  for (;;)
  {
    if ((pte = walkpgdir(pgdir, a, 1)) == 0)
      return -1;
    if (*pte & PTE_P)
      panic("remap");
    *pte = pa | perm | PTE_P;
    if (a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

// There is one page table per process, plus one that's used when
// a CPU is not running any process (kpgdir). The kernel uses the
// current process's page table during system calls and interrupts;
// page protection bits prevent user code from using the kernel's
// mappings.
//
// setupkvm() and exec() set up every page table like this:
//
//   0..KERNBASE: user memory (text+data+stack+heap), mapped to
//                phys memory allocated by the kernel
//   KERNBASE..KERNBASE+EXTMEM: mapped to 0..EXTMEM (for I/O space)
//   KERNBASE+EXTMEM..data: mapped to EXTMEM..V2P(data)
//                for the kernel's instructions and r/o data
//   data..KERNBASE+PHYSTOP: mapped to V2P(data)..PHYSTOP,
//                                  rw data + free physical memory
//   0xfe000000..0: mapped direct (devices such as ioapic)
//
// The kernel allocates physical memory for its heap and for user memory
// between V2P(end) and the end of physical memory (PHYSTOP)
// (directly addressable from end..P2V(PHYSTOP)).

// This table defines the kernel's mappings, which are present in
// every process's page table.
static struct kmap
{
  void *virt;
  uint phys_start;
  uint phys_end;
  int perm;
} kmap[] = {
    {(void *)KERNBASE, 0, EXTMEM, PTE_W},            // I/O space
    {(void *)KERNLINK, V2P(KERNLINK), V2P(data), 0}, // kern text+rodata
    {(void *)data, V2P(data), PHYSTOP, PTE_W},       // kern data+memory
    {(void *)DEVSPACE, DEVSPACE, 0, PTE_W},          // more devices
};

// Set up kernel part of a page table.
pde_t *
setupkvm(void)
{
  pde_t *pgdir;
  struct kmap *k;

  if ((pgdir = (pde_t *)kalloc()) == 0)
    return 0;
  memset(pgdir, 0, PGSIZE);
  if (P2V(PHYSTOP) > (void *)DEVSPACE)
    panic("PHYSTOP too high");
  for (k = kmap; k < &kmap[NELEM(kmap)]; k++)
    if (mappages(pgdir, k->virt, k->phys_end - k->phys_start,
                 (uint)k->phys_start, k->perm) < 0)
    {
      freevm(pgdir);
      return 0;
    }
  return pgdir;
}

// Allocate one page table for the machine for the kernel address
// space for scheduler processes.
void kvmalloc(void)
{
  kpgdir = setupkvm();
  switchkvm();
}

// Switch h/w page table register to the kernel-only page table,
// for when no process is running.
void switchkvm(void)
{
  lcr3(V2P(kpgdir)); // switch to the kernel page table
}

// Switch TSS and h/w page table to correspond to process p.
void switchuvm(struct proc *p)
{
  if (p == 0)
    panic("switchuvm: no process");
  if (p->kstack == 0)
    panic("switchuvm: no kstack");
  if (p->pgdir == 0)
    panic("switchuvm: no pgdir");

  pushcli();
  mycpu()->gdt[SEG_TSS] = SEG16(STS_T32A, &mycpu()->ts,
                                sizeof(mycpu()->ts) - 1, 0);
  mycpu()->gdt[SEG_TSS].s = 0;
  mycpu()->ts.ss0 = SEG_KDATA << 3;
  mycpu()->ts.esp0 = (uint)p->kstack + KSTACKSIZE;
  // setting IOPL=0 in eflags *and* iomb beyond the tss segment limit
  // forbids I/O instructions (e.g., inb and outb) from user space
  mycpu()->ts.iomb = (ushort)0xFFFF;
  ltr(SEG_TSS << 3);
  lcr3(V2P(p->pgdir)); // switch to process's address space
  popcli();
}

// Load the initcode into address 0 of pgdir.
// sz must be less than a page.
void inituvm(pde_t *pgdir, char *init, uint sz)
{
  char *mem;

  if (sz >= PGSIZE)
    panic("inituvm: more than a page");
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  mappages(pgdir, 0, PGSIZE, V2P(mem), PTE_W | PTE_U);
  memmove(mem, init, sz);
}

// Load a program segment into pgdir.  addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
{
  uint i, pa, n;
  pte_t *pte;

  if ((uint)addr % PGSIZE != 0)
    panic("loaduvm: addr must be page aligned");
  for (i = 0; i < sz; i += PGSIZE)
  {
    if ((pte = walkpgdir(pgdir, addr + i, 0)) == 0)
      panic("loaduvm: address should exist");
    pa = PTE_ADDR(*pte);
    if (sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if (readi(ip, P2V(pa), offset + i, n) != n)
      return -1;
  }
  return 0;
}

void insertPageToPrioQueue(int pageNum)
{
  struct proc *p = myproc();  
  //remove page if exist
  if (findInHeap(p->prioArr, pageNum, &p->prioSize).index != -1)
    deleteRoot(p->prioArr, pageNum, &p->prioSize); //return;

#if DEBUG
  //cprintf("insertPageToPrioQueue: insert pageNum:%d to prioQueue\n",pageNum);
#endif

#if NFUA
  p->ramPmd[pageNum].age = 0;
  insertHeap(p->prioArr, (struct heap_p){pageNum, p->ramPmd[pageNum].age}, &p->prioSize);
#endif
#if LAPA
  p->ramPmd[pageNum].age = 0xFFFFFFFF;
  insertHeap(p->prioArr, (struct heap_p){pageNum, countSetBits(p->ramPmd[pageNum].age)}, &p->prioSize);
#endif
#if SCFIFO | AQ
  insertHeap(p->prioArr, (struct heap_p){pageNum, p->prioArr[p->prioSize].priority + 1}, &p->prioSize);
#endif

}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  char *mem;
  uint a;
  uint indx;
  struct proc *p = myproc();
  //pte_t *pte;
  if (newsz >= KERNBASE)
    return 0;
  if (newsz < oldsz)
    return oldsz;

  #if DEBUG
    cprintf("allocuvm pid%d: oldsz:%d,newsz:%d\n",p->pid,oldsz,newsz);
  #endif

  int no_skip = 1;

  a = PGROUNDUP(oldsz);
  for (; a < newsz; a += PGSIZE)
  {
    #if NONE
      no_skip = 1;
    #endif
    
    if (no_skip){
      if (p->pagesInMemory >= MAX_PSYC_PAGES)
      {
        if (p->pagesInSwapfile == MAX_TOTAL_PAGES - MAX_PSYC_PAGES)
          panic("allocuvm: No free sapce either in ram and disc");
        
        if(p->pid > 2)
          updatePagesInPriorityQueue();
        pageToSwapFile(getPageIndex(IN_PHY,OCCUPIED,CLEAN_VA), 
          getPageIndex(IN_SWAP, VACANT, CLEAN_VA), pgdir);
      }
    }

    mem = kalloc();
    if (mem == 0)
    {
      cprintf("allocuvm out of memory\n");
      deallocuvm(pgdir, newsz, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    if (mappages(pgdir, (char *)a, PGSIZE, V2P(mem), PTE_W | PTE_U) < 0)
    {
      cprintf("allocuvm out of memory (2)\n");
      deallocuvm(pgdir, newsz, oldsz);
      kfree(mem);
      return 0;
    }

    if(no_skip){
        indx = getPageIndex(IN_PHY, VACANT, CLEAN_VA);
        p->ramPmd[indx].va = (char *)a;
        p->ramPmd[indx].pgdir = pgdir;
        p->ramPmd[indx].occupied = 1;
        p->pagesInMemory++;
        insertPageToPrioQueue(indx);

       #if DEBUG
        cprintf("allocuvm: a:%d, pages in memory:%d pages in swapFile:%d\n",a,p->pagesInMemory,p->pagesInSwapfile);
       #endif
    }
  }
  return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  pte_t *pte;
  uint a, pa, idex;
  struct proc *p = myproc();
  if (newsz >= oldsz)
    return oldsz;
  a = PGROUNDUP(newsz);
  for (; a < oldsz; a += PGSIZE)
  {
    pte = walkpgdir(pgdir, (char *)a, 0);
    if (!pte)
      a = PGADDR(PDX(a) + 1, 0, 0) - PGSIZE;
    else if ((*pte & PTE_P) != 0)
    {
      pa = PTE_ADDR(*pte);
      if (pa == 0)
        panic("kfree");
      char *v = P2V(pa);
      if (decrementReferencesAndGetPrevVal(v) == 1){
        kfree(v);
      }
      if (p->pid > 2){
        idex = getPagePgdirIndex(IN_PHY, pgdir, (char *)a);
        //idex = getPageIndex(IN_PHY, OCCUPIED, (char *)a);
        if (idex != -1)
        {
        
          if (p->ramPmd[idex].occupied)
          {
            --p->pagesInMemory;
            deleteRoot(p->prioArr, idex, &p->prioSize);
            initPmd(&p->ramPmd[idex]);
          }       
        }

        //ToDo: check if need to deallocuvm for PG as well    
      }

      *pte = 0;
    }
    
  }
  return newsz;
}

// Free a page table and all the physical memory pages
// in the user part.
void freevm(pde_t *pgdir)
{
  uint i;
  if (pgdir == 0)
    panic("freevm: no pgdir");
  deallocuvm(pgdir, KERNBASE, 0);
  for (i = 0; i < NPDENTRIES; i++)
  {
    if (pgdir[i] & PTE_P)
    {
      char *v = P2V(PTE_ADDR(pgdir[i]));
      if (decrementReferencesAndGetPrevVal(v) == 1){
        kfree(v);
      }
    }
  }
  kfree((char *)pgdir);
}

// Clear PTE_U on a page. Used to create an inaccessible
// page beneath the user stack.
void clearpteu(pde_t *pgdir, char *uva)
{
  pte_t *pte;

  pte = walkpgdir(pgdir, uva, 0);
  if (pte == 0)
    panic("clearpteu");
  *pte &= ~PTE_U;
}

pde_t *
copyuvm(pde_t *pgdir, uint sz)
{
  pde_t *d;
  pte_t *pte;
  uint pa, i, flags;
  char *mem;

  if ((d = setupkvm()) == 0)
    return 0;
  for (i = 0; i < sz; i += PGSIZE)
  {
    if ((pte = walkpgdir(pgdir, (void *)i, 0)) == 0)
      panic("copyuvm: pte should exist");
    if (!(*pte & PTE_P)) //
      panic("copyuvm: page not present");

    pa = PTE_ADDR(*pte);
    flags = PTE_FLAGS(*pte);
    if ((mem = kalloc()) == 0)
      goto bad;
    memmove(mem, (char *)P2V(pa), PGSIZE);                   //we need to copy the data from the parent page the the child page we just allocated
    if (mappages(d, (void *)i, PGSIZE, V2P(mem), flags) < 0) //and map the virtual address to the physical addres we just allocated
    {
      kfree(mem);
      goto bad;
    }
  }
  return d;

bad:
  freevm(d);
  return 0;
}



// Given a parent process's page table, create a copy
// of it for a child.
pde_t *
cowuvm(pde_t *pgdir, uint sz)
{
  pde_t *d;
  pte_t *pte;
  uint pa, i, flags;
  // char *mem;

  if ((d = setupkvm()) == 0)
    return 0;
  for (i = 0; i < sz; i += PGSIZE)
  {
    if ((pte = walkpgdir(pgdir, (void *)i, 0)) == 0)
      panic("copyuvm: pte should exist");
    if (!(*pte & PTE_P) && !(*pte & PTE_PG)) //
      panic("copyuvm: page not present");

    if(*pte & PTE_P){
      if(*pte & PTE_W)
        *pte  &= ~PTE_W;
      *pte |= PTE_COW;     
    }

    //mark ad read only
   
    pa = PTE_ADDR(*pte);
    flags = PTE_FLAGS(*pte);
    if (mappages(d, (void *)i, PGSIZE, pa, flags) < 0)
    {
      // kfree(mem);
      goto bad;
    }

    if (*pte & PTE_P){
      incrementReferences(P2V(pa));    
    }else if (*pte & PTE_PG){
      pte_t * d_pte = walkpgdir(d, (char *)i, 0);      
      *d_pte &= ~PTE_P;                   //not in ram
    }
    
    lcr3(V2P(pgdir));
  }
  return d;

bad:
  freevm(d);
  return 0;
}

//PAGEBREAK!
// Map user virtual address to kernel address.
char *
uva2ka(pde_t *pgdir, char *uva)
{
  pte_t *pte;

  pte = walkpgdir(pgdir, uva, 0);
  if ((*pte & PTE_P) == 0)
    return 0;
  if ((*pte & PTE_U) == 0)
    return 0;
  return (char *)P2V(PTE_ADDR(*pte));
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for PTE_U pages.
int copyout(pde_t *pgdir, uint va, void *p, uint len)
{
  char *buf, *pa0;
  uint n, va0;

  buf = (char *)p;
  while (len > 0)
  {
    va0 = (uint)PGROUNDDOWN(va);
    pa0 = uva2ka(pgdir, (char *)va0);
    if (pa0 == 0)
      return -1;
    n = PGSIZE - (va - va0);
    if (n > len)
      n = len;
    memmove(pa0 + (va - va0), buf, n);
    len -= n;
    buf += n;
    va = va0 + PGSIZE;
  }
  return 0;
}


//
void pageToSwapFile(int memIndex, int swapIndex, pde_t *pgdir)
{
  struct proc *p = myproc();
  p->pagedout++;

  #if DEBUG
    cprintf("pageToSwapFile pid%d: memIndex:%d mem va:%p,swapIndex:%d\n",p->pid,memIndex,p->ramPmd[memIndex].va,swapIndex);
  #endif

  
  if (memIndex == -1)
    panic("no valid memIndex");

  if (swapIndex == -1){
    cprintf("swapPages: pid of panic process is %d\n", p->pid);
    panic("no space left in swapFile");
  }

  //insert founded page to SwapFileArr
  p->swapPmd[swapIndex].pgdir = p->ramPmd[memIndex].pgdir;
  p->swapPmd[swapIndex].offset = swapIndex * PGSIZE;
  p->swapPmd[swapIndex].occupied = 1;
  p->swapPmd[swapIndex].va = p->ramPmd[memIndex].va;
  p->pagesInSwapfile++;

  // #if DEBUG
  //   cprintf("pageToSwapFile: inserted page with va:%p to swapFile\n",p->swapPmd[swapIndex].va);
  // #endif

  //remove founded page from ramArr
  struct paging_meta_data pageout = p->ramPmd[memIndex];
  // #if DEBUG
  //   cprintf("pageToSwapFile: remove page from mem with va:%p\n",p->ramPmd[memIndex].va);
  // #endif

  deleteRoot(p->prioArr,memIndex,&p->prioSize);
  initPmd(&p->ramPmd[memIndex]);
  p->pagesInMemory--;

  char *va = pageout.va;
  pte_t *pte = walkpgdir(pgdir, va, 0);
  uint pa = PTE_ADDR(*pte);
  memmove(buffer,P2V(pa),PGSIZE);
  writeToSwapFile(p, buffer, swapIndex * PGSIZE, PGSIZE);
 
  // #if DEBUG
  //   cprintf("pageToSwapFile: kfree va:%p, p2v(pa):%p\n",va,P2V(pa));
  // #endif
  
  if (decrementReferencesAndGetPrevVal(P2V(pa)) == 1){
    kfree(P2V(pa));
  }

  //set flags for page in swapfile and update address in pte
  *pte &= ~PTE_P; //not in ram
  *pte |= PTE_PG; //in swapFile
  lcr3(V2P(p->pgdir));
}

uint getPageIndex(int inSwapFile, int isOccupied, char *va)
{
  if (!isOccupied || inSwapFile)
  {
    return getPageIndexDefault(inSwapFile, isOccupied, va);
  }

#if NFUA | LAPA | SCFIFO | AQ
  struct proc *p = myproc();
  //if (p->prioSize != p->pagesInMemory)
  //initPagesInPriorityQueue();
  if (p->prioSize == 0)
    return -1;
  else
    return peekHeap(p->prioArr).index;
#endif

#if NONE
  return getPageIndexDefault(inSwapFile, isOccupied, va);
#endif

  //otherwise: default (none)
  return getPageIndexDefault(inSwapFile, isOccupied, va);
}

uint getPageIndexDefault(int inSwapFile, int isOccupied, char *va)
{
  struct proc *p = myproc();
  struct paging_meta_data *pg;
  pg = inSwapFile ? p->swapPmd : p->ramPmd;
  if (inSwapFile)
  {
    for (int c = 0; pg < &p->swapPmd[MAX_PSYC_PAGES]; pg++, c++)
    {
      if (pg->occupied == isOccupied)
      {
        if (va == (char *)-1 || pg->va == va)
        {
          return c;
        }
      }
    }
  }
  else
  {
    for (int c = 0; pg < &p->ramPmd[MAX_PSYC_PAGES]; pg++, c++)
    {
      if (pg->occupied == isOccupied)
      {
        if (va == (char *)-1 || pg->va == va)
        {
          return c;
        }
      }
    }
  }

  return -1;
}

uint getPagePgdirIndex(int inSwapFile, pde_t *pgdir, char *va)
{
  struct proc *p = myproc();
  struct paging_meta_data *pg;
  pg = inSwapFile ? p->swapPmd : p->ramPmd;
  if (inSwapFile){
    for (int c = 0; pg < &p->swapPmd[MAX_PSYC_PAGES]; pg++, c++){
      if (pg->pgdir == pgdir && pg->va == va)
          return c;
    }
  }else{
    for (int c = 0; pg < &p->ramPmd[MAX_PSYC_PAGES]; pg++, c++){
      if (pg->pgdir == pgdir && pg->va == va)
          return c;
    }
  }
  return -1;
}


void onPageFaultCow(char *va1){
  struct proc *p = myproc();
  pte_t *pte = walkpgdir(p->pgdir, va1, 0);
  uint flags,pa; //err = p->tf->err
  char *mem;
  pa = PTE_ADDR(*pte);      
  flags = PTE_FLAGS(*pte);

  //notice: only dec if refs > 1  
  if (decrementReferencesAndGetPrevVal(P2V(pa)) > 1){        
    if ((mem = kalloc()) == 0){
      cprintf("onpagefault: cow: mem failed\n");
      p->killed = 1;
      return;
    }

    // Copies memory from the virtual address gotten from fault pte and copies PGSIZE bytes to mem
    memmove(mem, (char *)P2V(pa), PGSIZE);
    *pte = V2P(mem) | flags | PTE_P | PTE_W;
    *pte &= ~PTE_COW;
  }else{
    #if DEBUG
      cprintf("cow:pid%d ref = 1 so change it to writeable: va1:%p,pgdir:%p\n",p->pid,va1,p->pgdir);
    #endif 
    *pte |= PTE_W;
    *pte &= ~PTE_COW;
  }
  lcr3(V2P(p->pgdir));
  return;
}

void onPageFaultPG(char *va1){
  struct proc * p= myproc();
  //pte_t *pte = walkpgdir(p->pgdir, va1, 0);
  //move to swap file if required
  #if DEBUG
    cprintf("onPageFaultPG pid%d: va1:%p\n",p->pid,va1);
  #endif


  if(p->pagesInMemory == MAX_PSYC_PAGES){
    if (p->pagesInSwapfile == MAX_TOTAL_PAGES - MAX_PSYC_PAGES)
      panic("onPageFaultPG: No free sapce either in ram and disc");

    pageToSwapFile(getPageIndex(IN_PHY,OCCUPIED,CLEAN_VA), 
      getPageIndex(IN_SWAP, VACANT, CLEAN_VA), p->pgdir);
  }
  //remove page from swapArr:
  uint swapIndx = getPageIndex(IN_SWAP, OCCUPIED, va1);
  if(swapIndx == -1){
    panic("onPageFaultPG: page nout found in swapFile");
  }

  p->swapPmd[swapIndx].occupied = 0;
  p->pagesInSwapfile--;

  //increase age
  increaseProcAge();

  //insert page to memory
  int ramIndex = getPageIndex(IN_PHY,VACANT,CLEAN_VA);
  if (ramIndex == -1)
    panic("onpagefault: no free space in memory");

  #if DEBUG
    cprintf("onpagefault: insert mem pmd to index%d\n",ramIndex);
  #endif

  p->ramPmd[ramIndex].va = va1;
  p->ramPmd[ramIndex].pgdir = p->pgdir;
  p->ramPmd[ramIndex].occupied = 1;
  insertPageToPrioQueue(ramIndex);

  //alloc mem and map it like in allocuvm
  char *mem;
  mem = kalloc();
  if (mem == 0)
  {
    cprintf("onPageFault:kalloc out of memory\n");
    return;
  }

  memset(mem, 0, PGSIZE);
  if (mappages(p->pgdir, (void*)va1, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
    panic("onPageFaultPG: failed to mappages");
  }
  readFromSwapFile(p,buffer,p->swapPmd[swapIndx].offset,PGSIZE);
  memmove(mem,buffer,PGSIZE);
  p->pagesInMemory++;
}

void onPageFault(uint va)
{ 
  //to get the start of the va's page
  struct proc *p = myproc();
  p->pagefaults++;
  //moved update to here to avoid sync issues
  if(p->pid > 2) 
    updatePagesInPriorityQueue();
  
  //round to start of the page
  char *va1 = (char *)PGROUNDDOWN(va);
  pte_t *pte; 

  if (va >= KERNBASE || (pte = walkpgdir(p->pgdir, va1, 0)) == 0)
  {
    cprintf("pid %d %s: Page fault: access to invalid address.va1:%p,pte:%p\n", p->pid, p->name,va1,pte);
    p->killed = 1;
    return;
  }

  #if DEBUG
    cprintf("onpagefault pid:%d va1:%p\n",p->pid,va1);
  #endif

  //ToDo: check tf-err maybe
  if(*pte & PTE_COW){
    onPageFaultCow(va1);
    return;
  }

  if(*pte & PTE_PG){
    onPageFaultPG(va1);
    return;
  }

  //probably a bug...
  p->killed = 1;
}

void swapPrio(uint *a, uint *b)
{
  uint temp = *b;
  *b = *a;
  *a = temp;
}

uint shiftCounter(int bit, int pageNum)
{
  return (bit << 31) | (myproc()->ramPmd[pageNum].age >> 1);
}

uint countSetBits(uint n)
{
  uint count = 0;
  while (n)
  {
    count += n & 1;
    n >>= 1;
  }
  return count;
}

void updatePageInPriorityQueue(int pageNum)
{
  struct proc *p = myproc();
  pte_t *pte = walkpgdir(p->pgdir, p->ramPmd[pageNum].va, 0);
  if(p->pid & *pte){}
  //insert the priority queue if not already exist

#if NFUA
  if (p->ramPmd[pageNum].occupied)
      deleteRoot(p->prioArr, pageNum, &p->prioSize); //we delete node from heap to make sure replace pld node with new node

  if (*pte & PTE_A)
  {
    p->ramPmd[pageNum].age = shiftCounter(1, pageNum);
    insertHeap(p->prioArr, (struct heap_p){pageNum, p->ramPmd[pageNum].age}, &p->prioSize);
    (*pte) &= ~PTE_A;
  }
  else
  {
    p->ramPmd[pageNum].age = shiftCounter(0, pageNum);
    insertHeap(p->prioArr, (struct heap_p){pageNum, p->ramPmd[pageNum].age}, &p->prioSize);
  }
#endif

#if LAPA
  if (p->ramPmd[pageNum].occupied)
      deleteRoot(p->prioArr, pageNum, &p->prioSize);
      
  if (*pte & PTE_A)
  {
    p->ramPmd[pageNum].age = shiftCounter(1, pageNum);
    insertHeap(p->prioArr, (struct heap_p){pageNum, countSetBits(p->ramPmd[pageNum].age)}, &p->prioSize);
    (*pte) &= ~PTE_A;
  }
  else
  {
    p->ramPmd[pageNum].age = shiftCounter(0, pageNum);
    insertHeap(p->prioArr, (struct heap_p){pageNum, countSetBits(p->ramPmd[pageNum].age)}, &p->prioSize);
  }
#endif

#if SCFIFO
  if (*pte & PTE_A)
  {
    if (p->ramPmd[pageNum].occupied){
      //if (DEBUG) cprintf("SCFIFO: pid%d pageNum:%d with va:%p was good boy -> transfer it to last place\n",p->pid,pageNum,p->ramPmd[pageNum].va);
      deleteRoot(p->prioArr, pageNum, &p->prioSize);
    }
    //pushing page to the end of the list by giving him max prio + 1
    insertHeap(p->prioArr, (struct heap_p){pageNum, p->prioArr[p->prioSize - 1].priority + 1}, &p->prioSize);
    (*pte) &= ~PTE_A;
  }
  else
  {
  }

#endif

//ToDo: maybe change AQ to normal linkedlist for better performance (reduce from O(nlogn) to O(n))
#if AQ
  if (*pte & PTE_A)
  {
    if (p->prioArr[p->prioSize - 1].index != pageNum)
    {
      struct heap_p a = deleteRoot(p->prioArr, pageNum, &p->prioSize);
      if (a.index == -1)
      {
        insertHeap(p->prioArr, (struct heap_p){pageNum, p->prioArr[p->prioSize - 1].priority + 1}, &p->prioSize);
      }
      else
      {
        int tempSize = p->prioSize;
        struct heap_p temp[tempSize];
        for (int i = 0; i < tempSize; i++)
          temp[i] = p->prioArr[i];
        while ((a = extractMin(temp, &tempSize)).index != pageNum)
        {
        }
        struct heap_p b = extractMin(temp, &tempSize);
        deleteRoot(p->prioArr, a.index, &p->prioSize);
        deleteRoot(p->prioArr, b.index, &p->prioSize);
        swapPrio(&a.priority, &b.priority);
        insertHeap(p->prioArr, a, &p->prioSize);
        insertHeap(p->prioArr, b, &p->prioSize);
      }
    }
  }

#endif

lcr3(V2P(p->pgdir));


}

void updatePagesInPriorityQueue()
{
  struct proc *p = myproc();
  if (p->pid <= 2)
    return;
  
#if NFUA | LAPA
  for (int i = 0; i < MAX_PSYC_PAGES; i++){
    if (p->ramPmd[i].occupied)
      updatePageInPriorityQueue(i);
  }
#endif

#if SCFIFO | AQ
  int tempSize = p->prioSize; //get size of priority queue
  struct heap_p temp[tempSize];
  for (int i = 0; i < tempSize; i++) //why go by the order?
    temp[i] = p->prioArr[i];
  //update pages from sort array
  while (tempSize)
    updatePageInPriorityQueue(extractMin(temp, &tempSize).index);
#endif
}

//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.