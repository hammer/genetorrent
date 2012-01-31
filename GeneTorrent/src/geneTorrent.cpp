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
 * geneTorrent.cpp
 *
 *  Created on: Aug 15, 2011
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

#include "geneTorrent.h"
#include "stringTokenizer.h"
#include "gtDefs.h"
#include "tclapOutput.h"
#include "geneTorrentUtils.h"
#include "gtLog.h"
#include "loggingmask.h"

// global variable that used to point to GeneTorrent to allow
// libtorrent callback for file inclusion and logging.
// initialized in geneTorrent constructor, used in the free
// function file_filter to call member fileFilter.
void *geneTorrCallBackPtr; 

// Lock to prevent the callback logger from clearing a buffer
// at the saem time another thread is trying to add to a buffer
static pthread_mutex_t callBackLoggerLock;

geneTorrent::geneTorrent (int argc, char **argv) : 
   _args (SHORT_DESCRIPTION, ' ', VERSION), 
   _bindIP (""), 
   _exposedIP (""), 
   _portStart (20892), 
   _portEnd (20900), 
   _exposedPortDelta (0), 
   _verbosityLevel (0), 
   _manifestFile (""), 
   _uploadUUID (""), 
   _uploadSubmissionURL (""), 
   _dataFilePath (""), 
   _cliArgsDownloadList (), 
   _downloadSavePath (""), 
   _maxChildren (8),
   _serverQueuePath (""), 
   _serverDataPath (""), 
   _authToken (""), 
   _operatingMode (UPLOAD_MODE), 
   _filesToUpload (), 
   _pieceSize (4194304), 
   _torrentListToDownload (), 
   _activeSessions (), 
   _tmpDir (""), 
   _confDir (CONF_DIR_DEFAULT), 
   _logDestination ("none"),     // default to no logging
   _logMask (0),                 // set all bits to 0
   _logToStdErr (false),
   _startUpComplete (false), 
   _devMode (false) 
{
   geneTorrCallBackPtr = (void *) this;          // Set the global geneTorr pointer that allows fileFilter callbacks from libtorrent

   pthread_mutex_init (&callBackLoggerLock, NULL);

   std::ostringstream startUpMessage;
   startUpMessage << "Starting version " << VERSION << " with the command line: ";

   for (int loop = 1; loop < argc; loop++)
   {
      startUpMessage << " " << argv[loop];
   }

   char *envValue = getenv ("GENETORRENT_DEVMODE");
   if (envValue != NULL)
   {
      _tmpDir = sanitizePath (envValue) + "/";
      _devMode = true;
   }
   else
   {
      setTempDir();
   }

   // General purpose command line arguments.
   // Bind IP address (on local machine), upload, download, cghub server
   TCLAP::ValueArg <std::string> bindIP ("b", "bindIP", "Description Not Used", false, "", "string");
   _args.add (bindIP);

   // configuration directory
   TCLAP::ValueArg <std::string> confDir ("C", "confDir", "Description Not Used", false, "", "string");
   _args.add (confDir);

   // Exposed IP address (of local machine), upload, download, cghub server
   TCLAP::ValueArg <std::string> exposedIP ("e", "advertisedIP", "Description Not Used", false, "", "string");
   _args.add (exposedIP);

   // Exposed ports (of local machine), upload, download, cghub server
   TCLAP::ValueArg <int> exposedPort ("f", "advertisedPort", "Description Not Used", false, 0, "int");
   _args.add (exposedPort);

   // Internal ports (of local machine), upload, download, cghub server
   TCLAP::ValueArg <std::string> internalPort ("i", "internalPorts", "Description Not Used", false, "", "string");
   _args.add (internalPort);

   // Logging option, 3 fields destination:level:mask
   TCLAP::ValueArg <std::string> logging ("l", "log", "Description Not Used", false, "", "string");
   _args.add (logging);

   // Generic path used by upload, download, cghub server
   TCLAP::ValueArg <std::string> genericPath ("p", "path", "Description Not Used", false, "", "string");
   _args.add (genericPath);

   // Verbosity
   TCLAP::MultiSwitchArg verbosity ("v", "verbose", "Description Not Used", false);
   _args.add (verbosity);

// Download mode
   // Credential File
   TCLAP::ValueArg <std::string> credentialFile ("c", "credentialFile", "Description Not Used", false, "", "string"); // this implies download mode
   _args.add (credentialFile);

   // download flag and repeatable argument indicating download items
   TCLAP::MultiArg <std::string> downloadList ("d", "download", "Description Not Used", false, "string"); // this implies download mode
   _args.add (downloadList);

   // Max number of children for download
   TCLAP::ValueArg <int> maxDownloadChildren ("", "maxChildren", "Description Not Used", false, 8, "int");
    _args.add (maxDownloadChildren);

// Upload mode 
   //
   TCLAP::ValueArg <std::string> manifestFN ("u", "manifestFile", "Description Not Used", false, "", "string"); // This implies upload mode
   _args.add (manifestFN);

// Seeder mode
   // path to queue directory
   TCLAP::ValueArg <std::string> serverQueuePath ("q", "queue", "Description Not Used", false, "", "string");
   _args.add (serverQueuePath);

   // server mode flag and path to directory containing UUID directories
   TCLAP::ValueArg <std::string> serverDataPath ("s", "server", "Description Not Used", false, "", "string");
   _args.add (serverDataPath);

   // URL to use for signing CSR in server mode
   TCLAP::ValueArg <std::string> serverModeCsrSigningUrl ("", "security-api", "Description Not Used", false, "", "string");
   _args.add (serverModeCsrSigningUrl);

   tclapOutput outputOverride;

   try
   {
      _args.setOutput (&outputOverride); // Setup our custom help page

      _args.parse (argc, argv); // Parse the command line

      // serverDataPath -> server mode, manifestFN -> upload mode, downloadList -> download mode
      // Verify only one mode is selected on the CLI
      if ((serverDataPath.isSet() && manifestFN.isSet()) || (serverDataPath.isSet() && downloadList.isSet()) || (manifestFN.isSet() && downloadList.isSet()))
      {
         TCLAP::ArgException argError ("Command line may only specifiy one of -d, -s, or -u", "");
         throw(argError);
      }

      // Process the General Args
      if (bindIP.isSet ())
      {
         _bindIP = bindIP.getValue ();
      }

      if (exposedIP.isSet ())
      {
         _exposedIP = exposedIP.getValue ();
      }

      if (internalPort.isSet ())
      {
         strTokenize strToken (internalPort.getValue (), ":", strTokenize::MERGE_CONSECUTIVE_SEPARATORS);

         int lowPort = strtol (strToken.getToken (1).c_str (), NULL, 10);
         int highPort = -1;

         if (strToken.size () > 1)
         {
            highPort = strtol (strToken.getToken (2).c_str (), NULL, 10);
         }

         if (highPort > 0)
         {
            if (lowPort <= highPort)
            {
               _portStart = lowPort;
               _portEnd = highPort;
            }
            else
            {
               TCLAP::ArgException argError ("when using -i (--internalPorts) string1 must be smaller than string2", "");
               throw(argError);
            }
         }
         else
         {
            _portStart = lowPort;
            _portEnd = _portStart + 8; // default 8 ports
         }
      }

      _maxActiveSessions = _portEnd - _portStart + 1;

      if (exposedPort.isSet ())
      {
         _exposedPortDelta = exposedPort.getValue () - _portStart;
      }

      // Generic path (option -p) is collected when needed

      // configuration directory
      if (confDir.isSet())
      {
         _confDir = sanitizePath (confDir.getValue ());
         if ((_confDir.size () == 0) || (_confDir[0] != '/'))
         {
             TCLAP::ArgException argError ("configuration directory must be an absolute path: \"" + _confDir + "\"", "");
             throw(argError);
         }

         if (statFileOrDirectory (_confDir) != 0)
         {
            gtError ("Failure opening configuration path:  " + _confDir, 202, ERRNO_ERROR, errno);
         }
      }

      if (logging.isSet())
      {
         strTokenize strToken (logging.getValue (), ":", strTokenize::MERGE_CONSECUTIVE_SEPARATORS);

         _logDestination = strToken.getToken(1);

         std::string level = strToken.getToken(2);

         if ("verbose" == level)
         {
            _logMask  = LOGMASK_VERBOSE;
         }
         else if ("full" == level)
         {
            _logMask  = LOGMASK_FULL;
         }
         else if ("standard" == level || level.size() == 0) // default to standard
         {
            _logMask  = LOGMASK_STANDARD;
         }
         else
         {
            _logMask = strtoul (level.c_str(), NULL, 0);
         }
      }

      _logToStdErr = gtLogger::create_globallog (PACKAGE_NAME, _logDestination);

      Log (PRIORITY_NORMAL, "%s (using tmpDir = %s)", startUpMessage.str().c_str(), _tmpDir.c_str());

      if (verbosity.isSet ())
      {
         _verbosityLevel = verbosity.getValue ();
      }

      if (manifestFN.isSet ()) // upload mode
      {
         _manifestFile = manifestFN.getValue ();

         if (statFileOrDirectory (_manifestFile) != 0)
         {
            gtError ("Failure opening " + _manifestFile + " for input.", 202, ERRNO_ERROR, errno);
         }

         _dataFilePath = sanitizePath (genericPath.getValue ()); // default is ""

         if (_dataFilePath.size () > 0)
         {
            if (statFileOrDirectory (_dataFilePath) != 0)
            {
               gtError ("Failure opening data path:  " + _dataFilePath, 202, ERRNO_ERROR, errno);
            }
         }
         loadCredentialsFile (credentialFile.isSet (), credentialFile.getValue ());
         _operatingMode = UPLOAD_MODE;
      }
      else if (downloadList.isSet ()) // download mode
      {
         _cliArgsDownloadList = downloadList.getValue ();

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
            loadCredentialsFile (credentialFile.isSet (), credentialFile.getValue ());
         }

         if (maxDownloadChildren.isSet ())
         {
            _maxChildren = maxDownloadChildren.getValue();
         }

         _downloadSavePath = sanitizePath (genericPath.getValue ()); // default is ""

         if (_downloadSavePath.size () > 0)
         {
            if (statFileOrDirectory (_downloadSavePath) != 0)
            {
               gtError ("Failure accessing download save path:  " + _downloadSavePath, 202, ERRNO_ERROR, errno);
            }
         }
         _operatingMode = DOWNLOAD_MODE;
      }
      else if (serverDataPath.isSet ()) // server  mode
      {
         _serverDataPath = sanitizePath (serverDataPath.getValue ());

         if (statFileOrDirectory (_serverDataPath) != 0)
         {
            gtError ("Failure opening server data path:  " + _serverDataPath, 202, ERRNO_ERROR, errno);
         }

         if (!serverQueuePath.isSet ())
         {
            TCLAP::ArgException argError ("Must include a queue path when operating in server mode", "");
            throw(argError);
         }

         _serverQueuePath = sanitizePath (serverQueuePath.getValue ());

         if (statFileOrDirectory (_serverQueuePath) != 0)
         {
            gtError ("Failure opening server queue path:  " + _serverQueuePath, 202, ERRNO_ERROR, errno);
         }

         if (!serverModeCsrSigningUrl.isSet())
         {
            TCLAP::ArgException argError ("--security-api missing, Must include a full URL to the security API services", "");
            throw(argError);
         }

         _serverModeCsrSigningUrl = serverModeCsrSigningUrl.getValue();

         loadCredentialsFile (credentialFile.isSet (), credentialFile.getValue ());
         _operatingMode = SERVER_MODE;
      }
      else
      {
         TCLAP::ArgException argError ("No operational mode detected, must include one of -u, -d, or -s when starting.", "");
         throw(argError);
      }
   }
   catch (TCLAP::ArgException &e) // catch any exceptions
   {
      outputOverride.failure (_args, e);
   }

   _dhParamsFile = _confDir + "/" + DH_PARAMS_FILE;
   _gtOpenSslConf = _confDir + "/" + GT_OPENSSL_CONF;

   if (statFileOrDirectory (_dhParamsFile) != 0)
   {
      gtError ("Failure opening SSL DH Params file:  " + _dhParamsFile, 202, ERRNO_ERROR, errno);
   }

   if (statFileOrDirectory (_gtOpenSslConf) != 0)
   {
      gtError ("Failure opening OPENSSL config file:  " + _gtOpenSslConf, 202, ERRNO_ERROR, errno);
   }

   if (!_devMode)
   {
      mkTempDir();
   }

   std::string gtTag;
   if (_operatingMode != SERVER_MODE)
   {
      gtTag = "GT";
   }
   else
   {
      gtTag = "Gt";
   }   

   strTokenize strToken (VERSION, ".", strTokenize::INDIVIDUAL_CONSECUTIVE_SEPARATORS);

   _gtFingerPrint = new libtorrent::fingerprint (gtTag.c_str(), strtol (strToken.getToken (1).c_str (), NULL, 10), strtol (strToken.getToken (2).c_str (), NULL, 10), strtol (strToken.getToken (3).c_str (), NULL, 10), strtol (strToken.getToken (4).c_str (), NULL, 10));

   _startUpComplete = true;
}

// 
void geneTorrent::setTempDir ()     
{
   // Setup the static part of the Temp dir used to store ssl bits
   std::ostringstream pathPart;
   pathPart << "/GeneTorrent-" << getuid() << "-" << std::setfill('0') << std::setw(6) << getpid() << "-" << time(NULL);

   char *envValue = getenv ("TMPDIR");

   if (envValue != NULL)
   {
      _tmpDir = sanitizePath(envValue) + pathPart.str();
   }
   else
   {
      _tmpDir = "/tmp" + pathPart.str();
   }
}

// 
void geneTorrent::mkTempDir () 
{
   int retValue = mkdir (_tmpDir.c_str(), 0700);       
   if (retValue != 0 )
   {
      gtError ("Failure creating temporary directory " + _tmpDir, 202, ERRNO_ERROR, errno);
   }

   _tmpDir += "/";
}

// 
void geneTorrent::loadCredentialsFile (bool credsSet, std::string credsFile)
{
   if (!credsSet)
   {
      TCLAP::ArgException argError ("Must include a credential file when attempting to communicate with CGHub", "-c  (--credentialFile)");
      throw(argError);
   }

   std::ifstream credFile;

   credFile.open (credsFile.c_str (), std::ifstream::in);

   if (!credFile.good ())
   {
      TCLAP::ArgException argError ("file not found (or is not readable):  " + credsFile, "");
      throw(argError);
   }

   credFile >> _authToken;

   credFile.close ();
}

// 
std::string geneTorrent::loadCSRfile (std::string csrFileName)
{
   std::string fileContent = "";
   
   if (statFileOrDirectory (csrFileName) != 0)
   {
      if (_operatingMode != SERVER_MODE)
      {
         gtError ("Failure opening " + csrFileName + " for input.", 202, ERRNO_ERROR, errno);
      }
      else
      {
         gtError ("Failure opening " + csrFileName + " for input.", ERROR_NO_EXIT, ERRNO_ERROR, errno);
         return "";
      }
   }

   std::ifstream csrFile;

   csrFile.open (csrFileName.c_str (), std::ifstream::in);

   if (!csrFile.good ())
   {
      if (_operatingMode != SERVER_MODE)
      {
         gtError ("Failure opening " + csrFileName + " for input.", 202, ERRNO_ERROR, errno);
      }
      else
      {
         gtError ("Failure opening " + csrFileName + " for input.", ERROR_NO_EXIT, ERRNO_ERROR, errno);
         return "";
      }
   }

   char inLine[100];

   while (csrFile.good())
   {
      std::istream &in = csrFile.getline (inLine, sizeof (inLine)-1);

      if (in.gcount() <= 0 || csrFile.eof())
      {
         break;
      }

      inLine[in.gcount()] = '\0';

      fileContent += std::string (inLine) + "\n";
   }

   csrFile.close ();

   return fileContent;
}

// 
std::string geneTorrent::sanitizePath (std::string inPath)
{
   if (inPath[inPath.size () - 1] == '/')
   {
      inPath.erase (inPath.size () - 1);
   }

   return (inPath);
}

// 
geneTorrent::~geneTorrent ()
{
}

// 
void geneTorrent::gtError (std::string errorMessage, int exitValue, gtErrorType errorType, long errorCode, std::string errorMessageLine2, std::string errorMessageErrorLine)
{
   std::ostringstream logMessage;

   if (exitValue == NO_EXIT)      // exitValue = 0
   {
      logMessage << "Warning:  " << errorMessage;
   }
   else  // exitValue or ERROR_NO_EXIT = -1, the caller handles the error and this permits syslogging only
   {
      logMessage << errorMessage;
   }

   if (errorMessageLine2.size () > 0)
   {
      logMessage << ", " << errorMessageLine2;
   }

   switch (errorType)
   {
      case geneTorrent::HTTP_ERROR:
      {
         logMessage << "  Additional Info:  " << getHttpErrorMessage (errorCode) << " (HTTP status code = " << errorCode << ").";
      }
         break;

      case geneTorrent::CURL_ERROR:
      {
         logMessage << "  Additional Info:  " << curl_easy_strerror (CURLcode (errorCode)) << " (curl code = " << errorCode << ").";
      }
         break;

      case geneTorrent::ERRNO_ERROR:
      {
         logMessage << "  Additional Info:  " << strerror (errorCode) << " (errno = " << errorCode << ").";
      }
         break;

      case geneTorrent::TORRENT_ERROR:
      {
         logMessage << "  Additional Info:  " << errorMessageErrorLine << " (GT code = " << errorCode << ").";
      }
         break;

      default:
      {
         if (errorMessageErrorLine.size () > 0)
         {
            logMessage << ", " << errorMessageErrorLine;
         }
      }
         break;
   }

   if (exitValue == ERROR_NO_EXIT)   // ERRROR in server mode
   {
      Log (PRIORITY_HIGH, "%s", logMessage.str().c_str());
   }
   else if (exitValue == NO_EXIT)    // Warning message
   {
      Log (PRIORITY_NORMAL, "%s", logMessage.str().c_str());
   }
   else                              // Error and Exit
   {
      Log (PRIORITY_HIGH, "%s", logMessage.str().c_str());

      if (_startUpComplete)
      {
         cleanupTmpDir();
      }
      exit (exitValue);
   }
}

// 
std::string geneTorrent::getHttpErrorMessage (int code)
{
   switch (code)
   {
      case 505:
      {
         return "HTTP Version Not Supported";
      } break;

      case 504:
      {
         return "Gateway Timeout";
      } break;

      case 503:
      {
         return "Service Unavailable";
      } break;

      case 502:
      {
         return "Bad Gateway";
      } break;

      case 501:
      {
         return "Not Implemented";
      } break;

      case 500:
      {
         return "Internal Server Error";
      } break;

      case 400:
      {
         return "Bad Request";
      } break;

      case 401:
      {
         return "Unauthorized";
      } break;

      case 403:
      {
         return "Forbidden";
      } break;

      case 404:
      {
         return "Not Found";
      } break;

      case 409:
      {
         return "Request Timeout";
      } break;

      case 411:
      {
         return "Gone";
      } break;

      case 412:
      {
         return "Length Required";
      } break;

      default:
      {
         return "Unknown, research given code";
      } break;
   }

   return "Unknown, research given code";
}

// 
int geneTorrent::curlCallBackHeadersWriter (char *data, size_t size, size_t nmemb, std::string *buffer)
{
   int result = 0; // What we will return

   if (buffer != NULL) // Is there anything in the buffer?
   {
      buffer->append (data, size * nmemb); // Append the data to the buffer

      result = size * nmemb; // How much did we write?
   }

   return result;
}

// 
void geneTorrent::run ()
{
   std::string saveDir = getWorkingDirectory ();

   switch (_operatingMode)
   {
      case DOWNLOAD_MODE:
      {
         if (_verbosityLevel > 0)
         {
            screenOutput ("Welcome to GeneTorrent version " << VERSION << ", download mode."); 
         }
         runDownloadMode (saveDir);
         chdir (saveDir.c_str ());       // shutting down, if the chdir back fails, so be it

      } break;

      case SERVER_MODE:
      {
         if (_verbosityLevel > 0)
         {
            screenOutput ("Welcome to GeneTorrent version " << VERSION << ", server mode."); 
         }
         runServerMode ();
      } break;

      default: // UPLOAD_MODE
      {
         if (_verbosityLevel > 0)
         {
            screenOutput ("Welcome to GeneTorrent version " << VERSION << ", upload mode."); 
         }

         performTorrentUpload ();
         chdir (saveDir.c_str ());      // shutting down, if the chdir back fails, so be it
      } break;
   }

   cleanupTmpDir();
}

// 
void geneTorrent::cleanupTmpDir()
{
   if (!_devMode)
   {
      system (("rm -rf " + _tmpDir.substr(0,_tmpDir.size()-1)).c_str());
   }
}

// 
void geneTorrent::bindSession (libtorrent::session *torrentSession)
{
   // the difference between the actual port used and the port to be advertised to the tracker
   if (_exposedPortDelta != 0) 
   {
      libtorrent::session_settings settings = torrentSession->settings ();
      settings.external_port_delta = _exposedPortDelta;
      torrentSession->set_settings (settings);
   }

   libtorrent::session_settings settings = torrentSession->settings ();

   // update the IP we bind on.  
   //
   // TODO: the listen_on function is deprecated in libtorrent, so upgrade to the newer method of doing this
   if (_bindIP.size () > 0)
   {
      torrentSession->listen_on (std::make_pair (_portStart, _portEnd), _bindIP.c_str (), 0);
   }
   else
   {
      torrentSession->listen_on (std::make_pair (_portStart, _portEnd), NULL, 0);
   }

   // the ip address sent to the tracker with the announce
   if (_exposedIP.size () > 0) 
   {
      settings.announce_ip = _exposedIP;
   }

   // when a connection fails, retry it more quickly than the libtorrent default.  Note that this is multiplied by the attempt
   // number, so the delay before the 2nd attempt is (min_reconnect_time * 2) and before the 3rd attempt is *3, and so on...
   settings.min_reconnect_time = 3;

   // This is the minimum interval that we will restatus the tracker.
   // The tracker itself tells us (via the interval parameter) when we
   // should restatus it.  However, if the value sent by the tracker
   // is less than the value below, then libtorrent follows the value
   // below.  So, setting this value low allows us to respond quickly
   // in the case where the tracker sends back a very low interval
   settings.min_announce_interval = 2;

    // prevent keepalives from being sent.  this will cause servers to
    // time out peers more rapidly.  
    //
    // TODO: probably a good idea to set this in ALL modes, but for now
    // we don't want to risk introducing a new bug to server mode
   if (_operatingMode != SERVER_MODE)
   { 
      settings.inhibit_keepalives = true;
   }
   torrentSession->set_settings (settings);
}

// 
void geneTorrent::bindSession (libtorrent::session &torrentSession)
{
   bindSession (&torrentSession);
}

// 
void geneTorrent::optimizeSession (libtorrent::session *torrentSession)
{
   libtorrent::session_settings settings = torrentSession->settings ();

   settings.allow_multiple_connections_per_ip = true;
#ifdef TORRENT_CALLBACK_LOGGER
   settings.loggingCallBack = &geneTorrent::loggingCallBack;
#endif
   settings.max_allowed_in_request_queue = 1000;
   settings.max_out_request_queue = 1000;
   settings.mixed_mode_algorithm = libtorrent::session_settings::prefer_tcp;
   settings.enable_outgoing_utp = false;
   settings.enable_incoming_utp = false;
   settings.apply_ip_filter_to_trackers= false;

   settings.no_atime_storage = true;
   settings.max_queued_disk_bytes = 1024 * 1024 * 1024;

   torrentSession->set_settings (settings);

   if (_bindIP.size() || _exposedIP.size())
   {
      libtorrent::ip_filter ipFilter;

      if (_bindIP.size())
      {
         ipFilter.add_rule (boost::asio::ip::address::from_string(_bindIP), boost::asio::ip::address::from_string(_bindIP), libtorrent::ip_filter::blocked);
      }

      if (_exposedIP.size())
      {
         ipFilter.add_rule (boost::asio::ip::address::from_string(_exposedIP), boost::asio::ip::address::from_string(_exposedIP), libtorrent::ip_filter::blocked);
      }

      torrentSession->set_ip_filter(ipFilter);
   }
}

// 
void geneTorrent::optimizeSession (libtorrent::session &torrentSession)
{
   optimizeSession (&torrentSession);
}

// 
void geneTorrent::loggingCallBack (std::string message)
{
   pthread_mutex_lock (&callBackLoggerLock);

   static std::ostringstream messageBuff;
   messageBuff << message;

   if (std::string::npos != message.find ('\n'))
   {
      std::string logMessage = messageBuff.str().substr(0, messageBuff.str().size() - 1);
      messageBuff.str("");
      pthread_mutex_unlock (&callBackLoggerLock);

      boost::regex searchPattern ("[0-9][0-9]:[0-9][0-9]:[0-9][0-9].[0-9][0-9][0-9]");

      if (regex_search (logMessage, searchPattern))
      {
         logMessage = logMessage.substr(12);
      }

      while (logMessage[0] == ' ' || logMessage[0] == '*' || logMessage[0] == '=' || logMessage[0] == '>')
      {
         logMessage.erase(0,1); 
      }

      if (logMessage.size() > 2)
      {
         if (((geneTorrent *)geneTorrCallBackPtr)->getLogMask() & LOG_LT_CALL_BACK_LOGGER)
         {
            Log (PRIORITY_NORMAL, "%s", logMessage.c_str());
         }
      }
      return;
   }
   pthread_mutex_unlock (&callBackLoggerLock);
}

// 
int geneTorrent::statFileOrDirectory (std::string dirFile)
{
   time_t dummyArg;

   return statFileOrDirectory (dirFile, dummyArg);
}

// 
int geneTorrent::statFileOrDirectory (std::string dirFile, time_t &fileMtime)
{
   struct stat status;

   int statVal = stat (dirFile.c_str (), &status);

   if (statVal == 0 && S_ISDIR (status.st_mode))
   {
      DIR *dir;

      dir = opendir (dirFile.c_str());
  
      if (dir != NULL)
      {
         closedir (dir);
         return 0;
      }
      else 
      {
         return -1;
      }
   }

   if (statVal == 0 && S_ISREG (status.st_mode))
   {
      FILE *file;
   
      file = fopen (dirFile.c_str(), "r");
  
      if (file != NULL)
      {
         fileMtime = status.st_mtime;
         fclose (file);
         return 0;
      }
      else 
      {
         return -1;
      }
   }

   return -1;
}

// 
std::string geneTorrent::getWorkingDirectory ()
{
   long size;
   char *buf;
   char *ptr;

   size = pathconf (".", _PC_PATH_MAX);

   if ((buf = (char *) malloc ((size_t) size)) != NULL)
      ptr = getcwd (buf, (size_t) size);

   std::string result = buf;
   free (buf);
   return result;
}

// 
bool geneTorrent::generateCSR (std::string uuid)
{
    // TODO: migrate this to a direct call of the OpenSSL API instead
    // of calling a subprocess
   std::string cmd = "openssl req -config " + _gtOpenSslConf + " -new -nodes -out " + _tmpDir + uuid + ".csr -keyout " + _tmpDir + uuid + ".key";

   int result = system (cmd.c_str());

   if (result != 0)
   {
      if (_operatingMode != SERVER_MODE)
      {
         gtError ("Failure creating SSL CSR and/or key file(s):  " + _tmpDir + uuid + ".csr and/or " + _tmpDir + uuid + ".key", 202, ERRNO_ERROR, errno);
      }
      else
      {
         gtError ("Failure creating SSL CSR and/or key file(s): " + _tmpDir + uuid + ".csr and/or " + _tmpDir + uuid + ".key", ERROR_NO_EXIT, ERRNO_ERROR, errno);
         return false;
      }
   }
   
   return true;
}

time_t geneTorrent::getExpirationTime (std::string torrentPathAndFileName)
{
   time_t expireTime = 2114406000;     // Default to 1/1/2037

   FILE *result = popen (("gtoinfo -x " + torrentPathAndFileName).c_str(), "r");

   if (result != NULL)
   {
      char vBuff[15];

      if (NULL != fgets (vBuff, 15, result))
      {
         expireTime = strtol (vBuff, NULL, 10);
      }
      else
      {
         Log (PRIORITY_HIGH, "Failure running gtoinfo on %s or no 'expires on' in GTO, serving using default expiration of 1/1/2037", torrentPathAndFileName.c_str());
      }
      pclose (result);
   }
   else
   {
      Log (PRIORITY_HIGH, "Failure running gtoinfo on %s, serving using default expiration of 1/1/2037", torrentPathAndFileName.c_str());
   }

   return expireTime;       
}

std::string geneTorrent::getFileName (std::string fileName)
{
   return boost::filesystem::path(fileName).filename().string();
}

std::string geneTorrent::getInfoHash (std::string torrentFile)
{
   libtorrent::error_code torrentError;
   libtorrent::torrent_info torrentInfo (torrentFile, torrentError);

   if (torrentError)
   {
      if (_operatingMode != SERVER_MODE)
      {
         gtError (".gto processing problem with " + torrentFile, 87, TORRENT_ERROR, torrentError.value (), "", torrentError.message ());
      }
      else
      {
         gtError (".gto processing problem with " + torrentFile, ERROR_NO_EXIT, TORRENT_ERROR, torrentError.value (), "", torrentError.message ());
         return "";
      }
   }

   return getInfoHash (&torrentInfo);
}

// 
std::string geneTorrent::getInfoHash (libtorrent::torrent_info *torrentInfo)
{
   libtorrent::sha1_hash const& info_hash = torrentInfo->info_hash();

   std::ostringstream infoHash;
   infoHash << info_hash;

   return infoHash.str();
}

// 
bool geneTorrent::generateSSLcertAndGetSigned(std::string torrentFile, std::string signUrl, std::string torrentUUID)
{
   std::string infoHash = getInfoHash(torrentFile);

   if (infoHash.size() < 20)
   {
      return false;
   }

   return acquireSignedCSR (infoHash, signUrl, torrentUUID);
}

// 
bool geneTorrent::acquireSignedCSR (std::string info_hash, std::string CSRSignURL, std::string uuid)
{
   bool csrStatus = generateCSR (uuid);

   if (false == csrStatus)
   {
      return csrStatus;
   }

   std::string certFileName = _tmpDir + uuid + ".crt";
   std::string csrFileName = _tmpDir + uuid + ".csr";

   std::string csrData = loadCSRfile (csrFileName);

   if (csrData.size() == 0)  // only enounter this in SERVER_MODE, other modes exit.
   {
      gtError ("Operating in Server Mode, empty or missing CSR file encountered.  Discarding GTO from serving qeueue.", ERROR_NO_EXIT);
      return false;
   }

   FILE *signedCert;

   signedCert = fopen (certFileName.c_str (), "w");

   if (signedCert == NULL)
   {
      if (_operatingMode != SERVER_MODE)
      {
         gtError ("Failure opening " + certFileName + " for output.", 202, ERRNO_ERROR, errno);
      }
      else
      {
         gtError ("Failure opening " + certFileName + " for output.", ERROR_NO_EXIT, ERRNO_ERROR, errno);
         return false;
      }
   }

   char errorBuffer[CURL_ERROR_SIZE + 1];
   errorBuffer[0] = '\0';

   std::string curlResponseHeaders = "";

   CURL *curl;
   curl = curl_easy_init ();

   if (!curl)
   {
      if (_operatingMode != SERVER_MODE)
      {
         gtError ("libCurl initialization failure", 201);
      }
      else
      {
         gtError ("libCurl initialization failure", ERROR_NO_EXIT);
         return false;
      }
   }

   curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 0);
   curl_easy_setopt (curl, CURLOPT_SSL_VERIFYHOST, 0);

   curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, errorBuffer);
   curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, NULL);
   curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION, &curlCallBackHeadersWriter);
   curl_easy_setopt (curl, CURLOPT_MAXREDIRS, 15);
   curl_easy_setopt (curl, CURLOPT_WRITEDATA, signedCert);
   curl_easy_setopt (curl, CURLOPT_WRITEHEADER, &curlResponseHeaders);
   curl_easy_setopt (curl, CURLOPT_NOSIGNAL, (long)1);
   curl_easy_setopt (curl, CURLOPT_POST, (long)1);

   struct curl_httppost *post=NULL;
   struct curl_httppost *last=NULL;

   curl_formadd (&post, &last, CURLFORM_COPYNAME, "token", CURLFORM_COPYCONTENTS, _authToken.c_str(), CURLFORM_END);
   curl_formadd (&post, &last, CURLFORM_COPYNAME, "cert_req", CURLFORM_COPYCONTENTS, csrData.c_str(), CURLFORM_END);
   curl_formadd (&post, &last, CURLFORM_COPYNAME, "info_hash", CURLFORM_COPYCONTENTS, info_hash.c_str(), CURLFORM_END);

   curl_easy_setopt (curl, CURLOPT_HTTPPOST, post);

   int timeoutVal = 15;
   int connTime = 4;

   curl_easy_setopt (curl, CURLOPT_URL, CSRSignURL.c_str());
   curl_easy_setopt (curl, CURLOPT_TIMEOUT, timeoutVal);
   curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, connTime);
   if (_verbosityLevel > VERBOSE_3)
   {
       curl_easy_setopt (curl, CURLOPT_VERBOSE, 1);
   }

   CURLcode res;

   res = curl_easy_perform (curl);

   if (res != CURLE_OK)
   {
      if (_operatingMode != SERVER_MODE)
      {
         gtError ("Problem communicating with GeneTorrent Executive while attempting a CSR signing transaction for UUID:  " + _uploadUUID, 203, geneTorrent::CURL_ERROR, res, "URL:  " + CSRSignURL);
      }
      else
      {
         gtError ("Problem communicating with GeneTorrent Executive while attempting a CSR signing transaction for UUID:  " + _uploadUUID, ERROR_NO_EXIT, geneTorrent::CURL_ERROR, res, "URL:  " + CSRSignURL);
         return false;
      }
   }

   long code;
   res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

   if (res != CURLE_OK)
   {
      if (_operatingMode != SERVER_MODE)
      {
         gtError ("Problem communicating with GeneTorrent Executive while attempting a CSR signing transaction for UUID:  " + _uploadUUID, 204, geneTorrent::DEFAULT_ERROR, 0, "URL:  " + CSRSignURL);
      }
      else
      {
         gtError ("Problem communicating with GeneTorrent Executive while attempting a CSR signing transaction for UUID:  " + _uploadUUID, ERROR_NO_EXIT, geneTorrent::DEFAULT_ERROR, 0, "URL:  " + CSRSignURL);
         return false;
      }
   }

   if (code != 200)
   {
      if (_operatingMode != SERVER_MODE)
      {
         gtError ("Problem communicating with GeneTorrent Executive while attempting a CSR signing transaction for UUID:  " + _uploadUUID, 205, geneTorrent::HTTP_ERROR, code, "URL:  " + CSRSignURL);
      }
      else
      {
         gtError ("Problem communicating with GeneTorrent Executive while attempting a CSR signing transaction for UUID:  " + _uploadUUID, ERROR_NO_EXIT, geneTorrent::HTTP_ERROR, code, "URL:  " + CSRSignURL);
         return false;
      }
   }

   if (_verbosityLevel > VERBOSE_3)
   {
      screenOutput ("Headers received from the client:  '" << curlResponseHeaders << "'" << std::endl);
   }

   curl_easy_cleanup (curl);

   fclose (signedCert);

   return true;
}

void geneTorrent::curlCleanupOnFailure (std::string fileName, FILE *gtoFile)
{
   fclose (gtoFile);
   int ret = unlink (fileName.c_str ());

   if (ret != 0)
   {
      gtError ("Unable to remove ", NO_EXIT, geneTorrent::ERRNO_ERROR, errno);
   }
}

