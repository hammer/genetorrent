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
 * gtDefs.h
 *
 *  Created on: Aug 15, 2011
 *      Author: donavan
 */

#ifndef GTDEFS_H_
#define GTDEFS_H_

// some constants
const std::string SHORT_DESCRIPTION = "GeneTorrent: Genomic Data Transfer Tool";
const std::string GTO_FILE_BUILDING_EXTENSION = ".gto-build";
const std::string GTO_FILE_DOWNLOAD_EXTENSION = "~";
const std::string GTO_ERROR_DOWNLOAD_EXTENSION = "-error";
const std::string GTO_FILE_EXTENSION = ".gto";
const std::string RESUME_FILE_EXT = ".resume";
const std::string PROGRESS_FILE_EXT = ".progress";

const std::string CONF_DIR_DEFAULT = "/usr/share/GeneTorrent";
const std::string DH_PARAMS_FILE = "dhparam.pem";
const std::string PYTHON_TRUE = "TRUE";

const int NO_EXIT = 0;
const int ERROR_NO_EXIT = -1;
const long UNKNOWN_HTTP_HEADER_CODE = 987654321;     // arbitrary number
const int ALERT_CHECK_PAUSE_INTERVAL = 50000;        // in useconds
const int COMMAND_LINE_OR_CONFIG_FILE_ERROR = 9;
const int HTTP_ERROR_EXIT_CODE = 10;

const int64_t DISK_FREE_WARN_LEVEL = 1000 * 1000 * 1000;  // 1 GB, aka 10^9

const unsigned long PROCESS_MIN = 4096; // preferred minimum user NPROC soft limit for download mode

// move to future config file
const std::string GT_CERT_SIGN_TAIL = "gtsession";
const std::string DEFAULT_CGHUB_HOSTNAME = "cghub.ucsc.edu";
const std::string CGHUB_WSI_BASE_URL = "https://"+ DEFAULT_CGHUB_HOSTNAME + "/cghub/data/analysis/";
const std::string DEFAULT_TRACKER_URL = "https://tracker.example.com/announce";

// Work around to disable SSL compression on Centos 5.5
#ifndef SSL_OP_NO_COMPRESSION
#define SSL_OP_NO_COMPRESSION   0x00020000L
#endif

// OpenSSL private key and CSR defaults
const int RSA_KEY_SIZE = 1024;
const int SSL_ERROR_EXIT_CODE = 237;
const int CSR_ATTRIBUTE_ENTRY_COUNT = 7;

// Command Line Option defines
const char SPACE = ' ';

// information options
const std::string HELP_CLI_OPT = "help";
const std::string VERSION_CLI_OPT = "version";

// general options, shared by all modes
const std::string CONFIG_FILE_CLI_OPT = "config-file";             // NO short option

const std::string BIND_IP_CLI_OPT = "bind-ip";                     // -b short option
const std::string BIND_IP_CLI_OPT_LEGACY = "bindIP";

const char CONF_DIR_SHORT_CLI_OPT = 'C';                           // -C short option ( capital C )
const std::string CONF_DIR_CLI_OPT = "config-dir";                 // -C short option ( capital C )
const std::string CONF_DIR_CLI_OPT_LEGACY = "confDir";

const std::string CRED_FILE_CLI_OPT = "credential-file";           // -c short option
const std::string CRED_FILE_CLI_OPT_LEGACY = "credentialFile";

const char ADVERT_IP_SHORT_CLI_OPT = 'e';                          // -e short option
const std::string ADVERT_IP_CLI_OPT = "advertised-ip";             
const std::string ADVERT_IP_CLI_OPT_LEGACY = "advertisedIP";

const char ADVERT_PORT_SHORT_CLI_OPT = 'f';                        // -f short option
const std::string ADVERT_PORT_CLI_OPT = "advertised-port";         
const std::string ADVERT_PORT_CLI_OPT_LEGACY = "advertisedPort";

const std::string INTERNAL_PORT_CLI_OPT = "internal-port";         // -i short option
const std::string INTERNAL_PORT_CLI_OPT_LEGACY = "internalPort";

const std::string LOGGING_CLI_OPT = "log";                         // -l short option

const std::string TIMESTAMP_STD_CLI_OPT = "timestamps";            // -t short option

const std::string NO_LONG_CLI_OPT = "";                            // 

const std::string CURL_NO_VERIFY_SSL_CLI_OPT = "ssl-no-verify-ca";         // NO short option

const std::string POSITIONAL_CLI_OPT = "positional";               // NO short option

// verbosity conversation, two different verbose settings are available
// Single letter -v with increasing count or --verbose=level
// this is a rather ugly hack, but due to shortcomings in boost program_options
// seems to be a workable solution
// -v and --verbose are independent options due to boost program_options, but work
// together.  short options are not permitted in config files.
//   verbose       verbose       -v       Result
//   in config                            config value
//                 on CLI                 cli value
//                             on CLI     -v cli count
//   in config     on CLI                 cli value
//   in config                 on CLI     -v cli count
//                 on CLI      on CLI     error
//   in config     on CLI      on CLI     error
//
// short options are hard coded (where needed) in gtMain.cpp
const char VERBOSITY_SHORT_CLI_OPT = 'v';                         // -v short option
const std::string VERBOSITY_CLI_OPT = "verbose";                 

// download and upload mode options
const std::string PATH_CLI_OPT = "path";                           // -p short option     
const std::string RATE_LIMIT_CLI_OPT = "rate-limit";               // -r short option     

const std::string GTA_CLIENT_CLI_OPT = "gta";                      // no short option

const char INACTIVE_TIMEOUT_SHORT_CLI_OPT = 'k';                   // -k short option
const std::string INACTIVE_TIMEOUT_CLI_OPT = "inactivity-timeout";

// Upload Mode
const std::string UPLOAD_FILE_CLI_OPT = "upload";                  // -u short option
const std::string UPLOAD_FILE_CLI_OPT_LEGACY = "manifestFile";

const std::string UPLOAD_GTO_PATH_CLI_OPT = "upload-gto-path";     // no short option
const std::string UPLOAD_GTO_ONLY_CLI_OPT = "gto-only";            // no short option

// Download Mode
const std::string DOWNLOAD_CLI_OPT = "download";                   // -d short option

const std::string MAX_CHILDREN_CLI_OPT = "max-children";           // NO short option
const std::string MAX_CHILDREN_CLI_OPT_LEGACY = "maxChildren";

// Server Mode
const std::string SERVER_CLI_OPT = "server";                       // -s short option

const std::string QUEUE_CLI_OPT = "queue";                         // -q short option

const std::string SECURITY_API_CLI_OPT = "security-api";           // NO short option

const std::string SERVER_FORCE_DOWNLOAD_OPT = "force-download-mode";           // NO short option
// Storage Options
const std::string NULL_STORAGE_OPT = "null-storage";               // NO short option
const std::string ZERO_STORAGE_OPT = "zero-storage";               // NO short option

const std::string PEER_TIMEOUT_OPT = "peer-timeout";               // NO short option

// define one free function used in command line and config file process, implementation is in gtMain.cpp
void commandLineError (std::string errMessage);

// This Macro is to be used to display output on the user's screen (in conjunction with the -v option)
// Since logs can be sent to stderr or stdout at the direction of the user, using this macro avoids
// send output messages to log files where users may not see them.
// X is one or more stream manipulters
#define screenOutput(x)                                    \
{                                                          \
   std::string timeStamp = "";                             \
   if (_addTimestamps)                                     \
   {                                                       \
      timeStamp = makeTimeStamp () + + " ";                \
   }                                                       \
   if (_logToStdErr)                                       \
   {                                                       \
      std::cout << timeStamp << x << std::endl;            \
      std::cout.flush ();                                  \
   }                                                       \
   else                                                    \
   {                                                       \
      if (global_gtAgentMode)                              \
      {                                                    \
         std::cout << timeStamp << x << std::endl;         \
         std::cout.flush ();                               \
      }                                                    \
      else                                                 \
      {                                                    \
         std::cerr << timeStamp << x << std::endl;         \
         std::cerr.flush ();                               \
      }                                                    \
   }                                                       \
}

#define screenOutputNoNewLine(x)                           \
{                                                          \
   std::string timeStamp = "";                             \
   if (_addTimestamps)                                     \
   {                                                       \
      timeStamp = makeTimeStamp () + + " ";                \
   }                                                       \
   if (_logToStdErr)                                       \
   {                                                       \
      std::cout << timeStamp << x;                         \
   }                                                       \
   else                                                    \
   {                                                       \
      std::cerr << timeStamp << x;                         \
   }                                                       \
}

// TODO figure out if these are saying in the subclass or this can be 
// refactored cleanly into something that doesn't throw compiler warnings

// Torrent status text for various GeneTorrent operational modes
//
// these are indexed by the torrent_status::state_t enum, found in torrent_handle.hpp

/*
static char const* server_state_str[] = { 
   "checking (q)",                   //         queued_for_checking,
   "checking",                       //         checking_files,
   "dl metadata",                    //         downloading_metadata,
   "receiving",                      //         downloading,
   "finished",                       //         finished,
   "serving",                        //         seeding,
   "allocating",                     //         allocating,
   "checking (r)"                    //         checking_resume_data
};

static char const* download_state_str[] = {
   "checking (q)",                   //         queued_for_checking,
   "checking",                       //         checking_files,
   "dl metadata",                    //         downloading_metadata,
   "downloading",                    //         downloading,
   "finished",                       //         finished,
   "cleanup",                        //         seeding,
   "allocating",                     //         allocating,
   "checking (r)"                    //         checking_resume_data
};

static char const* upload_state_str[] = { 
   "checking (q)",                   //         queued_for_checking,
   "checking",                       //         checking_files,
   "dl metadata",                    //         downloading_metadata,
   "starting",                       //         downloading,
   "finished",                       //         finished,
   "uploading",                      //         seeding,
   "allocating",                     //         allocating,
   "checking (r)"                    //         checking_resume_data
};

*/

#endif /* GTDEFS_H_ */
