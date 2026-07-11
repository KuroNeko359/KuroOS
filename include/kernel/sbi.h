#ifndef KERNEL_SBI_H
#define KERNEL_SBI_H

struct sbiret {
    long error;
    long value;
};

struct sbiret sbi_ecall(long ext, long fid,
                        long arg0, long arg1, long arg2,
                        long arg3, long arg4, long arg5);
void sbi_shutdown(void);
void sbi_set_timer(unsigned long stime_value);


#endif
