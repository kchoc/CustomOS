#include "system_init.h"
#include "process.h"
#include "elf.h"
#include "panic.h"

#include <sys/pcpu.h>

#include <vm/vm_map.h>

#include <kern/terminal.h>

#include <string.h>

void system_init() {
  proc_t* proc = create_process("init");
  if (!proc) PANIC("Failed to create init process!");

  vm_space_activate(proc->vmspace);
  vaddr_t load_addr = 0x1000000;
  vm_map_anon(proc->vmspace, &load_addr, 0x1000, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_USER, VM_REG_F_EARLYENTER);
  memcpy((void*)load_addr, (void*)load_init, 0x1000);
 
  thread_t* thread = create_user_thread((void*)load_addr, proc, 0, get_pcpu());
  if (!thread) PANIC("Failed to create init process task!");
}

