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
 * gtUpload.cpp
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

#include "gtUpload.h"
#include "stringTokenizer.h"
#include "gtDefs.h"
#include "geneTorrentUtils.h"
#include "gtLog.h"
#include "loggingmask.h"

static char const* upload_state_str[] = {
   "checking (q)",                     // queued_for_checking,
   "checking",                         // checking_files,
   "dl metadata",                      // downloading_metadata,
   "starting",                         // downloading,
   "finished",                         // finished,
   "uploading",                        // seeding,
   "allocating",                       // allocating,
   "checking (r)"                      // checking_resume_data
};

extern void *geneTorrCallBackPtr; 

gtUpload::gtUpload (boost::program_options::variables_map &vm) : gtBase (vm, UPLOAD_MODE),    _manifestFile (""), _uploadUUID (""), _uploadSubmissionURL (""), _filesToUpload (), _pieceSize (4194304), _dataFilePath ("")
{
   pcfacliUpload (vm);

   _dataFilePath = pcfacliPath (vm);

   checkCredentials ();

   _startUpComplete = true;

   if (_verbosityLevel > 0)
   {
      screenOutput ("Welcome to GeneTorrent version " << VERSION << ", upload mode."); 
   }
}

void gtUpload::pcfacliUpload (boost::program_options::variables_map &vm)
{
   if (vm.count (CRED_FILE_CLI_OPT) == 1 && vm.count (CRED_FILE_CLI_OPT_LEGACY) == 1)
   {
      commandLineError ("duplicate config options:  " + CRED_FILE_CLI_OPT + " and " + CRED_FILE_CLI_OPT_LEGACY + " are not permitted at the same time");
   }

   std::string credsPathAndFile;

   if (vm.count (UPLOAD_FILE_CLI_OPT) == 1)
   {
      _manifestFile = vm[UPLOAD_FILE_CLI_OPT].as<std::string>();
   }
   else if (vm.count (UPLOAD_FILE_CLI_OPT_LEGACY) == 1)
   {
      _manifestFile = vm[UPLOAD_FILE_CLI_OPT_LEGACY].as<std::string>();
   }

   if (statFileOrDirectory (_manifestFile) != 0)
   {
      commandLineError ("file not found (or is not readable):  " + credsPathAndFile);
   }
}

void gtUpload::run ()
{
   processManifestFile ();

   if (_uploadSubmissionURL.size () < 1)
   {
      gtError ("No Submission URL found in manifest file:  " + _manifestFile, 214, gtUpload::DEFAULT_ERROR);
   }

   if (_uploadUUID.size () < 1)
   {
      gtError ("No server_path (UUID) found in manifest file:  " + _manifestFile, 214, gtUpload::DEFAULT_ERROR);
   }

   if (_filesToUpload.size () < 1)
   {
      gtError ("No files found in manifest file:  " + _manifestFile, 214, gtUpload::DEFAULT_ERROR);
   }

   std::string torrentFileName;
   if (_devMode == false)
   {
      findDataAndSetWorkingDirectory ();
      setPieceSize ();

      torrentFileName = makeTorrent (_uploadUUID, _uploadUUID + GTO_FILE_EXTENSION);
      torrentFileName = submitTorrentToGTExecutive (torrentFileName);
   }
   else
   {
      torrentFileName = _uploadUUID + GTO_FILE_EXTENSION;
   }

   performGtoUpload (torrentFileName);
}

std::string gtUpload::submitTorrentToGTExecutive (std::string tmpTorrentFileName)
{
   std::string realTorrentFileName = tmpTorrentFileName.substr (0, tmpTorrentFileName.size () - 1); // drop the ~ from uuid.gto~

   std::string uuidForErrors = _uploadUUID;

   FILE *gtoFile;

   gtoFile = fopen (realTorrentFileName.c_str (), "wb");

   if (gtoFile == NULL)
   {
      gtError ("Failure opening " + realTorrentFileName + " for output.", 202, ERRNO_ERROR, errno);
   }

   char errorBuffer[CURL_ERROR_SIZE + 1];
   errorBuffer[0] = '\0';

   std::string curlResponseHeaders = "";

   CURL *curl;
   curl = curl_easy_init ();

   if (!curl)
   {
      gtError ("libCurl initialization failure", 201);
   }

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

   std::string data = "token=" + _authToken;

   struct curl_httppost *post=NULL;
   struct curl_httppost *last=NULL;

   curl_formadd (&post, &last, CURLFORM_COPYNAME, "token", CURLFORM_COPYCONTENTS, _authToken.c_str(), CURLFORM_END);

   curl_formadd (&post, &last, CURLFORM_COPYNAME, "file", CURLFORM_FILE, tmpTorrentFileName.c_str(), CURLFORM_END);

   curl_easy_setopt (curl, CURLOPT_HTTPPOST, post);

   int timeoutVal = 15;
   int connTime = 4;

   curl_easy_setopt (curl, CURLOPT_URL, _uploadSubmissionURL.c_str());
   curl_easy_setopt (curl, CURLOPT_TIMEOUT, timeoutVal);
   curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, connTime);

   CURLcode res;

   res = curl_easy_perform (curl);

   if (res != CURLE_OK)
   {
      curlCleanupOnFailure (realTorrentFileName, gtoFile);
      gtError ("Problem communicating with GeneTorrent Executive while trying to submit metadata for UUID:  " + _uploadUUID, 203, gtUpload::CURL_ERROR, res, "URL:  " + _uploadSubmissionURL);
   }

   long code;
   res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

   if (res != CURLE_OK)
   {
      curlCleanupOnFailure (realTorrentFileName, gtoFile);
      gtError ("Problem communicating with GeneTorrent Executive while trying to submit metadata for UUID:  " + _uploadUUID, 204, gtUpload::DEFAULT_ERROR, 0, "URL:  " + _uploadSubmissionURL);
   }

   if (code != 200)
   {
      curlCleanupOnFailure (realTorrentFileName, gtoFile);
      gtError ("Problem communicating with GeneTorrent Executive while trying to submit metadata for UUID:  " + _uploadUUID, 205, gtUpload::HTTP_ERROR, code, "URL:  " + _uploadSubmissionURL);
   }

   if (_verbosityLevel > VERBOSE_3)
   {
      screenOutput ("Headers received from the client:  '" << curlResponseHeaders << "'" << std::endl);
   }

   curl_easy_cleanup (curl);

   fclose (gtoFile);

   return realTorrentFileName;
}

void gtUpload::findDataAndSetWorkingDirectory ()
{
   vectOfStr missingFiles;

   if (_dataFilePath.size () > 0) // User included a -p on the command line, move there
   {
      int cdRet = chdir (_dataFilePath.c_str ());

      if (cdRet != 0 )
      {
         gtError ("Failure changing directory to " + _dataFilePath, 202, ERRNO_ERROR, errno);
      }
   }

   // first look for files in the current directory
   if (verifyDataFilesExist (missingFiles))
   {
      return; // all data is present
   }

   if (missingFiles.size () != _filesToUpload.size ()) // only some of the files are missing, we have the correct directory, error
   {                                                   // if all files are missing presume wrong directory and continue
      displayMissingFilesAndExit (missingFiles);
   }

   missingFiles.clear ();

   int cdRet = chdir (".."); // move up one directory and try again

   if (cdRet != 0 )
   {
      gtError ("Failure changing directory to .. from " + getWorkingDirectory(), 202, ERRNO_ERROR, errno);
   }

   if (verifyDataFilesExist (missingFiles))
   {
      return; // all data is present
   }

   displayMissingFilesAndExit (missingFiles); // Give up
}

// This function verifies that all files specified in the manifest file exist in a
// directory named with the UUID in the manifest.
// the caller must be in the correct system directory when calling this function
// error check is the responsibility of the caller
bool gtUpload::verifyDataFilesExist (vectOfStr &missingFileList)
{
   bool missingFiles = false;
   std::string workingDataPath = _uploadUUID + "/";

   vectOfStr::iterator vectIter = _filesToUpload.begin ();

   while (vectIter != _filesToUpload.end ())
   {
      if (statFileOrDirectory (workingDataPath + *vectIter) != 0)
      {
         missingFileList.push_back (*vectIter);
         missingFiles = true;
      }
      vectIter++;
   }

   return missingFiles ? false : true;
}

void gtUpload::setPieceSize ()
{
   struct stat fileStatus;
   unsigned long totalDataSize = 0;

   vectOfStr::iterator vectIter = _filesToUpload.begin ();

   while (vectIter != _filesToUpload.end ())
   {
      int ret = stat ((_uploadUUID + "/" + *vectIter).c_str (), &fileStatus);

      if (ret == 0)
      {
         totalDataSize += fileStatus.st_size;
      }
      vectIter++;
   }

   while (totalDataSize / _pieceSize > 15000)
   {
      _pieceSize *= 2;
   }
}

void gtUpload::displayMissingFilesAndExit (vectOfStr &missingFiles)
{
   vectOfStr::iterator vectIter = missingFiles.begin ();

   while (vectIter != missingFiles.end ())
   {
      screenOutput ("Error:  " << strerror (errno) << " (errno = " << errno << ")  FileName:  " << _uploadUUID << "/" << *vectIter);
      vectIter++;
   }
   gtError ("file(s) listed above were not found (or is (are) not readable)", 82, gtUpload::DEFAULT_ERROR);
}

std::string gtUpload::makeTorrent (std::string filePath, std::string torrentName)
{
   std::string creator = std::string ("GeneTorrent-") + VERSION;

   int flags = 0;

   torrentName += '~';

   try
   {
      libtorrent::file_storage fileStore;

      std::string full_path = libtorrent::complete (filePath);

      libtorrent::add_files (fileStore, full_path, file_filter, flags);

      libtorrent::create_torrent torrent (fileStore, _pieceSize, -1, 0);
      torrent.add_tracker (DEFAULT_TRACKER_URL);

      libtorrent::set_piece_hashes (torrent, libtorrent::parent_path(full_path));
      torrent.set_creator (creator.c_str ());

      std::vector <char> finishedTorrent;
      bencode (back_inserter (finishedTorrent), torrent.generate ());

      FILE *output = fopen (torrentName.c_str (), "wb+");

      if (output == NULL)
      {
         gtError ("Failure opening " + torrentName + " for output.", 202, ERRNO_ERROR, errno);
      }

      fwrite (&finishedTorrent[0], 1, finishedTorrent.size (), output);
      fclose (output);

      return torrentName;
   }
   catch (...)
   {
       // TODO: better error handling here
       exit (71);
   }
   return "";
}

void gtUpload::processManifestFile ()
{
   vectOfStr fileNames;

   XQilla xqilla;
   AutoDelete <XQQuery> query (xqilla.parse (X("for $variable in //SUBMISSION return //$variable/(SERVER_INFO/(@server_path|@submission_uri)|FILES/FILE/@filename)")));
   AutoDelete <DynamicContext> context (query->createDynamicContext ());
   Sequence seq = context->resolveDocument (X(_manifestFile.c_str()));

   if (!seq.isEmpty () && seq.first ()->isNode ())
   {
      context->setContextItem (seq.first ());
      context->setContextPosition (1);
      context->setContextSize (1);
   }

   Result result = query->execute (context);

   Item::Ptr item;

   item = result->next (context);

   while (item != NULL)
   {
      strTokenize strToken (std::string (UTF8(item->asString(context))), "{}\"=", strTokenize::MERGE_CONSECUTIVE_SEPARATORS);

      std::string token1 = strToken.getToken (1);

      if (token1 == "server_path")
      {
         _uploadUUID = strToken.getToken (2);
      }
      else if (token1 == "submission_uri")
      {
         _uploadSubmissionURL = strToken.getToken (2);
      }
      else if (token1 == "filename")
      {
         _filesToUpload.push_back (strToken.getToken (2));
      }
      else
      {
         gtError ("Invalid manifest.xml file, unexpected attribute returned:  '" + token1 + "'", 201);
      }

      item = result->next (context); // Get the next node in the list from the xquery
   }
}

void gtUpload::performGtoUpload (std::string torrentFileName)
{
   if (_verbosityLevel > 0)
   {
      screenOutput ("Sending " << torrentFileName); 
   }

   libtorrent::session torrentSession (*_gtFingerPrint, 0, libtorrent::alert::all_categories);
   optimizeSession (torrentSession);
   bindSession (torrentSession);

   libtorrent::add_torrent_params torrentParams;
   torrentParams.seed_mode = true;
   torrentParams.disable_seed_hash = true;
   torrentParams.allow_rfc1918_connections = true;
   torrentParams.auto_managed = false;
   torrentParams.save_path = "./";

   libtorrent::error_code torrentError;

   torrentParams.ti = new libtorrent::torrent_info (torrentFileName, torrentError);

   if (torrentError)
   {
      gtError (".gto processing problem", 217, TORRENT_ERROR, torrentError.value (), "", torrentError.message ());
   }

   libtorrent::torrent_handle torrentHandle = torrentSession.add_torrent (torrentParams, torrentError);

   if (torrentError)
   {
      gtError ("problem adding .gto to session", 217, TORRENT_ERROR, torrentError.value (), "", torrentError.message ());
   }

   std::string uuid = torrentFileName;
   uuid = uuid.substr (0, uuid.rfind ('.'));
   uuid = getFileName (uuid); 

   int sslCertSize = torrentParams.ti->ssl_cert().size();

   std::string uri = _uploadSubmissionURL;

   if (sslCertSize > 0 && _devMode == false)
   {
      std::string pathToKeep = "/cghub/data/";
  
      std::size_t foundPos;
 
      if (std::string::npos == (foundPos = uri.find (pathToKeep)))
      {
         gtError ("Unable to find " + pathToKeep + " in the URL:  " + uri, 214, gtUpload::DEFAULT_ERROR);
      }

      std::string certSignURL = uri.substr(0, foundPos + pathToKeep.size()) + GT_CERT_SIGN_TAIL;

      generateSSLcertAndGetSigned(torrentFileName, certSignURL, uuid);
   }
      
   if (sslCertSize > 0)
   {
      std::string sslCert = _tmpDir + uuid + ".crt";
      std::string sslKey = _tmpDir + uuid + ".key";
            
      torrentHandle.set_ssl_certificate (sslCert, sslKey, _dhParamsFile);
   }

   torrentHandle.resume();

   bool displayed100Percent = false;

   libtorrent::session_status sessionStatus = torrentSession.status ();
   libtorrent::torrent_status torrentStatus = torrentHandle.status ();

   while (torrentStatus.num_complete < 2)
   {
      torrentHandle.scrape_tracker();
      sessionStatus = torrentSession.status();
      torrentStatus = torrentHandle.status();

      libtorrent::ptime endMonitoring = libtorrent::time_now_hires() + libtorrent::seconds (5);

      while (torrentStatus.num_complete < 2 && libtorrent::time_now_hires() < endMonitoring)
      {
         checkAlerts (torrentSession);
         usleep(ALERT_CHECK_PAUSE_INTERVAL);
      }

      if (_verbosityLevel > 0 && !displayed100Percent)
      {
         if (torrentStatus.state != libtorrent::torrent_status::queued_for_checking && torrentStatus.state != libtorrent::torrent_status::checking_files)
         {
            double percentComplete = torrentStatus.total_payload_upload / (torrentParams.ti->total_size () * 1.0) * 100.0;

            if (percentComplete > 99.999999999)
            {
               percentComplete = 100.000000;
               displayed100Percent = true;
            }
            screenOutput ("Status:"  << std::setw(8) << (torrentStatus.total_payload_upload > 0 ? add_suffix(torrentStatus.total_payload_upload).c_str() : "0 bytes") <<
                                     " uploaded (" << std::fixed << std::setprecision(3) << percentComplete <<
                                     "% complete) current rate:  " << add_suffix (torrentStatus.upload_rate, "/s"));
         }
      }
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
}

// do not include files that are not present in _filesToUpload
bool gtUpload::fileFilter (std::string const fileName)
{
   vectOfStr::iterator vectIter = _filesToUpload.begin ();

   if (fileName == _uploadUUID) // Include the UUID directory
   {
      return true;
   }

   while (vectIter != _filesToUpload.end ())
   {
      if (*vectIter == fileName)
      {
         return true;
      }
      vectIter++;
   }
   return false;
}

// do not include files and folders whose name starts with a ., based on file_filter from libtorrent
bool gtUpload::file_filter (boost::filesystem::path const& filename)
{
   std::string fileName = filename.filename().string();

   if (fileName[0] == '.')
      return false;

   return ((gtUpload *)geneTorrCallBackPtr)->fileFilter (fileName);
}
