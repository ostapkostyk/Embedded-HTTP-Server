/**
 * this is based on this post
 * http://stm32discovery.nano-age.co.uk/open-source-development-with-the-stm32-discovery/getting-newlib-to-work-with-stm32-and-code-sourcery-lite-eabi
 */
#include "common.h"
#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>

#include "CircularBuffer.h"

#undef errno
extern int errno;

/*
 environ
 A pointer to a list of environment variables and their values.
 For a minimal environment, this empty list is adequate:
 */
char* __env[1] = { 0 };
char** environ = __env;

caddr_t _sbrk(int incr);
//clock_t _times(struct tms* buf);
int _close(int file);
int _execve(char* name, char** argv, char** env);
void _exit(int status);
//int _fork(void);
int _fstat(int file, struct stat* st);
//int _getpid(void);
int _isatty(int file);
//int _kill(int pid, int sig);
int _link(char* old, char* new);
int _lseek(int file, int ptr, int dir);
int _read(int file, char* ptr, int len);
//int _stat(const char* filepath, struct stat* st);
//int _unlink(char* name);
//int _wait(int* status);
int _write(int file, char* ptr, int len);

void _exit(int i)
{
    while (1);
}
/***************************************************************************/
caddr_t
_sbrk(int incr);

// ----------------------------------------------------------------------------

// The definitions used here should be kept in sync with the
// stack definitions in the linker script.

/*
 sbrk
 Increase program data space.
 Malloc and related functions depend on this
 */
caddr_t
_sbrk(int incr)
{

  extern char _ebss;        // Defined by the linker
  static char* heap_end;
  char* prev_heap_end;

  if (heap_end == 0) {
    heap_end = &_ebss;
  }
  prev_heap_end = heap_end;

  char* stack = (char*)__get_MSP();
  if (heap_end + incr > stack) {
#if 0
    _write(STDERR_FILENO, "Heap and stack collision\n", 25);
#endif
    errno = ENOMEM;

    return (caddr_t)-1;
    // abort ();
  }

  heap_end += incr;
  return (caddr_t)prev_heap_end;
}

//*****************************************************
extern UART_HandleTypeDef stdinout_huart;

/**********************************************************************************************************
 read
 Read a character to a file. `libc' subroutines will use this system routine for
 input from all files, including stdin
 Returns -1 on error or blocks until the number of characters have been read.
 **********************************************************************************************************/

int
_read(int file, char* ptr, int len)
{
  int num = 0;
  circular_buffer* cb_Rx;
  cb_Rx = (circular_buffer*)(stdinout_huart.pRxBuffPtr);
  char tmp;

  switch (file) {
    case STDIN_FILENO:
      /* we could read from USART here */
        num = 0;
        while(num < len)
        {
            if(SUCCESS == cb_pop_front(cb_Rx, &tmp))
            {
                *(ptr + num) = tmp;
                num++;
                if(tmp == EOF) { return num; }
                if(tmp == '\n') { return num; }
            }
        }
      break;

    default:
      errno = EBADF;
      return -1;
  }
  return num;
}

/*********************************************************************************************************
 write
 Write a character to a file. `libc' subroutines will use this system routine
 for output to all files, including stdout
 Returns -1 on error or number of bytes sent
 *********************************************************************************************************/
int
_write(int file, char* ptr, int len)
{
    int num = 0;
    circular_buffer* cb_Tx;

    cb_Tx = (circular_buffer*)(stdinout_huart.pTxBuffPtr);

  switch (file) {
    case STDERR_FILENO: /* stderr */
      /* we could write to USART here */
      //break;  //  write both stderr and stdout to the same uart
    case STDOUT_FILENO: /*stdout*/
      /* we could write to USART here */
        __HAL_UART_DISABLE_IT(&stdinout_huart, UART_IT_TXE);
        while(len)
        {
           if(SUCCESS != cb_push_back(cb_Tx, (ptr + num)))
           {
               __HAL_UART_ENABLE_IT(&stdinout_huart, UART_IT_TXE);
               return num;
           }
           else
           {
               num++;
               len--;
           }
        }
        /* Enable the UART Transmit Data Register Empty Interrupt */
        __HAL_UART_ENABLE_IT(&stdinout_huart, UART_IT_TXE);
      break;

    default:
      errno = EBADF;
      return -1;
  }
  return len;
}

/***************************************************************
 *
 ***************************************************************/

int
_close(int file)
{
  return -1;
}
/*
 execve
 Transfer control to a new process. Minimal implementation (for a system without
 processes):
 */
int
_execve(char* name, char** argv, char** env)
{
  errno = ENOMEM;
  return -1;
}

/*
 fstat
 Status of an open file. For consistency with other minimal implementations in
 these examples,
 all files are regarded as character special devices.
 The `sys/stat.h' header file required is distributed in the `include'
 subdirectory for this C library.
 */
int
_fstat(int file, struct stat* st)
{
  st->st_mode = S_IFCHR;
  return 0;
}

/*
 isatty
 Query whether output stream is a terminal. For consistency with the other
 minimal implementations,
 */
int
_isatty(int file)
{
  switch (file) {
    case STDOUT_FILENO:
    case STDERR_FILENO:
    case STDIN_FILENO:
      return 1;
    default:
      // errno = ENOTTY;
      errno = EBADF;
      return 0;
  }
}

/*
 link
 Establish a new name for an existing file. Minimal implementation:
 */

int
_link(char* old, char* new)
{
  errno = EMLINK;
  return -1;
}

/*
 lseek
 Set position in a file. Minimal implementation:
 */
int
_lseek(int file, int ptr, int dir)
{
  return 0;
}


/*** EOF ***/

