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
 * gtDownload.cpp
 *
 *  Created on: Jan 27, 2012
 *      Author: donavan
 */

#define _DARWIN_C_SOURCE 1

#include <config.h>

#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdio>

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/peer_info.hpp"
#include "libtorrent/ip_filter.hpp"
#include "libtorrent/alert_types.hpp"

#include <xqilla/xqilla-simple.hpp>

#include <curl/curl.h>

#include "gtBase.h"
#include "stringTokenizer.h"
#include "gtDefs.h"
#include "geneTorrentUtils.h"
#include "gtLog.h"
#include "loggingmask.h"
#include "gtDownload.h"

static char const* download_state_str[] = {
   "checking (q)",            // queued_for_checking,
   "checking",                // checking_files,
   "dl metadata",             // downloading_metadata,
   "downloading",             // downloading,
   "finished",                // finished,
   "cleanup",                 // seeding,
   "allocating",              // allocating,
   "checking (r)"             // checking_resume_data
};

extern void *geneTorrCallBackPtr; 

gtDownload::gtDownload (boost::program_options::variables_map &vm) : gtBase (vm, DOWNLOAD_MODE), _cliArgsDownloadList (), _downloadSavePath (""), _maxChildren (8), _torrentListToDownload ()
{
   pcfacliMaxChildren (vm);
   _downloadSavePath = pcfacliPath(vm);
   pcfacliDownloadList (vm);

   Log (PRIORITY_NORMAL, "%s (using tmpDir = %s)", startUpMessage.str().c_str(), _tmpDir.c_str());

   _startUpComplete = true;

   if (_verbosityLevel > VERBOSE_1)
   {
      screenOutput ("Welcome to GeneTorrent version " << VERSION << ", download mode."); 
   }
}

void gtDownload::pcfacliMaxChildren (boost::program_options::variables_map &vm)
{
   if (vm.count (MAX_CHILDREN_CLI_OPT) == 1 && vm.count (MAX_CHILDREN_CLI_OPT_LEGACY) == 1)
   {
      commandLineError ("duplicate config options:  " + MAX_CHILDREN_CLI_OPT + " and " + MAX_CHILDREN_CLI_OPT_LEGACY + " are not permitted at the same time");
   }

   if (vm.count (MAX_CHILDREN_CLI_OPT) == 1)
   {
      _maxChildren = vm[MAX_CHILDREN_CLI_OPT].as< int >();
   }
   else if (vm.count (MAX_CHILDREN_CLI_OPT_LEGACY) == 1)
   {
      _maxChildren = vm[MAX_CHILDREN_CLI_OPT_LEGACY].as< int >();
   }
   else   // Option not present
   {
      return;
   }

   if (_maxChildren < 1)
   {
      commandLineError ("--" + MAX_CHILDREN_CLI_OPT + " must be greater than 0");
   }

   startUpMessage << " --" << MAX_CHILDREN_CLI_OPT << "=" << _maxChildren;
}

void gtDownload::pcfacliDownloadList (boost::program_options::variables_map &vm)
{
   _cliArgsDownloadList = vm[DOWNLOAD_CLI_OPT].as< std::vector <std::string> >();

   vectOfStr::iterator vectIter = _cliArgsDownloadList.begin ();

   bool needCreds = false;

   while (vectIter != _cliArgsDownloadList.end () && needCreds == false) // Check the list of -d arguments to see if any require a credential file
   {
      std::string inspect = *vectIter;

      if (inspect.size () > 4) // Check for GTO file
      {
         if (inspect.substr (inspect.size () - 4) == GTO_FILE_EXTENSION)
         {
            vectIter++;
            continue;
         }
      }

      needCreds = true;
      vectIter++;
   }

   if (needCreds)
   {
      checkCredentials ();
   }

   vectIter = _cliArgsDownloadList.begin ();
   while (vectIter != _cliArgsDownloadList.end ())
   {
      startUpMessage << " --" << DOWNLOAD_CLI_OPT << "=" << *vectIter;
      vectIter++;
   }
}

void gtDownload::run ()
{
   time_t startTime = time(NULL);
   std::string saveDir = getWorkingDirectory ();

   prepareDownloadList ();

         if (_downloadSavePath.size())
         {
            int cdRet = chdir (_downloadSavePath.c_str());

            if (cdRet != 0 )
            {
               gtError ("Failure changing directory to " + _downloadSavePath, 202, ERRNO_ERROR, errno);
            }
         }

         uint64_t totalBytes; 
         int totalFiles;
         int totalGtos;

         validateAndCollectSizeOfTorrents (totalBytes, totalFiles, totalGtos);

         std::ostringstream message;
         message << "Ready to download " << totalGtos << " GTO(s) with " << totalFiles << " file(s) comprised of " << add_suffix (totalBytes) << " of data"; 

         Log (PRIORITY_NORMAL, "%s", message.str().c_str());

         if (_verbosityLevel > VERBOSE_1)
         {
            screenOutput (message.str()); 
         }

         performTorrentDownload (totalBytes);

         message.str("");
  
         time_t duration = time(NULL) - startTime;

         message << "Downloaded " << add_suffix (totalBytes) << " in " << durationToStr(duration) << ".  Overall Rate " << add_suffix (totalBytes/duration) << "/s";

         Log (PRIORITY_NORMAL, "%s", message.str().c_str());

         if (_verbosityLevel > VERBOSE_1)
         {
            screenOutput (message.str()); 
         }

   chdir (saveDir.c_str ());       // shutting down, if the chdir back fails, so be it
}

void gtDownload::prepareDownloadList ()
{
   vectOfStr urisToDownload; // This list of URIs is used to download .gto files.

   vectOfStr::iterator vectIter = _cliArgsDownloadList.begin ();

   // iterate over the list of -d cli arguments.
   while (vectIter != _cliArgsDownloadList.end ())
   {
      std::string inspect = *vectIter;

      if (inspect.size () > 4)
      {
         std::string tail = inspect.substr (inspect.size () - 4);

         if (tail == GTO_FILE_EXTENSION) // have an existing .gto
         {
            relativizePath(inspect);
            _torrentListToDownload.push_back (inspect);
         }
         else if (tail == ".xml" || tail == ".XML") // Extract a list of URIs from passed XML
         {
            relativizePath(inspect);
            extractURIsFromXML (inspect, urisToDownload);
         }
         else if (std::string::npos != inspect.find ("/")) // Have a URI
         {
            urisToDownload.push_back (inspect);
         }
         else // we have a UUID
         {
            std::string url = CGHUB_WSI_BASE_URL + "download/" + inspect;
            urisToDownload.push_back (url);
         }
      }
      else
      {
         gtError ("-d download argument unrecognized.  '" + inspect + "' is too short'", 201);
      }

      vectIter++;
   }

   downloadGtoFilesByURI (urisToDownload);
}

void gtDownload::downloadGtoFilesByURI (vectOfStr &uris)
{
   vectOfStr::iterator vectIter = uris.begin ();

   while (vectIter != uris.end ())
   {
      CURL *curl;
      curl = curl_easy_init ();

      if (!curl)
      {
         gtError ("libCurl initialization failure", 201);
      }

      std::string uri = *vectIter;

      std::string fileName = uri.substr (uri.find_last_of ('/') + 1);
      fileName = fileName.substr (0, fileName.find_first_of ('?'));
      std::string torrUUID = fileName;
      fileName += GTO_FILE_EXTENSION;

      FILE *gtoFile;

      gtoFile = fopen (fileName.c_str (), "wb");

      if (gtoFile == NULL)
      {
         gtError ("Failure opening " + fileName + " for output.", 202, ERRNO_ERROR, errno);
      }

      char errorBuffer[CURL_ERROR_SIZE + 1];
      errorBuffer[0] = '\0';

      std::string curlResponseHeaders = "";

      curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 0);
      curl_easy_setopt (curl, CURLOPT_SSL_VERIFYHOST, 0);

      curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, errorBuffer);
      curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, NULL);
      curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION, &curlCallBackHeadersWriter);
      curl_easy_setopt (curl, CURLOPT_MAXREDIRS, 15);
      curl_easy_setopt (curl, CURLOPT_WRITEDATA, gtoFile);
      curl_easy_setopt (curl, CURLOPT_WRITEHEADER, &curlResponseHeaders);
      curl_easy_setopt (curl, CURLOPT_NOSIGNAL, (long)1);

      curl_easy_setopt (curl, CURLOPT_POST, (long)1);

      struct curl_httppost *post=NULL;
      struct curl_httppost *last=NULL;

      curl_formadd (&post, &last, CURLFORM_COPYNAME, "token", CURLFORM_COPYCONTENTS, _authToken.c_str(), CURLFORM_END);

      curl_easy_setopt (curl, CURLOPT_HTTPPOST, post);

      int timeoutVal = 15;
      int connTime = 4;

      curl_easy_setopt (curl, CURLOPT_URL, uri.c_str());
      curl_easy_setopt (curl, CURLOPT_TIMEOUT, timeoutVal);
      curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, connTime);

      CURLcode res;

      res = curl_easy_perform (curl);

      if (res != CURLE_OK)
      {
         curlCleanupOnFailure (fileName, gtoFile);
         gtError ("Problem communicating with GeneTorrent Executive while trying to retrieve GTO for UUID:  " + torrUUID, 203, gtBase::CURL_ERROR, res, "URL:  " + uri);
      }

      long code;
      res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

      if (res != CURLE_OK)
      {
         curlCleanupOnFailure (fileName, gtoFile);
         gtError ("Problem communicating with GeneTorrent Executive while trying to retrieve GTO for UUID:  " + torrUUID, 204, gtBase::DEFAULT_ERROR, 0, "URL:  " + uri);
      }

      if (code != 200)
      {
         curlCleanupOnFailure (fileName, gtoFile);
         gtError ("Problem communicating with GeneTorrent Executive while trying to retrieve GTO for UUID:  " + torrUUID, 205, gtBase::HTTP_ERROR, code, "URL:  " + uri);
      }

      if (_verbosityLevel > VERBOSE_2)
      {
         screenOutput ("Headers received from the client:  '" << curlResponseHeaders << "'" << std::endl);
      }

      curl_easy_cleanup (curl);

      fclose (gtoFile);

      std::string torrFile = getWorkingDirectory () + '/' + fileName;

      _torrentListToDownload.push_back (torrFile);

      libtorrent::error_code torrentError;
      libtorrent::torrent_info torrentInfo (torrFile, torrentError);

      if (torrentError)
      {
         gtError (".gto processing problem with " + torrFile, 87, TORRENT_ERROR, torrentError.value (), "", torrentError.message ());
      }

      if (torrentInfo.ssl_cert().size() > 0 && _devMode == false)
      {
         std::string pathToKeep = "/cghub/data/";
  
         std::size_t foundPos;
 
         if (std::string::npos == (foundPos = uri.find (pathToKeep)))
         {
            gtError ("Unable to find " + pathToKeep + " in the URL:  " + uri, 214, gtBase::DEFAULT_ERROR);
         }

         std::string certSignURL = uri.substr(0, foundPos + pathToKeep.size()) + GT_CERT_SIGN_TAIL;

         generateSSLcertAndGetSigned(torrFile, certSignURL, torrUUID);
      }
      vectIter++;
   }
}

void gtDownload::extractURIsFromXML (std::string xmlFileName, vectOfStr &urisToDownload)
{
   XQilla xqilla;
   AutoDelete <XQQuery> query (xqilla.parse (X("//ResultSet/Result/analysis_data_uri/text()")));
   AutoDelete <DynamicContext> context (query->createDynamicContext ());
   Sequence seq = context->resolveDocument (X(xmlFileName.c_str()));

   if (!seq.isEmpty () && seq.first ()->isNode ())
   {
      context->setContextItem (seq.first ());
      context->setContextPosition (1);
      context->setContextSize (1);
   }
   else
   {
       // TODO: replace with better error message
       std::cerr << "Empty set, invalid xml" << std::endl; 
       exit (70);
   }

   Result result = query->execute (context);
   Item::Ptr item;

   // add to the list of URIs from the XML file
   item = result->next (context);

   while (item != NULL)
   {
      urisToDownload.push_back (UTF8(item->asString(context)));
      item = result->next (context); // Get the next node in the list from the xquery
   }
}

void gtDownload::performTorrentDownload (int64_t totalSizeOfDownload)
{
   int64_t freeSpace = getFreeDiskSpace();

   if (totalSizeOfDownload > freeSpace) 
   {
      gtError ("The system does not have enough free disk space to complete this transfer (transfer total size is " + add_suffix (totalSizeOfDownload) + "); free space is " + add_suffix (freeSpace), 97, gtBase::DEFAULT_ERROR, 0);
   }

   vectOfStr::iterator vectIter = _torrentListToDownload.begin ();

    // TODO: It would be good to use a system call to determine how
    // many cores this machine has.  There shouldn't be more children
    // than cores, so set 
    //    maxChildren = min(_maxChildren, number_of_cores)
    // Hard to do this in a portable manner however

   int maxChildren = _maxChildren;
   int pipes[maxChildren+1][2];

   int64_t totalDataDownloaded = 0;

   while (vectIter != _torrentListToDownload.end ())
   {
      libtorrent::error_code torrentError;
      libtorrent::torrent_info torrentInfo (*vectIter, torrentError);

      if (torrentError)
      {
         gtError (".gto processing problem", 217, TORRENT_ERROR, torrentError.value (), "", torrentError.message ());
      }

      int childrenThisGTO = torrentInfo.num_pieces() >= maxChildren ? maxChildren : torrentInfo.num_pieces();
      int childID=1;
  
      std::map <pid_t, childRec *> pidList;
      pid_t pid;

      while (childID <= childrenThisGTO)      // Spawn Children that will download this GTO
      {
         if (pipe (pipes[childID]) < 0)
         {
            gtError ("pipe() error", 107, ERRNO_ERROR, errno);
         }
         
         pid =fork();

         if (pid < 0)
         {
            gtError ("fork() error", 107, ERRNO_ERROR, errno);
         }
         else if (pid == 0)
         {
            close (pipes[childID][0]);

            FILE *foo =  fdopen (pipes[childID][1], "w");

            downloadChild (childID, childrenThisGTO, *vectIter, foo);
         }
         else
         {
            close (pipes[childID][1]);
            childRec *cRec = new childRec;
            cRec->childID = childID;
            cRec->dataDownloaded = 0;
            cRec->pipeHandle = fdopen (pipes[childID][0], "r");
            pidList[pid] = cRec;
         }

         childID++;
      }

      int64_t xfer = 0;
      int64_t childXfer = 0;
      int64_t dlRate = 0;

      while (pidList.size() > 0)
      {
         xfer = 0;
         dlRate = 0;
         childXfer = 0;

         std::map<pid_t, childRec *>::iterator pidListIter = pidList.begin();

         while (pidListIter != pidList.end())
         { 
            int retValue;
            char inBuff[40];
            
            if (NULL != fgets (inBuff, 40, pidListIter->second->pipeHandle))
            {
               childXfer = strtoll (inBuff, NULL, 10);
               pidListIter->second->dataDownloaded  = childXfer;
            }
            else
            {
               childXfer = pidListIter->second->dataDownloaded;  // return data for this poll
            }

            if (NULL != fgets (inBuff, 40, pidListIter->second->pipeHandle))
            {
               dlRate += strtol (inBuff, NULL, 10);
            }
            else
            {
               usleep (250000);   // If null read, fd is closed in child, but waitpid will not detect the dead child.  pause for 1/4 second.
            }

            pid_t pidStat = waitpid (pidListIter->first, &retValue, WNOHANG);

            if (pidStat == pidListIter->first)
            {
               if (WIFEXITED(retValue) && WEXITSTATUS(retValue) != 0) 
               {
                  char buffer[256];
                  snprintf(buffer, sizeof(buffer), "Child %d exited with exit code %d", pidListIter->first, WEXITSTATUS(retValue));
                  gtError (buffer, WEXITSTATUS(retValue), DEFAULT_ERROR);
               } 
               else if (WIFSIGNALED(retValue)) 
               {
                  char buffer[256];
                  snprintf(buffer, sizeof(buffer), "Child %d terminated with signal %d", pidListIter->first, WTERMSIG(retValue));
                  gtError (buffer, -1, DEFAULT_ERROR);
               } 

               totalDataDownloaded += pidListIter->second->dataDownloaded;
               fclose (pidListIter->second->pipeHandle);
               delete pidListIter->second;
               pidList.erase(pidListIter++);
            }
            else
            {
               xfer += childXfer;
               pidListIter++;
            }
         }
  
         if (pidList.begin() == pidList.end())   // Transfers are done, all children have died, 100% should have been reported on the previous pass.
         {
            break;
         }

         freeSpace = getFreeDiskSpace();

         // Note that totalDataDownloaded and xfer both reflect retransmissions, so they are only approximate.  It is
         // possible (even common) for 
         //    (totalDataDownloaded + xfer) > totalSizeOfDownload 
         // So we need to be careful with the equation below so that it doesn't ov
         //
         // Dec 16, 2011, the above comment is no longer applicable to the best of my knowledge (DN)
         if (totalSizeOfDownload > totalDataDownloaded + xfer + freeSpace) 
         {
            if (freeSpace > DISK_FREE_WARN_LEVEL)
            {
               gtError ("The system *might* run out of disk space before all downloads are complete", NO_EXIT, gtBase::DEFAULT_ERROR, 0, "Downloading will continue until less than " + add_suffix (DISK_FREE_WARN_LEVEL) + " is available.");
            }
            else
            {
                   gtError ("The system is running low on disk space.  Closing download client", 97, gtBase::DEFAULT_ERROR, 0);
            }
         }

         if (_verbosityLevel > VERBOSE_1) 
         {
            screenOutput ("Status:"  << std::setw(8) << (totalDataDownloaded+xfer > 0 ? add_suffix(totalDataDownloaded+xfer).c_str() : "0 bytes") <<  " downloaded (" << std::fixed << std::setprecision(3) << (100.0*(totalDataDownloaded+xfer)/totalSizeOfDownload) << "% complete) current rate:  " << add_suffix (dlRate).c_str() << "/s");
         }
      }

      vectIter++;
   }
}

int gtDownload::downloadChild(int childID, int totalChildren, std::string torrentName, FILE *fd)
{
   gtLogger::delete_globallog();
   _logToStdErr = gtLogger::create_globallog (PACKAGE_NAME, _logDestination, childID);

   libtorrent::session torrentSession (*_gtFingerPrint, 0, libtorrent::alert::all_categories);
   optimizeSession (torrentSession);
   bindSession (torrentSession);

   std::string uuid = torrentName;
   uuid = uuid.substr (0, uuid.rfind ('.'));
   uuid = getFileName (uuid); 

   libtorrent::add_torrent_params torrentParams;
   torrentParams.save_path = "./";
   torrentParams.allow_rfc1918_connections = true;
   torrentParams.auto_managed = false;

   if (statDirectory ("./" + uuid) == 0)
   {
      torrentParams.force_download = false;  // allows resume
   }
   else
   {
      torrentParams.force_download = true; 
   }

   libtorrent::error_code torrentError;

   torrentParams.ti = new libtorrent::torrent_info (torrentName, torrentError);

   if (torrentError)
   {
      gtError (".gto processing problem", 217, TORRENT_ERROR, torrentError.value (), "", torrentError.message ());
   }

   libtorrent::torrent_handle torrentHandle = torrentSession.add_torrent (torrentParams, torrentError);

   if (torrentError)
   {
      gtError ("problem adding .gto to session", 217, TORRENT_ERROR, torrentError.value (), "", torrentError.message ());
   }

   // Mark the pieces this child does NOT want
   int pieces = torrentParams.ti->num_pieces();
   int chunkSize = pieces / totalChildren;
   int myStartPiece = (childID - 1) * chunkSize;
   int myEndPiece = (childID * chunkSize);

   if (childID == totalChildren)
   {
      myEndPiece = pieces;
   }

   for (int idx = 0; idx < myStartPiece; idx++)
   {
      torrentHandle.piece_priority (idx, 0);
   }
 
   for (int idx = myEndPiece; idx < pieces; idx++)
   {
      torrentHandle.piece_priority (idx, 0);
   }

   torrentHandle.set_sequential_download (true);

   if (torrentParams.ti->ssl_cert().size() > 0)
   {
      std::string sslCert = _tmpDir + uuid + ".crt";
      std::string sslKey = _tmpDir + uuid + ".key";
            
      torrentHandle.set_ssl_certificate (sslCert, sslKey, _dhParamsFile);
   }
   
   torrentHandle.resume();

   libtorrent::torrent_status::state_t currentState = torrentHandle.status().state;
   while (currentState != libtorrent::torrent_status::seeding && currentState != libtorrent::torrent_status::finished)
   {
      if (getppid() == 1)   // Parent has died, follow course
      {
         if (childID == 1)   // Only alert that download children are dying in 1 child
         {
            gtError ("download parent process has died, all download children are exiting", 197, gtBase::DEFAULT_ERROR);
         }
         exit (197);
      }

      libtorrent::ptime endMonitoring = libtorrent::time_now_hires() + libtorrent::seconds (5);  // 5 seconds

      while (currentState != libtorrent::torrent_status::seeding && currentState != libtorrent::torrent_status::finished && libtorrent::time_now_hires() < endMonitoring)
      {
         checkAlerts (torrentSession);
         usleep(ALERT_CHECK_PAUSE_INTERVAL);
         currentState = torrentHandle.status().state;
      }

      if (currentState == libtorrent::torrent_status::seeding)
      {
         break;
      }

      checkAlerts (torrentSession);

      libtorrent::session_status sessionStatus = torrentSession.status ();
      libtorrent::torrent_status torrentStatus = torrentHandle.status ();

      fprintf (fd, "%lld\n", (long long) torrentStatus.total_wanted_done);
      fprintf (fd, "%d\n", torrentStatus.download_payload_rate);
      fflush (fd);

      if (_verbosityLevel > VERBOSE_2)
      {
         if (torrentStatus.state != libtorrent::torrent_status::queued_for_checking && torrentStatus.state != libtorrent::torrent_status::checking_files)
         {
            screenOutput ("Child " << childID << " " << download_state_str[torrentStatus.state] << "  " << add_suffix (torrentStatus.total_wanted_done).c_str () << "  (" << add_suffix (torrentStatus.download_payload_rate, "/s").c_str () << ")");
         }
      }
      currentState = torrentHandle.status().state;
   }

   checkAlerts (torrentSession);
   torrentSession.remove_torrent (torrentHandle);

    // Note that remove_torrent does at least two things asynchronously: 1) it
    // sets in motion the deletion of this torrent object, and 2) it sends the
    // stopped event to the tracker and waits for a response.  So if we were to
    // exit immediately, two bad things happen.  First, the stopped event is
    // probably not sent.  Second, we end up doubly-deleting some objects
    // inside of libtorrent (because the deletion is already in progress and
    // then the call to exit() causes some cleanup as well), and we get nasty
    // complaints about malloc corruption printed to the console by glibc.
    //
    // The "proper" approached from a libtorrent perspective is to wait to
    // receive both the cache_flushed_alert and the tracker_reply_alert,
    // indicating that all is well.  However in the case of tearing down a
    // torrent, libtorrent appears to squelch the tracker_reply_alert so we
    // never get it (even though the tracker does in fact respond to the
    // stopped event sent to it by libtorrent).
    //
    // Therefore, ugly as it is, for the time being we will simply sleep here.
    // TODO: fix libtorrent so we ca do the proper thing of waiting to receive
    // the two events mentioned above.

   sleep(5);
   checkAlerts (torrentSession);

   exit (0);
}

int64_t gtDownload::getFreeDiskSpace ()
{
   struct statvfs buf;

   static std::string workingDir = getWorkingDirectory ();

   if (!statvfs (workingDir.c_str (), &buf))
   {
      return buf.f_bsize * buf.f_bavail;
   }
   else
   {
      return -1;
   }
}

void gtDownload::validateAndCollectSizeOfTorrents (uint64_t &totalBytes, int &totalFiles, int &totalGtos)
{
   if (_torrentListToDownload.size() < 1)
   {
      gtError ("the XML file did not contain any GTO file URIs.", 97, gtBase::DEFAULT_ERROR);
   }

   vectOfStr::iterator vectIter = _torrentListToDownload.begin ();

   totalBytes = 0;
   totalFiles = 0;
   totalGtos = 0;

   while (vectIter != _torrentListToDownload.end ())
   {
      libtorrent::error_code torrentError;
      libtorrent::torrent_info torrentInfo (*vectIter, torrentError);

      if (torrentError)
      {
         gtError (".gto processing problem", 87, TORRENT_ERROR, torrentError.value (), "", torrentError.message ());
      }

      totalBytes += torrentInfo.total_size ();
      totalFiles += torrentInfo.num_files ();
      totalGtos++;
      vectIter++;
   }

   if (totalBytes < 1 || totalFiles < 1)
   {
      gtError ("no data in GTOs to download.", 97, gtBase::DEFAULT_ERROR);
   }
}
