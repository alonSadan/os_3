#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"
int bkMemNum,bkSwapNum,bkPrioNum;
int exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3 + MAXARG + 1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();

  begin_op();

  if ((ip = namei(path)) == 0)
  {
    end_op();
    cprintf("exec: fail\n");
    return -1;
  }
  ilock(ip);
  pgdir = 0;

  // Check ELF header
  if (readi(ip, (char *)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if (elf.magic != ELF_MAGIC)
    goto bad;

  if ((pgdir = setupkvm()) == 0)
    goto bad;

  //backup
  bkMemNum = curproc->pagesInMemory;
  bkSwapNum =  curproc->pagesInSwapfile;
  bkPrioNum = curproc->prioSize;

  struct paging_meta_data bkRam[MAX_PSYC_PAGES];
  struct paging_meta_data bkSwapFile[MAX_PSYC_PAGES];
  struct heap_p bkPrioArr[MAX_PSYC_PAGES*2+1];

  for (int i = 0; i < MAX_PSYC_PAGES; i++)
  {
    bkRam[i] = curproc->ramPmd[i];
    bkSwapFile[i] = curproc->swapPmd[i];
  }

  for (int i = 0; i < MAX_PSYC_PAGES*2+1; i++)
  {
    bkPrioArr[i] = curproc->prioArr[i];
  }
  
  //reset
  for (int i = 0; i < MAX_PSYC_PAGES; i++)
  {
    curproc->swapPmd[i].va = (char *)-1;
    curproc->ramPmd[i].va = (char *)-1;
    curproc->swapPmd[i].occupied = 0;
    curproc->ramPmd[i].occupied = 0;
  }
 
  curproc->prioSize = 0; 
  curproc->pagesInMemory = 0;
  curproc->pagesInSwapfile = 0;

  // Load program into memory.
  sz = 0;
  for (i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph))
  {
    if (readi(ip, (char *)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if (ph.type != ELF_PROG_LOAD)
      continue;
    if (ph.memsz < ph.filesz)
      goto bad;
    if (ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    if ((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0) //allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
      goto bad;
    if (ph.vaddr % PGSIZE != 0)
      goto bad;
    if (loaduvm(pgdir, (char *)ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  sz = PGROUNDUP(sz);
  if ((sz = allocuvm(pgdir, sz, sz + 2 * PGSIZE)) == 0)
    goto bad;
  clearpteu(pgdir, (char *)(sz - 2 * PGSIZE));
  sp = sz;

  // Push argument strings, prepare rest of stack in ustack.
  for (argc = 0; argv[argc]; argc++)
  {
    if (argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if (copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3 + argc] = sp;
  }
  ustack[3 + argc] = 0;

  ustack[0] = 0xffffffff; // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc + 1) * 4; // argv pointer

  sp -= (3 + argc + 1) * 4;
  if (copyout(pgdir, sp, ustack, (3 + argc + 1) * 4) < 0)
    goto bad;

  // Save program name for debugging.
  for (last = s = path; *s; s++)
    if (*s == '/')
      last = s + 1;
  safestrcpy(curproc->name, last, sizeof(curproc->name));

  for (int i = 0; i < MAX_PSYC_PAGES; i++)  //mark new memory as free
  {
    if (curproc->ramPmd[i].occupied)
      curproc->ramPmd[i].pgdir = pgdir;
    if (curproc->swapPmd[i].occupied)
      curproc->swapPmd[i].pgdir = pgdir;
  }


  // Commit to the user image.
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->tf->eip = elf.entry; // main
  curproc->tf->esp = sp;

  //removeSwapFile(curproc); //remove old swap file and create new one
  //createSwapFile(curproc);
  
  //   memset(curproc->swapPmd,0,MAX_PSYC_PAGES*sizeof(struct paging_meta_data));
  // memset(curproc->ramPmd,0,MAX_PSYC_PAGES*sizeof(struct paging_meta_data));

  switchuvm(curproc);
  freevm(oldpgdir);

  

  return 0;

bad:
  cprintf("exec: bad scenario");
  if (pgdir)
    freevm(pgdir);
  if (ip)
  {
    iunlockput(ip);
    end_op();
  }

  //resote
  curproc->pagesInMemory = bkMemNum;
  curproc->pagesInSwapfile = bkSwapNum;
  curproc->prioSize = bkPrioNum;
    
  for (int i = 0; i < MAX_PSYC_PAGES; i++)
  {
    curproc->ramPmd[i] = bkRam[i];
    curproc->swapPmd[i] = bkSwapFile[i];
  }

  for (int i = 0; i < MAX_PSYC_PAGES*2+1; i++)
  {
    curproc->prioArr[i] = bkPrioArr[i];
  }

  return -1;
}
