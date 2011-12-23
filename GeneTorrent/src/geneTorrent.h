/*                                           -*- mode: c++; tab-width: 2; -*-
 * $Id$
 *
 * Copyright (c) 2011, Annai Systems, Inc.
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
 * geneTorrent.h
 *
 *  Created on: Aug 15, 2011
 *      Author: donavan
 */

#ifndef GENETORRENT_H_
#define GENETORRENT_H_

#include <pthread.h>

#include <config.h>

#include <string>
#include <map>
#include <vector>

#include <boost/filesystem/v3/path.hpp>

#include <log4cpp/Category.hh>
//#include <log4cpp/FileAppender.hh>
#include <log4cpp/PatternLayout.hh>
#include <log4cpp/SyslogAppender.hh>

#include <tclap/CmdLine.h>

#include "libtorrent/session.hpp"
#include "libtorrent/fingerprint.hpp"

class geneTorrent
{
   typedef enum opMode_ {DOWNLOAD_MODE = 77, SERVER_MODE, UPLOAD_MODE} opMode;
   typedef enum verboseLevels_ {VERBOSE_1 = 0, VERBOSE_2, VERBOSE_3, VERBOSE_4} verboseLevels;
                            // VERBOSE_1:  Operation Progress Displayed on Screen
                            // VERBOSE_2:  low volume debugging
                            // VERBOSE_3:  medium volume debugging
                            // VERBOSE_4:  Detailed debugging
                            //
                            // -v (-vv, -vvv, -vvvv) sets the level, enum values are one less than the level to
                            //                       simplify the conditional test using >

   typedef enum gtErrorType_ {ERRNO_ERROR = 101, CURL_ERROR, HTTP_ERROR, TORRENT_ERROR, DEFAULT_ERROR} gtErrorType;

//   typedef std::map <std::string, vectOfStr> strVectOfStrMap;

   typedef struct activeTorrentRec_
   {
      libtorrent::add_torrent_params torrentParams;
      libtorrent::torrent_handle torrentHandle;
      int expires;
      bool overTimeAlertIssued;
   } activeTorrentRec;

   typedef struct activeSessionRec_
   {
      libtorrent::session *torrentSession;
      std::map <std::string, activeTorrentRec *> mapOfSessionTorrents;
   } activeSessionRec;

   typedef struct childRec_
   {
      int childID;
      FILE *pipeHandle;
      int64_t dataDownloaded;
   } childRec;

   typedef std::vector <std::string> vectOfStr;

   public:
      geneTorrent (int argc, char **argv);
      virtual ~geneTorrent ();

      void run (void);
      bool fileFilter (std::string const filename);
      static bool file_filter (boost::filesystem::path const& filename);

   private:
      //  Command line argument processing and variables
      TCLAP::CmdLine _args;

      std::string _bindIP;
      std::string _exposedIP;
                                      
      int    _portStart;         // based on --internalIP
      int    _portEnd;           // based on --internalIP

      int _exposedPortDelta;
      int _verbosityLevel;

      std::string _manifestFile;
      std::string _uploadUUID;
      std::string _uploadSubmissionURL;
      std::string _dataFilePath;

      vectOfStr _cliArgsDownloadList;
      std::string _downloadSavePath;

      std::string _serverQueuePath;
      std::string _serverDataPath;
      std::string _authToken;
      std::string _serverModeCsrSigningUrl;

      opMode _operatingMode;

      vectOfStr  _filesToUpload;
      int _pieceSize;

      vectOfStr _torrentListToDownload;

      std::list <activeSessionRec *> _activeSessions;
      unsigned int _maxActiveSessions;

      std::string _tmpDir;
      std::string _confDir;
      std::string _dhParamsFile;
      std::string _gtOpenSslConf;

      bool _startUpComplete;

      bool _devMode;  // This flag is used to control behaviors specific specific to development testing.  This is set to true when the environment variable GENETORRENT_DEVMODE is set.

      void bindSession(libtorrent::session &torrentSession);
      void bindSession(libtorrent::session *torrentSession);
      void performTorrentDownload(int64_t);
      void performTorrentUpload(void);
      void processManifestFile(void);

      std::string buildTorrentSymlinks(std::string UUID, std::string fileForTorrent);
      std::string makeTorrent(std::string, std::string);
      std::string getWorkingDirectory(void);
      void performGtoUpload (std::string torrentFileName);
      bool verifyDataFilesExist (vectOfStr &);
      void prepareDownloadList (void);
      void runServerMode(void); 
      void extractURIsFromXML (std::string xmlFileNmae, vectOfStr &urisToDownload);
      //
      void buildURIsToDownloadFromUUID (vectOfStr &uuids);
      void downloadGtoFilesByURI (vectOfStr &uris);

      void getFilesInQueueDirectory (vectOfStr &files);
      std::string sanitizePath (std::string inPath);
      std::string getFileName (std::string fileName);
      void optimizeSession (libtorrent::session *torrentSession);
      void optimizeSession (libtorrent::session &torrentSession);

      static int curlCallBackHeadersWriter (char *data, size_t size, size_t nmemb, std::string *buffer);
      static void loggingCallBack (std::string);

      int statFileOrDirectory (std::string);
      void setPieceSize (void);
      std::string getHttpErrorMessage (int code);

      void gtError (std::string errorMessage, int exitValue, gtErrorType errorType = geneTorrent::DEFAULT_ERROR, long errorCode = 0, std::string errorMessageLine2 = "", std::string errorMessageErrorLine = "");
      void curlCleanupOnFailure (std::string fileName, FILE *gtoFile);
      void validateAndCollectSizeOfTorrents (uint64_t &totalBytes, int &totalFiles, int &totalGtos);
      int64_t getFreeDiskSpace (void);
      libtorrent::session *addActiveSession (void);
      void checkSessions(void);
      geneTorrent::activeSessionRec *findSession (void);
      void findDataAndSetWorkingDirectory (void);

      void deleteGTOfromQueue (std::string fileName);
      void loadCredentialsFile (bool credsSet, std::string credsFile);
      void displayMissingFilesAndExit (vectOfStr &missingFiles);
      std::string  submitTorrentToGTExecutive (std::string torrentFileName);

      void sysLogMessage (std::string message);
      int downloadChild(int, int, std::string, FILE *);
      bool generateCSR (std::string uuid);
      bool generateSSLcertAndGetSigned (std::string torrentFile, std::string signUrl, std::string torrentUUID);
      bool acquireSignedCSR (std::string info_hash, std::string CSRsigningURL, std::string uuid);
      std::string loadCSRfile (std::string csrFileName);
      std::string getInfoHash (std::string torrentFile);
      std::string getInfoHash (libtorrent::torrent_info *torrentInfo);
      void cleanupTmpDir();
      void setTempDir ();
      void mkTempDir ();
      void checkAlerts (libtorrent::session &torrSession);
      void setupSysLog ();

      libtorrent::fingerprint *_gtFingerPrint;

      log4cpp::Appender *_logAppend;
      log4cpp::PatternLayout *_layout;
      log4cpp::Category *sysLogGT;              // a member but not named with _ ...
};

#endif /* GENETORRENT_H_ */
