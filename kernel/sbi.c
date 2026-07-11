#include <kernel/sbi.h>

#define SBI_EXT_SRST 0x53525354
#define SBI_SRST_RESET_TYPE_SHUTDOWN 0
#define SBI_SRST_RESET_REASON_NONE 0
#define SBI_EXT_TIME 0x54494D45 // TIME
#define SBI_TIME_SET_TIMER 0


/*
 * Call OpenSBI from S-mode.
 *
 * SBI is the S-mode-to-M-mode ABI. Arguments go in a0..a5, function id in a6,
 * extension id in a7, and ecall transfers control to OpenSBI.
 */
struct sbiret sbi_ecall(long ext, long fid,
                        long arg0, long arg1, long arg2,
                        long arg3, long arg4, long arg5)
{
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = ext;

    __asm__ volatile("ecall"
                     : "+r"(a0), "+r"(a1)
                     : "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                       "r"(a6), "r"(a7)
                     : "memory");

    return (struct sbiret){a0, a1};
}

/* Power off the machine through the SBI System Reset extension. */
void sbi_shutdown(void)
{
    sbi_ecall(SBI_EXT_SRST, 0,
              SBI_SRST_RESET_TYPE_SHUTDOWN,
              SBI_SRST_RESET_REASON_NONE,
              0, 0, 0, 0);
}



void sbi_set_timer(unsigned long stime_value)
{
    sbi_ecall(SBI_EXT_TIME, SBI_TIME_SET_TIMER,
              stime_value, 0, 0, 0, 0, 0);
}