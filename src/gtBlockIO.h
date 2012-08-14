/*
 * Copyright (c) 2012, Annai Systems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE
 *
 * Created under contract by Cardinal Peak, LLC.   www.cardinalpeak.com
 */

#ifndef GT_BLOCK_IO_H
#define GT_BLOCK_IO_H

#ifdef __cplusplus

// Place C++ Class declarations here.

#endif // End of C++ Class declarations.

#ifdef __cplusplus
extern "C" {
#endif

// DOC: start: Functional API

typedef struct _GTIO GTIO;
typedef struct _GTIO_Error GTIO_Error;

// Need this if implementing get_file_list.
typedef struct _GTIO_FileInfo GTIO_FileInfo;

// These only needed if we decide to implement opendir, readdir, closedir.
typedef struct _GTIO_Dir GTIO_Dir;
typedef struct _GTIO_Dirent GTIO_Dirent;

extern GTIO *gtio_open_gto (void *gto_buf, size_t gto_size, char *cred,
                            GTIO_Error *err);

extern GTIO *gtio_open_uri (char *uri, char *cred, GTIO_Error *err);

extern int gtio_close (GTIO *gtio, GTIO_Error *err);

extern int64_t gtio_read (GTIO *gtio, void *buf, size_t count, GTIO_Error *err);

extern int64_t gtio_pread (GTIO *gtio, void *buf, size_t count, int64_t offset,
                           GTIO_Error *err);

extern int gtio_lseek (GTIO *gtio, int64_t offset, int whence, GTIO_Error *err);

// Functions for getting a list of files in the torrent and their
// offsets and sizes.

// Need to choose between this:
extern int gtio_get_file_list(GTIO *gtio, GTIO_FileInfo ** fi, GTIO_Error *err);
// or this:
extern GTIO_Dir *gtio_opendir(GTIO *gtio, GTIO_Error *err);
extern int gtio_readdir (GTIO *gtio, GTIO_Dir *dirp, GTIO_Dirent *ent,
                         GTIO_Error *err);
extern int gtio_closedir(GTIO *gtio, GTIO_Dir *dirp);

// DOC: end: Functional API

#ifdef __cplusplus
}
#endif

#endif /* GT_BLOCK_IO_H */
