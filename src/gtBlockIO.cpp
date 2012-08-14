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

#include "gtBlockIO.h"

struct _GTIO {
   uint64_t offset;
};

struct _GTIO_Error {
};

GTIO *gtio_open_gto (void *gto_buf, size_t gto_size, char *cred, GTIO_Error *err)
{
}

GTIO *gtio_open_uri (char *uri, char *cred, GTIO_Error *err)
{
}

int gtio_close (GTIO *gtio, GTIO_Error *err)
{
}

// RST-DOC: start: read
int64_t gtio_read (GTIO *gtio, void *buf, size_t count, GTIO_Error *err)
{
   int64_t res = gtio_pread (gtio, buf, count, gtio->offset, err);

   if (res >= 0)
      gtio->offset += res;

   return res;
}
// RST-DOC: end: read

// RST-DOC: start: pread
int64_t gtio_pread (GTIO *gtio, void *buf, size_t count, int64_t offset,
                    GTIO_Error *err)
{
   int64_t rd_cnt = 0;

   // get piece and copy data from piece to buf.
   // Gotchas:
   //   * offset might not be on piece boundary
   //   * will need to get more pieces if count > piece size

   return rd_cnt;
}
// RST-DOC: end: pread

// RST-DOC: start: lseek
int gtio_lseek (GTIO *gtio, off64_t offset, int whence, GTIO_Error *err)
{
   // Error conditions:
   //   * Final offset is negative.
   //   * Final offset is beyond end of file.
   if (whence == SEEK_SET)
   {
      gtio->offset = offset;
      // check for error conditions
   }
   else if (whence == SEEK_CUR)
   {
      gtio->offset += offset;
      // check for error conditions
   }
   else if (whence == SEEK_END)
   {
      // Set error, can't seek beyond end of file.
      return -1;
   }
   else
   {
      // Bad whence: set error code and message in *err
      return -1;
   }

   return 0;
}
// RST-DOC: end: lseek

#ifdef USE_GET_FILE_LIST

struct _GTIO_FileInfo {
};

int gtio_get_file_list(GTIO *gtio, GTIO_FileInfo ** fi, GTIO_Error *err)
{
}

#endif  // USE_GET_FILE_LIST

#ifdef USE_READDIR

struct _GTIO_Dir {
};

struct _GTIO_Dirent {
};

GTIO_Dir *gtio_opendir(GTIO *gtio, GTIO_Error *err)
{
}

int gtio_readdir (GTIO *gtio, GTIO_Dir *dirp, GTIO_Dirent *ent,
                  GTIO_Error *err)
{
}

int gtio_closedir(GTIO *gtio, GTIO_Dir *dirp)
{
}

#endif  // USE_READDIR
