#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

long write(int fd, const char *buf, unsigned long len);
long read(int fd, char *buf, unsigned long len);
void yield(void);
void exit(int code);
long getpid(void);

#endif
