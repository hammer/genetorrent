/*                                           -*- mode: c++; tab-width: 2; -*-
 * $Id$
 *
 * Copyright (c) 2011-2012, Annai Systems, Inc.
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

/*
 * gtUtils.cpp
 *
 *  Created on: Feb 15, 2012
 *      Author: donavan
 */

#include <sys/statvfs.h>
#include <sys/types.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <cstdio>

#include "gtUtils.h"

// 
int statDirectory (std::string dirFile)
{
   time_t dummyArg;
   return statFileOrDirectory (dirFile, DIR_TYPE, dummyArg);
}

// 
int statFile (std::string dirFile)
{
   time_t dummyArg;
   return statFileOrDirectory (dirFile, FILE_TYPE, dummyArg);
}

// 
int statFile (std::string dirFile, time_t &timeStamp)
{
   return statFileOrDirectory (dirFile, FILE_TYPE, timeStamp);
}

// Do not use this function directly
int statFileOrDirectory (std::string dirFile, statType sType, time_t &fileMtime)
{
   struct stat status;

   int statVal = stat (dirFile.c_str (), &status);

   if (statVal == 0 && S_ISDIR (status.st_mode))
   {
      if (sType != DIR_TYPE)  // Trying to stat a file and have a directory
      {
         return -1;
      }

      DIR *dir;

      dir = opendir (dirFile.c_str());
  
      if (dir != NULL)
      {
         closedir (dir);
         return 0;
      }
      else 
      {
         return -1;
      }
   }

   if (statVal == 0 && S_ISREG (status.st_mode))
   {
      if (sType != FILE_TYPE)  // Trying to stat a directory and have a file
      {
         return -1;
      }

      FILE *file;
   
      file = fopen (dirFile.c_str(), "r");
  
      if (file != NULL)
      {
         fileMtime = status.st_mtime;
         fclose (file);
         return 0;
      }
      else 
      {
         return -1;
      }
   }

   return -1;
}
