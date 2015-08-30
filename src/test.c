/*=============================================================================

Copyright (c) 2015, Naoto Uegaki
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

  Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/

#include "rpmemlog.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOG_MSG(fmt,args...) do{ printf("[LOG][test.c:%03d] " fmt, __LINE__, ##args); }while(0);
#define ERR_MSG(fmt,args...) do{ printf("[ERR][test.c:%03d] " fmt, __LINE__, ##args); }while(0);

int test_basic(void)
{
    int fd;
    char buf[1024*8];

    fd = rpmemlog_open(buf, sizeof(buf), 1, (void*)printf );
    if( fd == -1 ){
        ERR_MSG("rpmemlog_open\n");
        return -1;
    }

    {
        char long_msg[128*1024];
        printf("-- long msg test --\n");
        memset(long_msg, 'a', sizeof(long_msg));
        long_msg[ sizeof(long_msg) - 1 ] = '\0';
        if( rpmemlog_printf(fd, long_msg) == -1 ){
            ERR_MSG("rpmemlog_printf");
            return -1;
        }
        printf("\n");
    }

    {
        printf("-- ja msg test --\n");
        if( rpmemlog_printf(fd, "¥Æ¥¹¥È\n") == -1 ){
            ERR_MSG("rpmemlog_printf");
            return -1;
        }
    }
    
    printf("-- rpmemlog_show_memlog --\n");
    if( rpmemlog_show_memlog(fd) != 0 ){
        ERR_MSG("rpmemlog_show_memlog\n");
        return -1;
    }
    
    if( rpmemlog_close(fd) != 0 ){
        ERR_MSG("rpmemlog_close\n");
        return -1;
    }

    {
        printf("-- short buf test --\n");
        fd = rpmemlog_open(buf, 8, 1, (void*)printf );
        if( fd == -1 ){
            ERR_MSG("rpmemlog_open\n");
            return -1;
        }
        *(int*)fd = 1;
        
        if( rpmemlog_printf(fd, "0123456789ABCDEF\n") == -1 ){
            ERR_MSG("rpmemlog_printf");
            return -1;
        }
        
        printf("-- rpmemlog_show_memlog --\n");
        if( rpmemlog_show_memlog(fd) != 0 ){
            ERR_MSG("rpmemlog_show_memlog\n");
            return -1;
        }
        
        if( rpmemlog_close(fd) != 0 ){
            ERR_MSG("rpmemlog_close\n");
            return -1;
        }
    }

    {
        printf("-- disable realtime test --\n");
        char msg[512] = { 0 };
        
        fd = rpmemlog_open(NULL, 1024, 0, NULL);
        if( fd == -1 ){
            ERR_MSG("rpmemlog_open\n");
            return -1;
        }

        memset(msg, 'a', sizeof(msg) - 1);
        if( rpmemlog_printf(fd, msg) == -1 ){
            ERR_MSG("rpmemlog_printf");
            return -1;
        }
        memset(msg, 'b', sizeof(msg) - 1);
        if( rpmemlog_printf(fd, msg) == -1 ){
            ERR_MSG("rpmemlog_printf");
            return -1;
        }
        
        printf("-- rpmemlog_show_memlog --\n");
        if( rpmemlog_show_memlog(fd) != 0 ){
            ERR_MSG("rpmemlog_show_memlog\n");
            return -1;
        }
        
        if( rpmemlog_close(fd) != 0 ){
            ERR_MSG("rpmemlog_close\n");
            return -1;
        }
    }

    return 0;
}

void *test_thread(void *arg)
{
    int fd = ((int*)arg)[0];
    int id = ((int*)arg)[1];
    int i;

    for(i=0; i<60*1000; ++i){
        if( i % 10000 == 0 ) sleep(1);
        if( rpmemlog_printf(fd, "[%d] %010d\n", id, i) == -1 ){
            ERR_MSG("rpmemlog_printf\n");
            exit(-1);
        }
    }

    return NULL;
}

int test_multi_thread(void)
{
    int fd;
    char buf[1024*8];
    pthread_t th[4];
    int i, arg[4][2];

    fd = rpmemlog_open(buf, sizeof(buf), 1, (void*)printf );
    if( fd == -1 ){
        ERR_MSG("rpmemlog_open\n");
        return -1;
    }

    for(i=0; i<(sizeof(th)/sizeof(pthread_t)); ++i){
        arg[i][0] = fd;
        arg[i][1] = i;
        if( pthread_create(&th[i], NULL, test_thread, (void*)arg[i] ) != 0 ){
            ERR_MSG("pthread_create\n");
            return -1;
        }
    }

    for(i=0; i<(sizeof(th)/sizeof(pthread_t)); ++i){
        if( pthread_join(th[i], NULL) != 0 ){
            ERR_MSG("pthread_join\n");
            return -1;
        }
    }

    printf("------------------------------------------------------------\n");
    if( rpmemlog_show_memlog(fd) != 0 ){
        ERR_MSG("rpmemlog_show_memlog\n");
        return -1;
    }
    
    if( rpmemlog_close(fd) == -1 ){
        ERR_MSG("rpmemlog_close\n");
        return -1;
    }

    return 0;
}

int main()
{
    if( test_basic() != 0 ){
        ERR_MSG("test_long_msg\n");
        return -1;
    }

#if 1
    if( test_multi_thread() != 0 ){
        ERR_MSG("test_multi_thread\n");
        return -1;
    }
#endif
    
    return 0;
}
