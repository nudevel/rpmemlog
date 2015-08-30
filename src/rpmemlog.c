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
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>
#include <ctype.h>

#define DBG_MSG(fmt,args...) do{ if(ptr->dbg) ptr->printf("rpmemlog.c:%03d debug " fmt, __LINE__, ##args); }while(0);
#define ERR_MSG(fmt,args...) do{ ptr->printf("rpmemlog.c:%03d " fmt, __LINE__, ##args); }while(0);


struct rpmemlog
{
    int dbg;
    char msg[1024*2];
    char *buf, *wp;
    size_t len;
    pthread_mutex_t mutex;
    int realtime;
    int buf_shall_be_freed;
    void (*printf)(const char *fmt, ...);
    uint32_t overflow;
};

static int _snprintf_timestamp(char *buf, size_t len)
{
    struct timeval tv;
    struct tm tm;

    if( gettimeofday(&tv, NULL) != 0 ){
        return -1;
    }

    if( localtime_r(&tv.tv_sec, &tm) == NULL ){
        return -1;
    }

    return snprintf( buf, len,  "<%02d:%02d:%02d.%03d>", tm.tm_hour, tm.tm_min, tm.tm_sec, (int)(tv.tv_usec / 1000) );
}

int rpmemlog_open(char *buf, size_t len, int realtime, void (*user_printf)(const char *fmt, ...) )
{
    struct rpmemlog *ptr = NULL;
    
    ptr = calloc( 1, sizeof(struct rpmemlog) );
    if( ptr == NULL ){
        goto _err_end_;
    }

    if( buf ){
        ptr->buf = buf;
        memset(buf, 0, len);
    } else {
        ptr->buf_shall_be_freed = 1;
        ptr->buf = calloc(1, len);
        if( ptr->buf == NULL )
          goto _err_end_;
    }
    ptr->len = len;
    ptr->wp = ptr->buf;
    ptr->realtime = realtime;
    if(user_printf)
      ptr->printf = user_printf;
    else
      ptr->printf = (void*)printf;
    pthread_mutex_init(&ptr->mutex, NULL); // always return 0

    return (int)ptr;

_err_end_:
    if( ptr->buf_shall_be_freed  &&  ptr->buf ) free(ptr->buf);
    if( ptr ) free(ptr);
    return -1;
}

int rpmemlog_close(int fd)
{
    struct rpmemlog *ptr = (struct rpmemlog *)fd;

    if( !ptr )
      return -1;

    pthread_mutex_destroy( &ptr->mutex );
    if( ptr->buf_shall_be_freed  &&  ptr->buf ) free(ptr->buf);
    free(ptr);

    return 0;
}

int rpmemlog_printf(int fd, const char *fmt, ... )
{
    struct rpmemlog *ptr = (struct rpmemlog *)fd;
    int ret[2], len = -1;
    va_list ap;

    if( !ptr )
      return -1;

    if( pthread_mutex_lock( &ptr->mutex ) != 0 )
      return -1;

    ret[0] = _snprintf_timestamp( ptr->msg, sizeof(ptr->msg) );
    if( ret[0] < 0 ){
        goto _end_;
    } else if( ret[0] >= sizeof(ptr->msg) ){
        ptr->overflow++;
        ERR_MSG("overflow %d (%d >= %d)\n", ptr->overflow, ret[0], sizeof(ptr->msg));
        ret[0] = sizeof(ptr->msg) - 1;
    }
    
    va_start( ap, fmt );
    ret[1] = vsnprintf( ptr->msg + ret[0], sizeof(ptr->msg) - ret[0], fmt, ap);
	va_end( ap );
    if( ret[1] < 0 ){
        goto _end_;
    } else if( ret[1] >= sizeof(ptr->msg) - ret[0] ){
        ptr->overflow++;
        ERR_MSG("overflow %d (%d >= %d)\n", ptr->overflow, ret[1], sizeof(ptr->msg) - ret[0]);
        ret[1] = ( sizeof(ptr->msg) - ret[0] ) - 1;
    }

    len = ret[0] + ret[1];
    DBG_MSG("%d = %d + %d\n", len, ret[0], ret[1]);
    if( len > ptr->len ){
        ptr->overflow++;
        ERR_MSG("overflow %d (%d > %d)\n", ptr->overflow, len, ptr->len);
        len = ptr->len;
    }

    if( ptr->wp + len > ptr->buf + ptr->len ){
        int tmp = ptr->buf + ptr->len - ptr->wp;
        memcpy( ptr->wp, ptr->msg, tmp );
        memcpy( ptr->buf, ptr->msg + tmp, len - tmp );
        ptr->wp = ptr->buf + len - tmp;
    } else {
        memcpy( ptr->wp, ptr->msg, len );
        ptr->wp += len;
    }

    if( ptr->realtime )
      ptr->printf(ptr->msg);

  _end_:
    if( pthread_mutex_unlock( &ptr->mutex ) != 0 )
      return -1;

    return len;
}

int rpmemlog_realtime(int fd, int on)
{
    struct rpmemlog *ptr = (struct rpmemlog *)fd;

    if( !ptr )
      return -1;
    ptr->realtime = on;

    return 0;
}

int rpmemlog_show_memlog(int fd)
{
    struct rpmemlog *ptr = (struct rpmemlog *)fd;
    char *rp;

    if( !ptr )
      return -1;

    if( pthread_mutex_lock( &ptr->mutex ) != 0 )
      return -1;

    if( ptr->overflow ){
        ERR_MSG("overflow %d\n", ptr->overflow);
    }

    for( rp = ptr->wp; rp < ptr->buf + ptr->len; rp++ ){
        if( *rp != 0 )
          ptr->printf("%c", *rp);
    }
    for( rp = ptr->buf; rp < ptr->wp; rp++ ){
        if( *rp != 0 )
          ptr->printf("%c", *rp);
    }
    //ptr->printf("\n");

    if( pthread_mutex_unlock( &ptr->mutex ) != 0 )
      return -1;
    
    return 0;
}

