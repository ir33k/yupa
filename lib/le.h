// LogErr.H (LEH) the logger v2.0
// 
// The LOG, WARN and ERR macros prints current file name, line number,
// function name, log prefix and FMT formatted message (printf(3))
// with optional varying number of arguments for FMT.  When FMT ends
// with ':' then result of perror(0) is appended.  When ERR macro is
// used then program exits with 1.
// 
// 	$ cat -n demo.c
// 	     2	#include "le.h"
// 	     3	int main(void)
// 	     4	{
// 	     5		LOG("Print args %d %s", 1, "test");
// 	     6		WARN("fopen:"); // Append perror(0)
// 	     7		ERR("Print error and exit with 1");
// 	     8		return 0;
// 	     9	}
// 
// 	$ cc demo.c && ./a.out
// 	demo.c:5	main()	>>>> Print args 1 test
// 	demo.c:6	main()	WARN fopen: No such file or directory
// 	demo.c:7	main()	ERR Print error and exit with 1
// 
// ISC-License
// 
// Copyright 2023 Irek <irek@gabr.pl>
// 
// Permission to use, copy, modify, and/or distribute this software
// for any purpose with or without fee is hereby granted, provided
// that the above copyright notice and this permission notice appear
// in all copies.
// 
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
// WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
// AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
// CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
// OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
// NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
// CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifndef _LOGERR_H
#define _LOGERR_H

#define LOG(...)  _LEH_LOG(">>>> " __VA_ARGS__, 0)
#define WARN(...) _LEH_LOG("WARN " __VA_ARGS__, 0)
#define ERR(...)  _LEH_LOG("ERR "  __VA_ARGS__, 1)
#define _LEH_LOG(fmt, ...) _leh_log("%s:%d:\t%s()\t" fmt, \
	__FILE__, __LINE__, __func__, __VA_ARGS__)

void _leh_log(const char *fmt, ...);

#endif // _LOGERR_H
