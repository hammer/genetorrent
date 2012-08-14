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
 * gtUplaod.h
 *
 *  Created on: feb 5, 2012
 *      Author: donavan
 */

#ifndef GT_UPLOAD_H_
#define GT_UPLOAD_H_

#include "gtBase.h"

class gtUpload : public gtBase
{
   public:
      gtUpload (boost::program_options::variables_map &vm);
      void run ();

      static bool file_filter (boost::filesystem::path const& filename);
      bool fileFilter (std::string const filename);

      void hashCallbackImpl (int piece);
   protected:

   private:
      std::string _manifestFile;
      std::string _uploadUUID;
      std::string _uploadSubmissionURL;
      vectOfStr  _filesToUpload;
      int _pieceSize;
      std::string _dataFilePath;
      std::string _uploadGTODir;  //  This directory is used to store the upload GTO and upload progress state when set (otherwise the uuid directory is used)
      int _piecesInTorrent;       // Used by the hash callback function to display progress

      void submitTorrentToGTExecutive (std::string torrentFileName, bool);
      void findDataAndSetWorkingDirectory ();
      bool verifyDataFilesExist (vectOfStr &);
      unsigned long setPieceSize (unsigned &);
      void displayMissingFilesAndExit (vectOfStr &missingFiles);
      void makeTorrent(std::string);
      void processManifestFile();
      void performGtoUpload (std::string torrentFileName, long, bool);
      void performTorrentUpload();
      void configureUploadGTOdir (std::string uuid);
      long evaluateUploadResume (time_t, std::string);

      void pcfacliUpload (boost::program_options::variables_map &vm);
      void pcfacliUploadGTODir (boost::program_options::variables_map &vm);
      static void hashCallback (int piece);
};

#endif
