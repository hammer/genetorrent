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

#include "gtBase.h"
#include "stringTokenizer.h"
#include "gtDefs.h"
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

extern int global_verbosity;

gtBase::gtBase (boost::program_options::variables_map &commandLine, opMode mode) : 
   _verbosityLevel (0), 
   _logToStdErr (false),
   _authToken (""), 
   _devMode (false),
   _tmpDir (""), 
   _logDestination ("none"),     // default to no logging
   _portStart (20892), 
   _portEnd (20900), 
   _exposedPortDelta (0), 
   _startUpComplete (false),
   _bindIP (""), 
   _exposedIP (""), 
   _operatingMode (mode), 
   _confDir (CONF_DIR_DEFAULT), 
   _logMask (0)                 // set all bits to 0
{
   geneTorrCallBackPtr = (void *) this;          // Set the global geneTorr pointer that allows fileFilter callbacks from libtorrent

   pthread_mutex_init (&callBackLoggerLock, NULL);

   std::ostringstream startUpMessage;

// TODO fix to add the command line to the startup message
   startUpMessage << "Starting version " << VERSION << " with the command line: ";

/*
   for (int loop = 1; loop < argc; loop++)
   {
      startUpMessage << " " << argv[loop];
   }
*/

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

   processConfigFileAndCLI (commandLine);

   _verbosityLevel = global_verbosity;

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
}

void gtBase::processConfigFileAndCLI (boost::program_options::variables_map &vm)
{
   pcfacliBindIP (vm);
   pcfacliConfDir (vm);
   pcfacliCredentialFile (vm);
   pcfacliAdvertisedIP (vm);
   pcfacliInternalPort (vm);    // Internal ports must be processed before the Advertised port
   pcfacliAdvertisedPort (vm);
   pcfacliLog (vm);
   pcfacliPath (vm);
}

void gtBase::pcfacliBindIP (boost::program_options::variables_map &vm)
{
   if (vm.count (BIND_IP_CLI_OPT) == 1 && vm.count (BIND_IP_CLI_OPT_LEGACY) == 1)
   {
      commandLineError ("duplicate config options:  " + BIND_IP_CLI_OPT + " and " + BIND_IP_CLI_OPT_LEGACY + " are not permitted at the same time");
   }

   if (vm.count (BIND_IP_CLI_OPT) == 1)
   { 
      _bindIP = vm[BIND_IP_CLI_OPT].as<std::string>();
   }
   else if (vm.count (BIND_IP_CLI_OPT_LEGACY) == 1)
   { 
      _bindIP = vm[BIND_IP_CLI_OPT_LEGACY].as<std::string>();
   }
}

void gtBase::pcfacliConfDir (boost::program_options::variables_map &vm)
{
   if (vm.count (CONF_DIR_CLI_OPT) == 1 && vm.count (CONF_DIR_CLI_OPT_LEGACY) == 1)
   {
      commandLineError ("duplicate config options:  " + CONF_DIR_CLI_OPT + " and " + CONF_DIR_CLI_OPT_LEGACY + " are not permitted at the same time");
   }

   if (vm.count (CONF_DIR_CLI_OPT) == 1)
   { 
      _confDir = sanitizePath (vm[CONF_DIR_CLI_OPT].as<std::string>());
   }
   else if (vm.count (CONF_DIR_CLI_OPT_LEGACY) == 1)
   { 
      _confDir = sanitizePath (vm[CONF_DIR_CLI_OPT_LEGACY].as<std::string>());
   }
   else   // Option not present
   {
      return;    
   }

   if ((_confDir.size () == 0) || (_confDir[0] != '/'))
   {
      commandLineError ("configuration directory '" + _confDir + "' must be an absolute path");
   }

   if (statFileOrDirectory (_confDir) != 0)
   {
      commandLineError ("unable to opening configuration directory '" + _confDir + "'");
   }
}

void gtBase::pcfacliCredentialFile (boost::program_options::variables_map &vm)
{
   if (vm.count (CRED_FILE_CLI_OPT) == 1 && vm.count (CRED_FILE_CLI_OPT_LEGACY) == 1)
   {
      commandLineError ("duplicate config options:  " + CRED_FILE_CLI_OPT + " and " + CRED_FILE_CLI_OPT_LEGACY + " are not permitted at the same time");
   }

   std::string credsPathAndFile;

   if (vm.count (CRED_FILE_CLI_OPT) == 1)
   { 
      credsPathAndFile = vm[CRED_FILE_CLI_OPT].as<std::string>();
   }
   else if (vm.count (CRED_FILE_CLI_OPT_LEGACY) == 1)
   { 
      credsPathAndFile = vm[CRED_FILE_CLI_OPT_LEGACY].as<std::string>();
   }
   else   // Option not present
   {
      return;    
   }

   std::ifstream credFile;

   credFile.open (credsPathAndFile.c_str(), std::ifstream::in);

   if (!credFile.good ())
   {
      commandLineError ("file not found (or is not readable):  " + credsPathAndFile);
   }

   credFile >> _authToken;

   credFile.close ();
}

void gtBase::pcfacliAdvertisedIP (boost::program_options::variables_map &vm)
{
   if (vm.count (ADVERT_IP_CLI_OPT) == 1 && vm.count (ADVERT_IP_CLI_OPT_LEGACY) == 1)
   {
      commandLineError ("duplicate config options:  " + ADVERT_IP_CLI_OPT + " and " + ADVERT_IP_CLI_OPT_LEGACY + " are not permitted at the same time");
   }

   if (vm.count (ADVERT_IP_CLI_OPT) == 1)
   { 
      _exposedIP = vm[ADVERT_IP_CLI_OPT].as<std::string>();
   }
   else if (vm.count (ADVERT_IP_CLI_OPT_LEGACY) == 1)
   { 
      _exposedIP = vm[ADVERT_IP_CLI_OPT_LEGACY].as<std::string>();
   }
}

void gtBase::pcfacliInternalPort (boost::program_options::variables_map &vm)
{
   if (vm.count (INTERNAL_PORT_CLI_OPT) == 1 && vm.count (INTERNAL_PORT_CLI_OPT_LEGACY) == 1)
   {
      commandLineError ("duplicate config options:  " + INTERNAL_PORT_CLI_OPT + " and " + INTERNAL_PORT_CLI_OPT_LEGACY + " are not permitted at the same time");
   }

   std::string portList;

   if (vm.count (INTERNAL_PORT_CLI_OPT) == 1)
   { 
      portList = vm[INTERNAL_PORT_CLI_OPT].as<std::string>();
   }
   else if (vm.count (INTERNAL_PORT_CLI_OPT_LEGACY) == 1)
   { 
      portList = vm[INTERNAL_PORT_CLI_OPT_LEGACY].as<std::string>();
   }
   else   // Option not present
   {
      return;    
   }

   strTokenize strToken (portList, ":", strTokenize::MERGE_CONSECUTIVE_SEPARATORS);

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
         commandLineError ("when using -i (--internal-port) string1 must be smaller than string2");
      }
   }
   else
   {
      _portStart = lowPort;
      _portEnd = _portStart + 8; // default 8 ports
   }
}

void gtBase::pcfacliAdvertisedPort (boost::program_options::variables_map &vm)
{
   if (vm.count (ADVERT_PORT_CLI_OPT) == 1 && vm.count (ADVERT_PORT_CLI_OPT_LEGACY) == 1)
   {
      commandLineError ("duplicate config options:  " + ADVERT_PORT_CLI_OPT + " and " + ADVERT_PORT_CLI_OPT_LEGACY + " are not permitted at the same time");
   }

   uint32_t exposedPort;

   if (vm.count (ADVERT_PORT_CLI_OPT) == 1)
   { 
      exposedPort = vm[ADVERT_PORT_CLI_OPT].as< uint32_t >();
   }
   else if (vm.count (ADVERT_PORT_CLI_OPT_LEGACY) == 1)
   { 
      exposedPort = vm[ADVERT_PORT_CLI_OPT_LEGACY].as< uint32_t >();
   }
   else   // Option not present
   {
      return;    
   }

   _exposedPortDelta = exposedPort - _portStart;
}

void gtBase::pcfacliLog (boost::program_options::variables_map &vm)
{
   if (vm.count (LOGGING_CLI_OPT) < 1)
   {
      return;    
   }

   strTokenize strToken (vm[LOGGING_CLI_OPT].as<std::string>(), ":", strTokenize::MERGE_CONSECUTIVE_SEPARATORS);

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

   _logToStdErr = gtLogger::create_globallog (PACKAGE_NAME, _logDestination);

// TODO, logstartup message   Log (PRIORITY_NORMAL, "%s (using tmpDir = %s)", startUpMessage.str().c_str(), _tmpDir.c_str());
}

// Used by download and upload
std::string gtBase::pcfacliPath (boost::program_options::variables_map &vm)
{
   if (vm.count (PATH_CLI_OPT) < 1)
   {
      return "";    
   }

   std::string path = sanitizePath (vm[PATH_CLI_OPT].as<std::string>());

   if (path.size() == 0)
   {
      commandLineError ("command line or config file contains no value for '" + PATH_CLI_OPT + "'");
   }

   if (statFileOrDirectory (path) != 0)
   {
      commandLineError ("unable to opening directory '" + path + "'");
   }

   return path;
}

// 
void gtBase::setTempDir ()     
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
void gtBase::mkTempDir () 
{
   int retValue = mkdir (_tmpDir.c_str(), 0700);       
   if (retValue != 0 )
   {
      gtError ("Failure creating temporary directory " + _tmpDir, 202, ERRNO_ERROR, errno);
   }

   _tmpDir += "/";
}

// 
void gtBase::checkCredentials ()
{
   if (_authToken.size() < 1)
   {
      commandLineError ("Must include a credential file when attempting to communicate with CGHub, use -c or --" + CRED_FILE_CLI_OPT);
   }
}

// 
std::string gtBase::loadCSRfile (std::string csrFileName)
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
std::string gtBase::sanitizePath (std::string inPath)
{
   if (inPath[inPath.size () - 1] == '/')
   {
      inPath.erase (inPath.size () - 1);
   }

   return (inPath);
}

// 
gtBase::~gtBase ()
{
}

// 
void gtBase::gtError (std::string errorMessage, int exitValue, gtErrorType errorType, long errorCode, std::string errorMessageLine2, std::string errorMessageErrorLine)
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
      case gtBase::HTTP_ERROR:
      {
         logMessage << "  Additional Info:  " << getHttpErrorMessage (errorCode) << " (HTTP status code = " << errorCode << ").";
      }
         break;

      case gtBase::CURL_ERROR:
      {
         logMessage << "  Additional Info:  " << curl_easy_strerror (CURLcode (errorCode)) << " (curl code = " << errorCode << ").";
      }
         break;

      case gtBase::ERRNO_ERROR:
      {
         logMessage << "  Additional Info:  " << strerror (errorCode) << " (errno = " << errorCode << ").";
      }
         break;

      case gtBase::TORRENT_ERROR:
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
std::string gtBase::getHttpErrorMessage (int code)
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
int gtBase::curlCallBackHeadersWriter (char *data, size_t size, size_t nmemb, std::string *buffer)
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
//void gtBase::run ()
//{
/*
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
*/

// 
void gtBase::cleanupTmpDir()
{
   if (!_devMode)
   {
      system (("rm -rf " + _tmpDir.substr(0,_tmpDir.size()-1)).c_str());
   }
}

// 
void gtBase::bindSession (libtorrent::session *torrentSession)
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
void gtBase::bindSession (libtorrent::session &torrentSession)
{
   bindSession (&torrentSession);
}

// 
void gtBase::optimizeSession (libtorrent::session *torrentSession)
{
   libtorrent::session_settings settings = torrentSession->settings ();

   settings.allow_multiple_connections_per_ip = true;
#ifdef TORRENT_CALLBACK_LOGGER
   settings.loggingCallBack = &gtBase::loggingCallBack;
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
void gtBase::optimizeSession (libtorrent::session &torrentSession)
{
   optimizeSession (&torrentSession);
}

// 
void gtBase::loggingCallBack (std::string message)
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
         if (((gtBase *)geneTorrCallBackPtr)->getLogMask() & LOG_LT_CALL_BACK_LOGGER)
         {
            Log (PRIORITY_NORMAL, "%s", logMessage.c_str());
         }
      }
      return;
   }
   pthread_mutex_unlock (&callBackLoggerLock);
}

// 
int gtBase::statFileOrDirectory (std::string dirFile)
{
   time_t dummyArg;

   return statFileOrDirectory (dirFile, dummyArg);
}

// 
int gtBase::statFileOrDirectory (std::string dirFile, time_t &fileMtime)
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
std::string gtBase::getWorkingDirectory ()
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
bool gtBase::generateCSR (std::string uuid)
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

std::string gtBase::getFileName (std::string fileName)
{
   return boost::filesystem::path(fileName).filename().string();
}

std::string gtBase::getInfoHash (std::string torrentFile)
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
std::string gtBase::getInfoHash (libtorrent::torrent_info *torrentInfo)
{
   libtorrent::sha1_hash const& info_hash = torrentInfo->info_hash();

   std::ostringstream infoHash;
   infoHash << info_hash;

   return infoHash.str();
}

// 
bool gtBase::generateSSLcertAndGetSigned(std::string torrentFile, std::string signUrl, std::string torrentUUID)
{
   std::string infoHash = getInfoHash(torrentFile);

   if (infoHash.size() < 20)
   {
      return false;
   }

   return acquireSignedCSR (infoHash, signUrl, torrentUUID);
}

// 
bool gtBase::acquireSignedCSR (std::string info_hash, std::string CSRSignURL, std::string uuid)
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
         gtError ("Problem communicating with GeneTorrent Executive while attempting a CSR signing transaction for UUID:  " + uuid, 203, gtBase::CURL_ERROR, res, "URL:  " + CSRSignURL);
      }
      else
      {
         gtError ("Problem communicating with GeneTorrent Executive while attempting a CSR signing transaction for UUID:  " + uuid, ERROR_NO_EXIT, gtBase::CURL_ERROR, res, "URL:  " + CSRSignURL);
         return false;
      }
   }

   long code;
   res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

   if (res != CURLE_OK)
   {
      if (_operatingMode != SERVER_MODE)
      {
         gtError ("Problem communicating with GeneTorrent Executive while attempting a CSR signing transaction for UUID:  " + uuid, 204, gtBase::DEFAULT_ERROR, 0, "URL:  " + CSRSignURL);
      }
      else
      {
         gtError ("Problem communicating with GeneTorrent Executive while attempting a CSR signing transaction for UUID:  " + uuid, ERROR_NO_EXIT, gtBase::DEFAULT_ERROR, 0, "URL:  " + CSRSignURL);
         return false;
      }
   }

   if (code != 200)
   {
      if (_operatingMode != SERVER_MODE)
      {
         gtError ("Problem communicating with GeneTorrent Executive while attempting a CSR signing transaction for UUID:  " + uuid, 205, gtBase::HTTP_ERROR, code, "URL:  " + CSRSignURL);
      }
      else
      {
         gtError ("Problem communicating with GeneTorrent Executive while attempting a CSR signing transaction for UUID:  " + uuid, ERROR_NO_EXIT, gtBase::HTTP_ERROR, code, "URL:  " + CSRSignURL);
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

void gtBase::curlCleanupOnFailure (std::string fileName, FILE *gtoFile)
{
   fclose (gtoFile);
   int ret = unlink (fileName.c_str ());

   if (ret != 0)
   {
      gtError ("Unable to remove ", NO_EXIT, gtBase::ERRNO_ERROR, errno);
   }
}
