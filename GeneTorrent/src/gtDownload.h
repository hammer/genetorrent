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
 * gtDownload.h
 *
 *  Created on: feb 5, 2012
 *      Author: donavan
 */
#ifndef GT_DOWNLOAD_H_
#define GT_DOWNLOAD_H_

#include "gtBase.h"

class gtDownload : public gtBase
{
   public:
      gtDownload (boost::program_options::variables_map &vm);
      void run ();

   protected:

   private:
      vectOfStr _cliArgsDownloadList;
      std::string _downloadSavePath;
      int _maxChildren;
      vectOfStr _torrentListToDownload;

      void runDownloadMode (std::string startupDir);
      void prepareDownloadList ();
      void downloadGtoFilesByURI (vectOfStr &uris);
      void extractURIsFromXML (std::string xmlFileName, vectOfStr &urisToDownload);
      void performTorrentDownload (int64_t totalSizeOfDownload);
      int downloadChild(int childID, int totalChildren, std::string torrentName, FILE *fd);
      int64_t getFreeDiskSpace ();
      void validateAndCollectSizeOfTorrents (uint64_t &totalBytes, int &totalFiles, int &totalGtos);

      void pcfacliDownloadList (boost::program_options::variables_map &vm);
      void pcfacliMaxChildren (boost::program_options::variables_map &vm);
};

#endif
