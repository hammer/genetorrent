/* -*- mode: C++; c-basic-offset: 3; tab-width: 3; -*-
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

#include "gt_config.h"

#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdio>

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
#include "gtNullStorage.h"
#include "gtZeroStorage.h"

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

gtDownload::gtDownload (gtDownloadOpts &opts, bool show_startup_message):
   gtBase (opts, DOWNLOAD_MODE),
   _downloadSavePath (opts.m_downloadSavePath),
   _cliArgsDownloadList (opts.m_cliArgsDownloadList),
   _maxChildren (opts.m_maxChildren),
   _torrentListToDownload (),
   _uriListToDownload (),
   _downloadModeCsrSigningUrl (opts.m_csrSigningUrl)
{
   if (show_startup_message)
   {
      startUpMessage (opts.m_progName);
   }

   _startUpComplete = true;
}

void gtDownload::run ()
{
#if defined HAVE_GETRLIMIT && defined HAVE_SETRLIMIT && defined RLIMIT_NPROC
   // check system resources
   // GeneTorrent downloads are thread/process intensive
   // If possible, set the soft process limit for user to at
   // least PROCESS_MIN for this process.
   struct rlimit r;

   if (!getrlimit (RLIMIT_NPROC, &r))
   {
      if (r.rlim_cur < PROCESS_MIN &&
          (r.rlim_max >= PROCESS_MIN || r.rlim_max == RLIM_INFINITY))
      {
         r.rlim_cur = PROCESS_MIN;
         // no change to rlim_max, since that is a hard system policy

         std::ostringstream errorstr;
         errorstr << "setting user process soft limit to ";
         errorstr << PROCESS_MIN;
         gtError (errorstr.str(), NO_EXIT);

         if (setrlimit (RLIMIT_NPROC, &r))
         {
            gtError ("setting user process soft limit failed", NO_EXIT);
         }
      }
   }
#endif

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

   int64_t totalBytes = 0;
   int totalFiles = 0;
   int totalGtos = 0;

   std::ostringstream message;
   message << "Ready to download";

   Log (PRIORITY_NORMAL, "%s", message.str().c_str());

   screenOutput (message.str(), VERBOSE_1);

   // First, go download any torrents that the user requested by
   // passing a .gto on the command line. The assumption here is that
   // the .gto file has already been requested from GTO Executive and
   // thus the clock is ticking before the .gto expires.
   performTorrentDownloadsByGTO (totalBytes, totalFiles, totalGtos);

   // Next, download any torrents the user requested by passing either
   // URI or xml file on the command line. This will result in the
   // .gto being requested and downloaded so that the torrent can be
   // downloaded.
   performTorrentDownloadsByURI (totalBytes, totalFiles, totalGtos);

   message.str("");
  
   time_t duration = time(NULL) - startTime;
   time_t rate = duration ? (totalBytes / duration) : 0;

   message << "Downloaded " << add_suffix (totalBytes) << " in " << durationToStr(duration) << ".  Overall Rate " << add_suffix (rate) << "/s";

   Log (PRIORITY_NORMAL, "%s", message.str().c_str());

   screenOutput (message.str(), VERBOSE_1);

   if (chdir (saveDir.c_str ())) {
      Log (PRIORITY_NORMAL, "Failed to chdir to saveDir"); 
   }
}

void gtDownload::prepareDownloadList ()
{
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
            extractURIsFromXML (inspect, _uriListToDownload);
         }
         else if (std::string::npos != inspect.find ("/")) // Have a URI
         {
            _uriListToDownload.push_back (inspect);
         }
         else // we have a UUID
         {
            std::string url = CGHUB_WSI_BASE_URL + "download/" + inspect;
            _uriListToDownload.push_back (url);
         }
      }
      else
      {
         gtError ("-d download argument unrecognized.  '" + inspect + "' is too short'", 201);
      }

      vectIter++;
   }
}

bool gtDownload::downloadGTO (std::string uri, std::string fileName, std::string torrUUID, int retryCount)
{
   checkIPFilter (uri);

   bool curl_status;
   CURL *curl;
   curl = curl_easy_init ();

   if (!curl)
   {
      gtError ("libCurl initialization failure", 201);
   }

   std::string tmpFileName = fileName + ".tmp";

   FILE *gtoFile;

   gtoFile = fopen (tmpFileName.c_str (), "wb");

   if (gtoFile == NULL)
   {
      gtError ("Failure opening " + tmpFileName + " for output.", 202, ERRNO_ERROR, errno);
   }

   char errorBuffer[CURL_ERROR_SIZE + 1];
   errorBuffer[0] = '\0';

   std::string curlResponseHeaders = "";

   if (!_curlVerifySSL)
   {
      curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 0);
      curl_easy_setopt (curl, CURLOPT_SSL_VERIFYHOST, 0);
   }

   curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, errorBuffer);
   curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, NULL);
   curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION, &curlCallBackHeadersWriter);
   curl_easy_setopt (curl, CURLOPT_MAXREDIRS, 15);
   curl_easy_setopt (curl, CURLOPT_WRITEDATA, gtoFile);
   curl_easy_setopt (curl, CURLOPT_WRITEHEADER, &curlResponseHeaders);
   curl_easy_setopt (curl, CURLOPT_NOSIGNAL, (long)1);
#ifdef __CYGWIN__
   std::string winInst = getWinInstallDirectory () + "/cacert.pem";
   curl_easy_setopt (curl, CURLOPT_CAINFO, winInst.c_str ());
#endif /* __CYGWIN__ */

   curl_easy_setopt (curl, CURLOPT_POST, (long)1);

   struct curl_httppost *post=NULL;
   struct curl_httppost *last=NULL;

   curl_formadd (&post, &last, CURLFORM_COPYNAME, "token",
                 CURLFORM_COPYCONTENTS, _authToken.c_str(), CURLFORM_END);

   curl_easy_setopt (curl, CURLOPT_HTTPPOST, post);

   // CGHUBDEV-22: Set CURL timeouts to 20 seconds
   int timeoutVal = 20;
   int connTime = 20;

   curl_easy_setopt (curl, CURLOPT_URL, uri.c_str());
   curl_easy_setopt (curl, CURLOPT_TIMEOUT, timeoutVal);
   curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, connTime);

   CURLcode res;

   res = curl_easy_perform (curl);

   if (res == CURLE_SSL_CONNECT_ERROR || res == CURLE_OPERATION_TIMEDOUT)
   {
      // Only retry on SSL connect errors or timeouts in case the
      // other end is temporarily overloaded.

      // Give the other end time to become less loaded.
      sleep (2);
   }

   fclose (gtoFile);

   screenOutput ("Headers received from the client:  '" << curlResponseHeaders << "'" << std::endl, VERBOSE_2);

   curl_status = processCurlResponse (curl, res, tmpFileName, uri, torrUUID, "Problem communicating with GeneTorrent Executive while trying to retrieve GTO for UUID:", retryCount);

   curl_formfree(post);
   curl_easy_cleanup (curl);

   if (curl_status)
   {
      // If we got this far, the gto file should not have any chance of
      // containing xml error messages instead of torrent
      // information. Rename the tmp file to the real gto file.
      if (rename (tmpFileName.c_str(), fileName.c_str()) < 0)
      {
         gtError ("Failed to rename tmp gto to gto: " + tmpFileName + " -> " + fileName, 88, TORRENT_ERROR, 0, "", strerror(errno));
      }
   }

   return curl_status;
}

// Returns the path to the .gto file that was downloaded.
std::string gtDownload::downloadGtoFileByURI (std::string uri)
{
   screenOutput ("Communicating with GT Executive ...        ", VERBOSE_1);

   std::string fileName = uri.substr (uri.find_last_of ('/') + 1);
   fileName = fileName.substr (0, fileName.find_first_of ('?'));
   std::string torrUUID = fileName;
   fileName += GTO_FILE_EXTENSION;

   int retries = 5;

   while (retries > 0)
   {
      retries--;

      if (downloadGTO (uri, fileName, torrUUID, retries))
      {
         //success = true;
         break;
      }

      LogNormal ("GTO download failed, retrying");
   }

   std::string torrFile = getWorkingDirectory () + '/' + fileName;

   libtorrent::error_code torrentError;
   libtorrent::torrent_info torrentInfo (torrFile, torrentError);

   if (torrentError)
   {
      gtError (".gto processing problem with " + torrFile, 87, TORRENT_ERROR, torrentError.value (), "", torrentError.message ());
   }

   initiateCSR (torrUUID, torrFile, torrentInfo, uri);

   // FIXME: Does this also need to be moved into the performSingleTorrentDownload() function?
   if (global_gtAgentMode)
   {
      std::vector<libtorrent::announce_entry> const trackers = torrentInfo.trackers();

      std::vector<libtorrent::announce_entry>::const_iterator trackerIter = trackers.begin();
      while (trackerIter != trackers.end())
      {
         std::cout << (*trackerIter).url << std::endl;
         trackerIter++;
      }
   }

   return torrFile;
}

void gtDownload::extractURIsFromXML (std::string xmlFileName, vectOfStr &urisToDownload)
{
   XQilla xqilla;
   try
   {
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
         throw ("Empty set, invalid xml");
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
   catch (...)
   {
      gtError ("Encountered an error attempting to process the file:  " + xmlFileName + ".  Review the contents of the file.", 97, gtBase::DEFAULT_ERROR, 0);
   }
}

void gtDownload::initiateCSR(std::string torrUUID, std::string torrFile,
                             libtorrent::torrent_info &torrentInfo, std::string uri)
{
   if (torrentInfo.ssl_cert().size() > 0 && _devMode == false)
   {
      std::string certSignURL;

      // If no URI is passed, we're downloading from a GTO file
      // Use --security-api to sign our CSR
      if (uri.size () == 0)
      {
         if (_downloadModeCsrSigningUrl.size () == 0)
         {
            gtError ("No security API URI given for SSL-enabled GTO download:  " + torrFile, 214, gtBase::DEFAULT_ERROR);
         }

         certSignURL = _downloadModeCsrSigningUrl;
      }
      else
      {
         std::string pathToKeep = "/cghub/data/";

         std::size_t foundPos;

         if (std::string::npos == (foundPos = uri.find (pathToKeep)))
         {
            gtError ("Unable to find " + pathToKeep + " in the URL:  " + uri, 214, gtBase::DEFAULT_ERROR);
         }

         certSignURL = uri.substr(0, foundPos + pathToKeep.size()) + GT_CERT_SIGN_TAIL;
      }

      generateSSLcertAndGetSigned(torrFile, certSignURL, torrUUID);
   }
}

void gtDownload::spawnDownloadChildren (childMap &pidList, std::string torrentName, int num_pieces)
{
    // TODO: It would be good to use a system call to determine how
    // many cores this machine has.  There shouldn't be more children
    // than cores, so set
    //    maxChildren = min(_maxChildren, number_of_cores)
    // Hard to do this in a portable manner however

   int maxChildren = _maxChildren;
   int pipes[maxChildren+1][2];

   int childrenThisGTO = num_pieces >= maxChildren ? maxChildren : num_pieces;

   int childID=1;
   pid_t pid;

   while (childID <= childrenThisGTO)      // Spawn Children that will download this GTO
   {
      if (pipe (pipes[childID]) < 0)
      {
         gtError ("pipe() error", 107, ERRNO_ERROR, errno);
      }

      pid = fork();

      if (pid < 0)
      {
         gtError ("fork() error", 107, ERRNO_ERROR, errno);
      }
      else if (pid == 0)
      {
         close (pipes[childID][0]);

         FILE *foo =  fdopen (pipes[childID][1], "w");

         downloadChild (childID, childrenThisGTO, torrentName, foo);
         // Should never return from downloadChild().
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

      // throttle fork() calls so we don't overload the system with new
      // processes and their libtorrent threads
      // 1/10th of a second seems reasonable, and should be mostly imperceptible
      // to users (default calls to fork is 8)
      // In particular, we want GeneTorrent and libtorrent to stop spawning new
      // children (and threads of children) as soon as the resource is exhausted
      usleep (100000);

      // Stop forking if any children have terminated
      childMap::iterator pidListIter = pidList.begin();

      while (pidListIter != pidList.end())
      {
         int retValue = 0;
         pid_t pidStat = waitpid (pidListIter->first, &retValue, WNOHANG);

         if (pidStat == pidListIter->first &&
             (WIFEXITED(retValue) || WIFSIGNALED(retValue)))
         {
            // Child terminated abnormally, stop forking
            gtError ("download child exited abnormally", 107);
         }

         pidListIter++;
      }
   }
}

void gtDownload::performSingleTorrentDownload (std::string torrentName, int64_t &totalBytes, int &totalFiles)
{
   libtorrent::error_code torrentError;
   libtorrent::torrent_info torrentInfo (torrentName, torrentError);

   if (torrentError)
   {
      gtError (".gto processing problem", 217, TORRENT_ERROR, torrentError.value (), "", torrentError.message ());
   }

   int64_t totalSizeOfDownload = torrentInfo.total_size ();
   int64_t totalDataDownloaded = 0;

   int numFilesDownloaded = torrentInfo.num_files ();

   if (totalSizeOfDownload < 1)
   {
      gtError("Size of torrent data is zero bytes: " + torrentName, NO_EXIT);
   }

   if (numFilesDownloaded < 1)
   {
      gtError("Torrent contains no files: " + torrentName, NO_EXIT);
   }

   childMap pidList;

   spawnDownloadChildren(pidList, torrentName, torrentInfo.num_pieces());

   int64_t xfer = 0;
   int64_t childXfer = 0;
   int64_t totalXfer = 0; // total bytes downloaded as of last outer loop iteration
   int64_t dlRate = 0;
   time_t lastActivity = timeout_update ();  // initialize last activity time for inactvitiy timeout

   while (pidList.size() > 0)
   {
      xfer = 0;
      dlRate = 0;
      childXfer = 0;

      // Inactivity timeout check
      if (timeout_check_expired (&lastActivity))
      {
         // Timeout message and exit here
         // This is parent process, children will exit
         std::ostringstream timeLenStr;
         timeLenStr << _inactiveTimeout;
         gtError ("Inactivity timeout triggered after " + timeLenStr.str() + " minute(s).  Shutting down download client.", 206, gtBase::DEFAULT_ERROR, 0);
      }

      childMap::iterator pidListIter = pidList.begin();

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
               gtError (buffer, 207, DEFAULT_ERROR);
            }

            totalDataDownloaded += pidListIter->second->dataDownloaded;
            timeout_update (&lastActivity);
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

      int64_t freeSpace = getFreeDiskSpace();

      if (freeSpace > 0 && totalSizeOfDownload > totalDataDownloaded +
         xfer + freeSpace)
      {
         if (freeSpace > DISK_FREE_WARN_LEVEL)
         {
            gtError ("The system *might* run out of disk space before all downloads are complete", ERROR_NO_EXIT, gtBase::DEFAULT_ERROR, 0, "Downloading will continue until less than " + add_suffix (DISK_FREE_WARN_LEVEL) + " is available.");
         }
         else
         {
            gtError ("The system is running low on disk space.  Shutting down download client.", 97, gtBase::DEFAULT_ERROR, 0);
         }
      }

      screenOutput ("Status:"  << std::setw(8) << (totalDataDownloaded+xfer > 0 ? add_suffix(totalDataDownloaded+xfer).c_str() : "0 bytes") <<  " downloaded (" << std::fixed << std::setprecision(3) << (100.0*(totalDataDownloaded+xfer)/totalSizeOfDownload) << "% complete) current rate:  " << add_suffix (dlRate).c_str() << "/s", VERBOSE_1);

      if (totalDataDownloaded + xfer > totalXfer)
         timeout_update (&lastActivity);
      totalXfer = totalDataDownloaded + xfer;
   }

   totalBytes += totalDataDownloaded;
   totalFiles += numFilesDownloaded;
}

void gtDownload::performTorrentDownloadsByGTO (int64_t &totalBytes, int &totalFiles, int &totalGtos)
{
   vectOfStr::iterator iter = _torrentListToDownload.begin ();

   while (iter != _torrentListToDownload.end ())
   {
      std::string torrentName = *iter;

      // Capture analysis object UUID from torrent file name
      size_t offset = torrentName.find_last_of("/\\");
      if (std::string::npos == offset)
         offset = 0;
      else
         offset++;

      std::string basename = torrentName.substr (offset);
      std::string uuid = basename.substr (0, basename.find_first_of ('.'));

      libtorrent::error_code torrentError;
      libtorrent::torrent_info torrentInfo (torrentName, torrentError);

      if (torrentError)
      {
         gtError (".gto processing problem", 217, TORRENT_ERROR, torrentError.value (), "", torrentError.message ());
      }

      initiateCSR (uuid, torrentName, torrentInfo);

      performSingleTorrentDownload (torrentName, totalBytes, totalFiles);
      totalGtos++;

      iter++;
   }
}

void gtDownload::performTorrentDownloadsByURI (int64_t &totalBytes, int &totalFiles, int &totalGtos)
{
   vectOfStr::iterator iter = _uriListToDownload.begin ();

   while (iter != _uriListToDownload.end ())
   {
      std::string uri = *iter;

      std::string torrentName = downloadGtoFileByURI (uri);
      performSingleTorrentDownload (torrentName, totalBytes, totalFiles);
      totalGtos++;

      iter++;
   }
}

int gtDownload::downloadChild(int childID, int totalChildren, std::string torrentName, FILE *fd)
{
   gtLogger::delete_globallog();
   _logToStdErr = gtLogger::create_globallog (_progName, _logDestination, childID);

#if __CYGWIN__
   // Ignore SIGPIPE on Windows to prevent download child from being killed
   // by signal when accept returns ECONNABORTED in libtorrent
   struct sigaction action;
   action.sa_handler = SIG_IGN;
   sigemptyset(&action.sa_mask);
   action.sa_flags = 0;
   sigaction(SIGPIPE, &action, NULL);
#endif

   libtorrent::session *torrentSession = makeTorrentSession ();

   if (!torrentSession)
   {
      // Exiting with non-zero return code causes parent to exit
      // Other children notice that they're init orphans and exit in turn
      gtError ("unable to open a libtorrent session", 218, DEFAULT_ERROR);
   }

   std::string uuid = torrentName;
   uuid = uuid.substr (0, uuid.rfind ('.'));
   uuid = getFileName (uuid); 

   libtorrent::add_torrent_params torrentParams;
   torrentParams.save_path = "./";
   torrentParams.allow_rfc1918_connections = true;
   torrentParams.auto_managed = false;

   if (_use_zero_storage)
   {
      torrentParams.storage = zero_storage_constructor;
   }
   else if (_use_null_storage)
   {
      torrentParams.storage = null_storage_constructor;

      libtorrent::session_settings settings = torrentSession->settings ();
      settings.disable_hash_checks = true;
      torrentSession->set_settings (settings);

      screenOutput ("Hash checks disabled due to null-storage enabled.", VERBOSE_0);
   }

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

   libtorrent::torrent_handle torrentHandle = torrentSession->add_torrent (torrentParams, torrentError);

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
  
   if (_rateLimit > 0)
   { 
      torrentHandle.set_download_limit (_rateLimit/totalChildren);
   }

   // Don't allow upload connections
   torrentHandle.set_max_uploads(0);

   torrentHandle.resume();

   libtorrent::torrent_status::state_t currentState = torrentHandle.status().state;
   while (currentState != libtorrent::torrent_status::seeding && currentState != libtorrent::torrent_status::finished)
   {
      libtorrent::ptime endMonitoring = libtorrent::time_now_hires() + libtorrent::seconds (5);  // 5 seconds

      while (currentState != libtorrent::torrent_status::seeding && currentState != libtorrent::torrent_status::finished && libtorrent::time_now_hires() < endMonitoring)
      {
         checkAlerts (torrentSession);
         usleep(ALERT_CHECK_PAUSE_INTERVAL);
         currentState = torrentHandle.status().state;

         if (getppid() == 1)   // Parent has died, follow course
         {
            gtError ("download parent process has exited, gracefully exiting child process.", NO_EXIT);
            break;
         }
      }

      checkAlerts (torrentSession);

      // libtorrent::session_status sessionStatus = torrentSession->status ();
      libtorrent::torrent_status torrentStatus = torrentHandle.status ();

      fprintf (fd, "%lld\n", (long long) torrentStatus.total_wanted_done);
      fprintf (fd, "%d\n", torrentStatus.download_payload_rate);
      fflush (fd);

      if (currentState == libtorrent::torrent_status::seeding || getppid() == 1)
      {
         break;
      }

      if (torrentStatus.state != libtorrent::torrent_status::queued_for_checking && torrentStatus.state != libtorrent::torrent_status::checking_files)
      {
         screenOutput ("Child " << childID << " " << download_state_str[torrentStatus.state] << "  " << add_suffix (torrentStatus.total_wanted_done).c_str () << "  (" << add_suffix (torrentStatus.download_payload_rate, "/s").c_str () << ")", VERBOSE_2);
      }
      currentState = torrentHandle.status().state;
   }

   checkAlerts (torrentSession);
   torrentSession->remove_torrent (torrentHandle);

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

   // Tear down torrent session before exiting
   // This prevents race conditions by calling the libtorrent session
   // thread joins and object destructors in the expected order
   delete torrentSession;

   gtLogger::delete_globallog();

   exit (0);
}

int64_t gtDownload::getFreeDiskSpace ()
{
   struct statvfs buf;

   static std::string workingDir = getWorkingDirectory ();

   if (!statvfs (workingDir.c_str (), &buf))
   {
      int64_t bsize = buf.f_bsize;
      int64_t bavail = buf.f_bavail;
      return bsize * bavail;
   }
   else
   {
      gtError ("statvfs call failed", ERROR_NO_EXIT, gtBase::ERRNO_ERROR, errno);
      return -1;
   }
}
