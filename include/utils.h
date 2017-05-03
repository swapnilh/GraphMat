/******************************************************************************
** Copyright (c) 2015, Intel Corporation                                     **
** All rights reserved.                                                      **
**                                                                           **
** Redistribution and use in source and binary forms, with or without        **
** modification, are permitted provided that the following conditions        **
** are met:                                                                  **
** 1. Redistributions of source code must retain the above copyright         **
**    notice, this list of conditions and the following disclaimer.          **
** 2. Redistributions in binary form must reproduce the above copyright      **
**    notice, this list of conditions and the following disclaimer in the    **
**    documentation and/or other materials provided with the distribution.   **
** 3. Neither the name of the copyright holder nor the names of its          **
**    contributors may be used to endorse or promote products derived        **
**    from this software without specific prior written permission.          **
**                                                                           **
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       **
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         **
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR     **
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      **
** HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    **
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  **
** TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR    **
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* ******************************************************************************/
/* Narayanan Sundaram (Intel Corp.)
 * ******************************************************************************/

#ifndef __UTILS_H
#define __UTILS_H


#include "immintrin.h"
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

//SSE 
//#define CPU_FREQ (2.7e9)

#define SIMD_WIDTH (4)
#define SIMDINTTYPE         __m128i
#define SIMDMASKTYPE         __m128i
#define _MM_LOADU(Addr)     _mm_loadu_si128((__m128i *)(Addr))
#define _MM_CMP_EQ(A,B)     _mm_cmpeq_epi32(A,B)
#define _MM_SET1(val)           _mm_set1_epi32(val)

void start_pin_tracing()
{
    asm volatile(".byte 0xbb,0x11,0x22,0x33,0x44,0x64,0x67,0x90" : : : "ebx");
}
void stop_pin_tracing()
{
    asm volatile(".byte 0xbb,0x11,0x22,0x33,0x55,0x64,0x67,0x90" : : : "ebx");
}


struct perf_event_attr pe;
int fd[4];
uint64_t id[4];

struct read_format {
    uint64_t nr;
    struct {
        uint64_t value;
        uint64_t id;
    } values[4];
};

long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
        int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
            group_fd, flags);
    return ret;
}

uint64_t create_config(uint8_t event, uint8_t umask, uint8_t cmask) {
    return uint64_t ( (cmask << 24) | (umask << 8) | (event) );
}

void start_perf_tracing()
{
  printf("Starting Perf Tracing!\n");
  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = PERF_TYPE_RAW;
  pe.size = sizeof(struct perf_event_attr);
// All unhalted core execution cycles
  pe.config = create_config(0x3C, 0x00, 0);
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;
  pe.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;

  fd[0] = perf_event_open(&pe, 0, -1, -1, 0);
  if (fd[0] == -1) {
          fprintf(stderr, "Error opening leader %llx\n", pe.config);
          exit(EXIT_FAILURE);
  }
  ioctl(fd[0], PERF_EVENT_IOC_ID, &id[0]);

  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = PERF_TYPE_RAW;
  pe.size = sizeof(struct perf_event_attr);
// Load misses causes a page walk
  pe.config = create_config(0x08, 0x01, 0);
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;
  pe.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
  fd[1] = syscall(__NR_perf_event_open, &pe, 0, -1, fd[0], 0); // <-- here
  ioctl(fd[1], PERF_EVENT_IOC_ID, &id[1]);

  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = PERF_TYPE_RAW;
  pe.size = sizeof(struct perf_event_attr);
// Cycles in Page Walk
  pe.config = create_config(0x08, 0x10, 1);
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;
  pe.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
  fd[2] = syscall(__NR_perf_event_open, &pe, 0, -1, fd[0], 0); // <-- here
  ioctl(fd[2], PERF_EVENT_IOC_ID, &id[2]);

  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = PERF_TYPE_RAW;
  pe.size = sizeof(struct perf_event_attr);
// Total Instructions Retired
  pe.config = create_config(0xC0, 0x00, 0);
  pe.disabled = 1;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;
  pe.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
  fd[3] = syscall(__NR_perf_event_open, &pe, 0, -1, fd[0], 0); // <-- here
  ioctl(fd[3], PERF_EVENT_IOC_ID, &id[3]);

  ioctl(fd[0], PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
  ioctl(fd[0], PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
}

void stop_perf_tracing()
{
  ioctl(fd[0], PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
  
  char buf[4096];
  int i;
  struct read_format* rf = (struct read_format*) buf;
  uint64_t val[4];
  int ret = read(fd[0], buf, sizeof(buf));
  for (i = 0; i < rf->nr; i++) {
          if (rf->values[i].id == id[0]) {
                  val[0] = rf->values[i].value;
          } else if (rf->values[i].id == id[1]) {
                  val[1] = rf->values[i].value;
          } else if (rf->values[i].id == id[2]) {
                  val[2] = rf->values[i].value;
          } else if (rf->values[i].id == id[3]) {
                  val[3] = rf->values[i].value;
          }
  }
  close(fd[0]);
  close(fd[1]);
  close(fd[2]);
  close(fd[3]);
  printf("Total CPU Cycles | Total Instructions Retired | Total PageWalk Cycles | Total DTLB misses\n", val[0]);
  printf("%llu %llu %llu %llu \n", val[0], val[3], val[2], val[1]);  
}

#endif
