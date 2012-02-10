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

#include <config.h>

// some constants
const std::string SHORT_DESCRIPTION = "GeneTorrent: Genomic Data Transfer Tool";
const std::string GTO_FILE_EXTENSION = ".gto";
const std::string CONF_DIR_DEFAULT = "/usr/share/GeneTorrent";
const std::string DH_PARAMS_FILE = "dhparam.pem";
const std::string GT_OPENSSL_CONF = "GeneTorrent.openssl.conf";
const std::string PYTHON_TRUE = "TRUE";

const int NO_EXIT = 0;
const int ERROR_NO_EXIT = -1;
const long UNKNOWN_HTTP_HEADER_CODE = 987654321;     // arbitrary number
const int ALERT_CHECK_PAUSE_INTERVAL = 50000;        // in useconds
const int COMMAND_LINE_OR_CONFIG_FILE_ERROR = 9;

const int64_t DISK_FREE_WARN_LEVEL = 1000 * 1000 * 1000;  // 1 GB, aka 10^9

// move to future config file
const std::string GT_CERT_SIGN_TAIL = "gtsession";
const std::string DEFAULT_CGHUB_HOSTNAME = "cghub.ucsc.edu";
const std::string CGHUB_WSI_BASE_URL = "https://"+ DEFAULT_CGHUB_HOSTNAME + "/cghub/data/analysis/";
const std::string DEFAULT_TRACKER_URL = "https://tracker.example.com/announce";

// Work around to disable SSL compression on Centos 5.5
#ifndef SSL_OP_NO_COMPRESSION
#define SSL_OP_NO_COMPRESSION   0x00020000L
#endif

// Command Line Option defines
const char SPACE = ' ';


// information options
const std::string HELP_CLI_OPT = "help";
const std::string VERSION_CLI_OPT = "version";

// general options, shared by all modes
const std::string CONFIG_FILE_CLI_OPT = "config-file";             // NO short option

const std::string BIND_IP_CLI_OPT = "bind-ip";                     // -b short option
const std::string BIND_IP_CLI_OPT_LEGACY = "bindIP";

const std::string CONF_DIR_CLI_OPT = "config-dir";                 // -C short option ( capital C )
const std::string CONF_DIR_CLI_OPT_LEGACY = "confDir";

const std::string CRED_FILE_CLI_OPT = "credential-file";           // -c short option
const std::string CRED_FILE_CLI_OPT_LEGACY = "credentialFile";

const std::string ADVERT_IP_CLI_OPT = "advertised-ip";             // -e short option
const std::string ADVERT_IP_CLI_OPT_LEGACY = "advertisedIP";

const std::string ADVERT_PORT_CLI_OPT = "advertised-port";         // -f short option
const std::string ADVERT_PORT_CLI_OPT_LEGACY = "advertisedPort";

const std::string INTERNAL_PORT_CLI_OPT = "internal-port";         // -i short option
const std::string INTERNAL_PORT_CLI_OPT_LEGACY = "internalPort";

const std::string LOGGING_CLI_OPT = "log";                         // -l short option

//verbosity
const std::string NO_LONG_OPTION = "";                             // -v short option

// download and upload mode options
const std::string PATH_CLI_OPT = "path";                           // -p short option     

// Upload Mode
const std::string UPLOAD_FILE_CLI_OPT = "upload";                  // -u short option
const std::string UPLOAD_FILE_CLI_OPT_LEGACY = "manifestFile";

// Download Mode
const std::string DOWNLOAD_CLI_OPT = "download";                   // -d short option

const std::string MAX_CHILDREN_CLI_OPT = "max-children";           // NO short option
const std::string MAX_CHILDREN_CLI_OPT_LEGACY = "maxChildren";

// Server Mode
const std::string SERVER_CLI_OPT = "server";                       // -s short option

const std::string QUEUE_CLI_OPT = "queue";                         // -q short option

const std::string SECURITY_API_CLI_OPT = "security-api";           // NO short option

// define one free function used in command line and config file process, implementation is in gtMain.cpp
void commandLineError (std::string errMessage);

// This Macro is to be used to display output on the user's screen (in conjunction with the -v option)
// Since logs can be sent to stderr or stdout at the direction of the user, using this macro avoids
// send output messages to log files where users may not see them.
// X is one or more stream manipulters
#define screenOutput(x)               \
{                                     \
   if (_logToStdErr)                  \
   {                                  \
      std::cout << x << std::endl;    \
   }                                  \
   else                               \
   {                                  \
      std::cerr << x << std::endl;    \
   }                                  \
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
