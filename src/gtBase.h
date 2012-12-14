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
 * gtBase.h
 *
 *  Created on: Aug 15, 2011
 *      Author: donavan
 */

#ifndef GT_BASE_H_
#define GT_BASE_H_

#include <pthread.h>

#include <string>
#include <map>
#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

#include <curl/curl.h>

#include "libtorrent/session.hpp"
#include "libtorrent/fingerprint.hpp"
#include "libtorrent/alert_types.hpp"

#include "gtDefs.h"
#include "gtUtils.h"
#include "gtLog.h"
#include "accumulator.hpp"

extern bool global_gtAgentMode;
extern int global_verbosity;

typedef struct attributeEntry_
{
    std::string key;
    std::string value;
}attributeEntry;

class gtBase
{
   public:
      typedef enum opMode_ {DOWNLOAD_MODE = 77, SERVER_MODE, UPLOAD_MODE} opMode;
      typedef enum logLevelValue_ {LOG_STANDARD=10, LOG_VERBOSE, LOG_FULL} logLevelValue;
      typedef enum verboseLevels_ {VERBOSE_1 = 0, VERBOSE_2} verboseLevels;
                            // VERBOSE_1:  Low volume debugging
                            // VERBOSE_2:  Detailed debuggin
                            // -v and -vv (or --verbose=1 or 2) sets the level, enum values are one less than the level to
                            //                       simplify the conditional test using >

      typedef enum gtErrorType_ {ERRNO_ERROR = 101, CURL_ERROR, HTTP_ERROR, TORRENT_ERROR, DEFAULT_ERROR} gtErrorType;

      typedef struct activeTorrentRec_
      {
         libtorrent::add_torrent_params torrentParams;
         libtorrent::torrent_handle torrentHandle;
         time_t expires;            // When the torrent expires
         time_t mtime;              // file modification time
         std::string infoHash;         
         bool overTimeAlertIssued;  // tracks if the overtime message has been reported to syslog
         bool downloadGTO;
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

      typedef std::map<pid_t, childRec *> childMap;

      typedef std::vector <std::string> vectOfStr;

      gtBase (boost::program_options::variables_map &vm, opMode mode,
              std::string progName);
      virtual ~gtBase ();

      static std::string version_str;

      virtual void run () = 0;
      uint32_t getLogMask() {return _logMask;}
      gtLogLevel logLevelFromBool (bool high) {return high? PRIORITY_HIGH : PRIORITY_NORMAL;}
      static std::vector<std::string> vmValueToStrings(boost::program_options::variable_value vv);

   protected:
      int  _verbosityLevel;
      bool _logToStdErr;           // flag to track if logging is being done to stderr, if it is, -v (-vvvv) output is redirected to stdout.
      std::string _authToken;
      bool _devMode;               // This flag is used to control behaviors specific specific to development testing.  This is set to true when the environment variable GENETORRENT_DEVMODE is set.
      std::string _tmpDir;
      std::string _logDestination;
      std::string _dhParamsFile;

      // Null and Zero storage are mutually exclusive. Neither should be used
      // in production.
      bool _use_null_storage;
      bool _use_zero_storage;

      int _portStart;              // based on --internalIP
      int _portEnd;                // based on --internalIP
      int _exposedPortDelta;
      bool _addTimestamps;         // Controls the addition of timestamps to stderr/stdout screen messages.

      long _rateLimit;            // limits data transfer rate of uploads and downloads (if set), this is expressed in libtorrent units of bytes per second
                                  // The conversion from command line argument value to libtorrent value is performed in pcfacliRateLimit()
      int _inactiveTimeout;       // amount of time (in minutes) after which downloads and uploads are terminated due to inactivity
      bool _curlVerifySSL;

      libtorrent::fingerprint *_gtFingerPrint;

      bool _startUpComplete;

      void startUpMessage (std::string app_name);

      void processConfigFileAndCLI (boost::program_options::variables_map &vm);
      void gtError (std::string errorMessage, int exitValue, gtErrorType errorType = gtBase::DEFAULT_ERROR, long errorCode = 0, std::string errorMessageLine2 = "", std::string errorMessageErrorLine = "");
      void checkAlerts (libtorrent::session &torrSession);
      void checkAlerts (libtorrent::session *torrSession);
      void getGtoNameAndInfoHash (libtorrent::torrent_alert *alert, std::string &gtoName, std::string &infoHash);

      libtorrent::session *makeTorrentSession ();
      void bindSession (libtorrent::session &torrentSession);
      void bindSession (libtorrent::session *torrentSession);
      void optimizeSession (libtorrent::session *torrentSession);
      void optimizeSession (libtorrent::session &torrentSession);

      std::string makeTimeStamp ();
      bool generateSSLcertAndGetSigned (std::string torrentFile, std::string signUrl, std::string torrentUUID);

      static int curlCallBackHeadersWriter (char *data, size_t size, size_t nmemb, std::string *buffer);
      bool processCurlResponse (CURL *curl, CURLcode result, std::string fileName, std::string url, std::string uuid, std::string defaultMessage);
      bool processHTTPError (int errorCode, std::string, int exitCode = HTTP_ERROR_EXIT_CODE);
      void curlCleanupOnFailure (std::string fileName, FILE *gtoFile);

      std::string getWorkingDirectory();
#ifdef __CYGWIN__
      std::string getWinInstallDirectory();
#endif /* _CYGWIN_ */

      std::string getFileName (std::string fileName);
      std::string getInfoHash (std::string torrentFile);

      std::string pcfacliPath (boost::program_options::variables_map &vm); // Used by download and upload
      void pcfacliRateLimit (boost::program_options::variables_map &vm);
      void pcfacliInactiveTimeout (boost::program_options::variables_map &vm);
      void checkCredentials ();

      std::string sanitizePath (std::string inPath);
      void relativizePath (std::string &inPath);

      void removeFile (std::string fileName);

      // inactivity timeout functions for upload and download modes
      time_t timeout_update (time_t *timer = NULL);
      bool timeout_check_expired (time_t *timer);

      gtLogLevel makeDebugIfServerModeUnlessError (bool haveError);

   private:
      attributeEntry attributes[CSR_ATTRIBUTE_ENTRY_COUNT];
      std::string _bindIP;
      std::string _exposedIP;
      opMode _operatingMode;
      std::string _confDir;

      uint32_t _logMask;      // bits are used to control which messages classes are logged; bits are number right to left, bit 0-X are for litorrent alerts and bits X-Y are GeneTorrent message classes

      bool _successfulTrackerComms;

      int _peerTimeout;           // peer timeout in seconds for libtorrent session settings

      static void loggingCallBack (std::string);

      std::string getHttpErrorMessage (int code);

      bool generateCSR (std::string uuid);
      bool acquireSignedCSR (std::string info_hash, std::string CSRsigningURL, std::string uuid);
      void processSSLError (std::string message);
      void initSSLattributes ();
      std::string loadCSRfile (std::string csrFileName);
      std::string getInfoHash (libtorrent::torrent_info *torrentInfo);
      std::string authTokenFromURI (std::string url);

      void cleanupTmpDir();
      void setTempDir ();
      void mkTempDir ();
 
      void processUnimplementedAlert (bool haveError, libtorrent::alert *alrt);
      void processPeerNotification (bool haveError, libtorrent::alert *alrt);
      void processDebugNotification (bool haveError, libtorrent::alert *alrt);
      void processStorageNotification(bool, libtorrent::alert*);
      void processStatNotification(bool, libtorrent::alert*);
      void processPerformanceWarning(bool, libtorrent::alert*);
      void processIpBlockNotification(bool, libtorrent::alert*);
      void processProgressNotification(bool, libtorrent::alert*);
      void processTrackerNotification(bool, libtorrent::alert*);
      void processStatusNotification(bool, libtorrent::alert*);

      // Process command config file and command line interface
      void pcfacliBindIP (boost::program_options::variables_map &vm);
      void pcfacliConfDir (boost::program_options::variables_map &vm);
      void pcfacliCredentialFile (boost::program_options::variables_map &vm);
      void pcfacliAdvertisedIP (boost::program_options::variables_map &vm);
      void pcfacliInternalPort (boost::program_options::variables_map &vm);
      void pcfacliAdvertisedPort (boost::program_options::variables_map &vm);
      void pcfacliLog (boost::program_options::variables_map &vm);
      void pcfacliTimestamps (boost::program_options::variables_map &vm);
      void pcfacliGTAgentMode (boost::program_options::variables_map &vm);
      void pcfacliCurlNoVerifySSL (boost::program_options::variables_map &vm);
      void pcfacliStorageFlags (boost::program_options::variables_map &vm);
      void pcfacliPeerTimeout (boost::program_options::variables_map &vm);

      void log_options_used (boost::program_options::variables_map &vm);

};
#endif /* GT_BASE_H_ */
