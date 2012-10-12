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
 * gtServer.h
 *
 *  Created on: feb 5, 2012
 *      Author: donavan
 */

#ifndef GT_SERVER_H_
#define GT_SERVER_H_

#include "gtBase.h"

class gtServer : public gtBase
{

   public:
      gtServer (boost::program_options::variables_map &vm);
      void run ();

   protected:

   private:
      std::string _serverQueuePath;
      std::string _serverDataPath;
      std::string _serverModeCsrSigningUrl;
      bool _serverForceDownload;
      std::list <activeSessionRec *> _activeSessions;
      unsigned int _maxActiveSessions;

      void getFilesInQueueDirectory (vectOfStr &files);
      void checkSessions();
      void runServerMode();
      void processServerModeAlerts();
      void servedGtosMaintenance (time_t timeNow, std::set <std::string> &activeTorrents, bool shutdownFlag = false);
      bool isDownloadModeGetFromGTO (std::string torrentPathAndFileName);
      bool addTorrentToServingList (std::string);
      gtBase::activeSessionRec *findSession ();
      void deleteGTOfromQueue (std::string fileName);
      libtorrent::session *addActiveSession ();
      time_t getExpirationTime (std::string torrentPathAndFileName);

      void pcfacliSecurityAPI (boost::program_options::variables_map &vm);
      void pcfacliQueue (boost::program_options::variables_map &vm);
      void pcfacliServer (boost::program_options::variables_map &vm);
      void pcfacliServerForceDownload (boost::program_options::variables_map &vm);
};

#endif
