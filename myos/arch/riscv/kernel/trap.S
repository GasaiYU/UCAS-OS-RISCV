#include <asm.h>
#include <csr.h>

ENTRY(setup_exception)
  /* TODO: 
   * save exception_handler_entry into STVEC
   * enable global exceptions */
 

  la s0, exception_handler_entry
  csrw CSR_STVEC, s0
  li s0, SR_SIE
  csrw CSR_SSTATUS, s0
  
 
  
  li s0, SIE_STIE | SIE_SEIE | SIE_SSIE
  csrw CSR_SIE, s0

  csrr s1, CSR_MHARTID
  beqz s1, end
  ori s0, s0, SIE_SSIE
  csrw CSR_SIE, s0
  end:
  jr ra

  
ENDPROC(setup_exception)


ENTRY(read_sip)
  csrr a0, CSR_SIP
  jr ra
ENDPROC(read_sip)