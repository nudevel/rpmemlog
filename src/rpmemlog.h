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

#ifndef _RPMEMLOG_H_
#define _RPMEMLOG_H_

/*!
  \mainpage
  RPMEMLOG (Real-time Printable Memory Log) is a lightweight logging library which
  stores logs into ring buffer while printing them into stdout in real-time.
  This software is distributed under the BSD (Berkeley Standard Distribution) license.
*/

#include <stdio.h>

/*!
  \brief
  "rpmemlog_open()" creates a descriptor by using given arguments.
  
  \param[in] buf
  A ring buffer address. If NULL is specifiled, it will be allocated internally.
  
  \param[in] len
  The length of the ring buffer.

  \param[in] realtime
  If realtime is 0, rpmemlog_printf() only stores a message into ring buffer which can
  be printed by calling rpmemlog_show_memlog(). If not 0 , rpmemlog_printf() also calls
  user_printf() to print the message in real-time.

  \param[in] user_printf
  A function pointer of your print function. If NULL, printf() in stdio.h is used internally.
  
  \return
  It returns a descriptor. On error, -1 is returned.
*/
int rpmemlog_open(char *buf, size_t len, int realtime, void (*user_printf)(const char *fmt, ...) );

/*!
  \brief
  "rpmemlog_close()" closes the descriptor.
  
  \param[in] fd
  The descriptor.
  
  \return
  It returns zero on success. On error, -1 is returned.
*/
int rpmemlog_close(int fd);

/*!
  \brief
  "rpmemlog_printf()" prints a message into the ring buffer. If realtime flag is asserted,
  this message is printed by calling user_printf().
  
  \param[in] fd
  The descriptor.
  
  \param[in] fmt
  A format string like the standard printf function in stdio.h.

  \return
  It returns the number of characters printed (excluding the null byte used to end output to
  strings). On error, -1 is returned.
*/
int rpmemlog_printf(int fd, const char *fmt, ... );

/*!
  \brief
  "rpmemlog_realtime()" changes the realtime flag.
  
  \param[in] fd
  The descriptor.
  
  \param[in] on
  The real-time flag. See rpmemlog_open().

  \return
  It returns zero on success. On error, -1 is returned.
*/
int rpmemlog_realtime(int fd, int on);

/*!
  \brief
  "rpmemlog_show_memlog()" prints the ring buffer.
  
  \param[in] fd
  The descriptor.
  
  \return
  It returns zero on success. On error, -1 is returned.
*/
int rpmemlog_show_memlog(int fd);

#endif
