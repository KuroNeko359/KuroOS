    .section .text.start
    .global _start
    
_start:
    # Set up stack pointer
    li sp, 0x40000000
    
    # Call main
    call main
    
    # If main returns, exit with code 0
    li a0, 0
    call exit
    
    # Should not reach here
1:  wfi
    j 1b
