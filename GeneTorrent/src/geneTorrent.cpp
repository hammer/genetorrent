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

void *geneTorr; // global variable that used to point to GeneTorrent to allow libtorrent callback for file inclusion and sys logging.
                // initialized in geneTorrent constructor, used in the free function file_filter to call member fileFilter.

geneTorrent::geneTorrent (int argc, char **argv) : _args (SHORT_DESCRIPTION, ' ', VERSION), _bindIP (""), _exposedIP (""), _portStart (20892), _portEnd (20900), _exposedPortDelta (0), _verbosityLevel (0), _manifestFile (""), _uploadUUID (""), _uploadSubmissionURL (""), _dataFilePath (""), _cliArgsDownloadList (), _downloadSavePath (""), _serverQueuePath (""), _serverDataPath (""), _authToken (""), _operatingMode (UPLOAD_MODE), _filesToUpload (), _pieceSize (4194304), _torrentListToDownload (), _activeSessions (), _tmpDir (""), _confDir (CONF_DIR_DEFAULT), _startUpComplete (false), _devMode (false), _logAppend (NULL), _layout (NULL)
{
   geneTorr = (void *) this;          // Set the global geneTorr pointer that allows fileFilter callbacks from libtorrent

   setupSysLog();

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

   *sysLogGT << log4cpp::Priority::INFO << startUpMessage.str() << " (using tmpDir = " << _tmpDir << ")";

   // General purpose command line arguments.
   // Bind IP address (on local machine), upload, download, cghub server
   TCLAP::ValueArg <std::string> bindIP ("b", "bindIP", "Description Not Used", false, "", "string");
   _args.add (bindIP);

   // Exposed IP address (of local machine), upload, download, cghub server
   TCLAP::ValueArg <std::string> exposedIP ("e", "advertisedIP", "Description Not Used", false, "", "string");
   _args.add (exposedIP);

   // Exposed ports (of local machine), upload, download, cghub server
   TCLAP::ValueArg <int> exposedPort ("f", "advertisedPort", "Description Not Used", false, 0, "int");
   _args.add (exposedPort);

   // Internal ports (of local machine), upload, download, cghub server
   TCLAP::ValueArg <std::string> internalPort ("i", "internalPorts", "Description Not Used", false, "", "string");
   _args.add (internalPort);

   // Generic path used by upload, download, cghub server
   TCLAP::ValueArg <std::string> genericPath ("p", "path", "Description Not Used", false, "", "string");
   _args.add (genericPath);

   // configuration directory
   TCLAP::ValueArg <std::string> confDir ("C", "confDir", "Description Not Used", false, "", "string");
   _args.add (confDir);

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

void geneTorrent::setupSysLog ()
{
   // Configure log4cpp to use syslog
   std::ostringstream appPid;
   appPid << "GeneTorrent: [" << getpid() << "]";

   if (_logAppend != NULL)
   {
      sysLogGT->removeAllAppenders();
   }

   _layout = new log4cpp::PatternLayout ();
   _layout->setConversionPattern (log4cpp::PatternLayout::DEFAULT_CONVERSION_PATTERN);
   _logAppend = new log4cpp::SyslogAppender ("SyslogAppender", appPid.str());
   _logAppend->setLayout (_layout);
   sysLogGT = &(log4cpp::Category::getInstance ("sysLogGT"));
   sysLogGT->setAdditivity (false);
   sysLogGT->setAppender (_logAppend);
   sysLogGT->setPriority (log4cpp::Priority::INFO);
}

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

void geneTorrent::mkTempDir () 
{
   int retValue = mkdir (_tmpDir.c_str(), 0700);       
   if (retValue != 0 )
   {
      gtError ("Failure creating temporary directory " + _tmpDir, 202, ERRNO_ERROR, errno);
   }

   _tmpDir += "/";
}

void geneTorrent::loadCredentialsFile (bool credsSet, std::string credsFile)
{
   if (!credsSet)
   {
      TCLAP::ArgException argError ("Must include a credential file when attempting to download", "-c  (--credentialFile)");
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

std::string geneTorrent::sanitizePath (std::string inPath)
{
   if (inPath[inPath.size () - 1] == '/')
   {
      inPath.erase (inPath.size () - 1);
   }

   return (inPath);
}

geneTorrent::~geneTorrent ()
{
}

void geneTorrent::prepareDownloadList ()
{
   if (_verbosityLevel > 0)
   {
      std::cerr << "Welcome to GeneTorrent version " << VERSION << ", download mode." << std::endl; // prints Welcome to GeneTorrent
   }

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
            if (inspect[0] == '/') // presume a full path is present
            {
               _torrentListToDownload.push_back (getWorkingDirectory () + "/" + inspect);
            }
            else
            {
               _torrentListToDownload.push_back (getWorkingDirectory () + "/" + inspect);
            }
         }
         else if (tail == ".xml" || tail == ".XML") // Extract a list of URIs from passed XML
         {
            extractURIsFromXML (inspect, urisToDownload);
         }
         else if (std::string::npos != inspect.find ("/")) // Have a URI, add the token later if needed.
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

void geneTorrent::gtError (std::string errorMessage, int exitValue, gtErrorType errorType, long errorCode, std::string errorMessageLine2, std::string errorMessageErrorLine)
{
   std::ostringstream sysLogMess;

   if (exitValue > 0)
   {
      sysLogMess << "Error:  " << errorMessage;
      std::cerr << "Error:  " << errorMessage << std::endl;
   }
   else if (exitValue == NO_EXIT)
   {
      sysLogMess << "Warning:  " << errorMessage;
      std::cerr << "Warning:  " << errorMessage << std::endl;
   }
   else  // exitValue == ERROR_NO_EXIT, the caller handles the error and this permits syslogging only
   {
      sysLogMess << "Error:  " << errorMessage;
   }

   switch (errorType)
   {
      case geneTorrent::HTTP_ERROR:
      {
         if (errorMessageLine2.size () > 0)
         {
            sysLogMess << ", " << errorMessageLine2;
            std::cerr << errorMessageLine2 << std::endl;
         }
         sysLogMess << "  Additional Info:  " << getHttpErrorMessage (errorCode) << " (HTTP status code = " << errorCode << ").";
         std::cerr << "Additional Info:  " << getHttpErrorMessage (errorCode) << " (HTTP status code = " << errorCode << ")." << std::endl;
      }
         break;

      case geneTorrent::CURL_ERROR:
      {
         if (errorMessageLine2.size () > 0)
         {
            sysLogMess << ", " << errorMessageLine2;
            std::cerr << errorMessageLine2 << std::endl;
         }
         sysLogMess << "  Additional Info:  " << curl_easy_strerror (CURLcode (errorCode)) << " (curl code = " << errorCode << ").";
         std::cerr << "Additional Info:  " << curl_easy_strerror (CURLcode (errorCode)) << " (curl code = " << errorCode << ")." << std::endl;
      }
         break;

      case geneTorrent::ERRNO_ERROR:
      {
         if (errorMessageLine2.size () > 0)
         {
            std::cerr << errorMessageLine2 << std::endl;
         }
         sysLogMess << "  Additional Info:  " << strerror (errorCode) << " (errno = " << errorCode << ").";
         std::cerr << "Additional Info:  " << strerror (errorCode) << " (errno = " << errorCode << ")." << std::endl;
      }
         break;

      case geneTorrent::TORRENT_ERROR:
      {
         if (errorMessageLine2.size () > 0)
         {
            sysLogMess << ", " << errorMessageLine2;
            std::cerr << errorMessageLine2 << std::endl;
         }
         sysLogMess << "  Additional Info:  " << errorMessageErrorLine << " (GT code = " << errorCode << ").";
         std::cerr << "Additional Info:  " << errorMessageErrorLine << " (GT code = " << errorCode << ")." << std::endl;
      }
         break;

      default:
      {
         if (errorMessageLine2.size () > 0)
         {
            sysLogMess << ", " << errorMessageLine2;
            std::cerr << errorMessageLine2 << std::endl;
         }

         if (errorMessageErrorLine.size () > 0)
         {
            sysLogMess << ", " << errorMessageErrorLine;
            std::cerr << "Additional Info:  " << errorMessageErrorLine << std::endl;
         }
      }
         break;
   }

   *sysLogGT << log4cpp::Priority::INFO << sysLogMess.str();

   if (exitValue > 0)
   {
      std::cerr << "Cannot continue." << std::endl;
      if (_startUpComplete)
      {
         cleanupTmpDir();
      }
      exit (exitValue);
   }
}

void geneTorrent::downloadGtoFilesByURI (vectOfStr &uris)
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
         gtError ("Problem communicating with GeneTorrent Executive while trying to retrieve transfer metadata for UUID:  " + torrUUID, 203, geneTorrent::CURL_ERROR, res, "URL:  " + uri);
      }

      long code;
      res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

      if (res != CURLE_OK)
      {
         curlCleanupOnFailure (fileName, gtoFile);
         gtError ("Problem communicating with GeneTorrent Executive while trying to retrieve transfer metadata for UUID:  " + torrUUID, 204, geneTorrent::DEFAULT_ERROR, 0, "URL:  " + uri);
      }

      if (code != 200)
      {
         curlCleanupOnFailure (fileName, gtoFile);
         gtError ("Problem communicating with GeneTorrent Executive while trying to retrieve transfer metadata for UUID:  " + torrUUID, 205, geneTorrent::HTTP_ERROR, code, "URL:  " + uri);
      }

      if (_verbosityLevel > VERBOSE_3)
      {
         std::cerr << "Headers received from the client:  '" << curlResponseHeaders << "'" << std::endl << std::endl;
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
            gtError ("Unable to find " + pathToKeep + " in the URL:  " + uri, 214, geneTorrent::DEFAULT_ERROR);
         }

         std::string certSignURL = uri.substr(0, foundPos + pathToKeep.size()) + GT_CERT_SIGN_TAIL;

         generateSSLcertAndGetSigned(torrFile, certSignURL, torrUUID);
      }
      vectIter++;
   }
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

std::string geneTorrent::getHttpErrorMessage (int code)
{
   switch (code)
   {
      case 505:
      {
         return "HTTP Version Not Supported";
      }
         break;

      case 504:
      {
         return "Gateway Timeout";
      }
         break;

      case 503:
      {
         return "Service Unavailable";
      }
         break;

      case 502:
      {
         return "Bad Gateway";
      }
         break;

      case 501:
      {
         return "Not Implemented";
      }
         break;

      case 500:
      {
         return "Internal Server Error";
      }
         break;

      case 400:
      {
         return "Bad Request";
      }
         break;

      case 401:
      {
         return "Unauthorized";
      }
         break;

      case 403:
      {
         return "Forbidden";
      }
         break;

      case 404:
      {
         return "Not Found";
      }
         break;

      case 409:
      {
         return "Request Timeout";
      }
         break;

      case 411:
      {
         return "Gone";
      }
         break;

      case 412:
      {
         return "Length Required";
      }
         break;

      default:
      {
         return "Unknown, research given code";
      }
         break;
   }

   return "Unknown, research given code";
}

void geneTorrent::extractURIsFromXML (std::string xmlFileName, vectOfStr &urisToDownload)
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

void geneTorrent::run ()
{
   std::string saveDir = getWorkingDirectory ();

   switch (_operatingMode)
   {
      case DOWNLOAD_MODE:
      {
         time_t startTime = time(NULL);

         if (_downloadSavePath.size())
         {
            int cdRet = chdir (_downloadSavePath.c_str());

            if (cdRet != 0 )
            {
               gtError ("Failure changing directory to " + _downloadSavePath, 202, ERRNO_ERROR, errno);
            }
         }

         prepareDownloadList ();

         uint64_t totalBytes; 
         int totalFiles;
         int totalGtos;

         validateAndCollectSizeOfTorrents (totalBytes, totalFiles, totalGtos);

         std::ostringstream message;
         message << "Ready to download " << totalGtos << " GTO(s) with " << totalFiles << " file(s) comprised of " << add_suffix (totalBytes) << " of data"; 

         *sysLogGT << log4cpp::Priority::INFO << message.str();

         if (_verbosityLevel > 0)
         {
            std::cerr << message.str() << std::endl;
         }

         performTorrentDownload (totalBytes);

         chdir (saveDir.c_str ());       // shutting down, if the chdir back fails, so be it
         message.str("");
  
         time_t duration = time(NULL) - startTime;
         message << "Downloaded " << add_suffix (totalBytes) << " in " << std::setw(2) << std::setfill('0') << duration / 3600  << ":" << std::setw(2) << std::setfill('0') << duration/60 << ":" << std::setw(2) << std::setfill('0') << duration%60 << ".  Overall Rate " << add_suffix (totalBytes/duration) << "/s";

         *sysLogGT << log4cpp::Priority::INFO << message.str();

         if (_verbosityLevel > 0)
         {
            std::cerr << message.str() << std::endl;
         }

      } break;

      case SERVER_MODE:
      {
         runServerMode ();
      } break;

      default: // UPLOAD_MODE
      {
         performTorrentUpload ();
         chdir (saveDir.c_str ());      // shutting down, if the chdir back fails, so be it
      } break;
   }

   cleanupTmpDir();
}

void geneTorrent::cleanupTmpDir()
{
   if (!_devMode)
   {
      system (("rm -rf " + _tmpDir.substr(0,_tmpDir.size()-1)).c_str());
   }
}

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
		 settings.inhibit_keepalives = true;

   torrentSession->set_settings (settings);
}

void geneTorrent::bindSession (libtorrent::session &torrentSession)
{
   bindSession (&torrentSession);
}

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

void geneTorrent::optimizeSession (libtorrent::session &torrentSession)
{
   optimizeSession (&torrentSession);
}

void geneTorrent::loggingCallBack (std::string message)
{
   static std::ostringstream messageBuff;

   messageBuff << message;

   if (std::string::npos != message.find ('\n'))
   {
      std::string logMessage = messageBuff.str().substr(0, messageBuff.str().size() - 1);
      messageBuff.str("");

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
         geneTorrent *myThis = (geneTorrent *) geneTorr;
         myThis->sysLogMessage (logMessage);
      }
   }
}

void geneTorrent::sysLogMessage (std::string message)
{
   *sysLogGT << log4cpp::Priority::INFO << message;
}

void geneTorrent::performTorrentDownload (int64_t totalSizeOfDownload)
{
   int64_t freeSpace = getFreeDiskSpace();
   if (totalSizeOfDownload > freeSpace) 
   {
      gtError ("The system does not have enough free disk space to complete this transfer (transfer total size is " + add_suffix (totalSizeOfDownload) + "; free space is " + add_suffix (freeSpace), 97, geneTorrent::DEFAULT_ERROR, 0);
   }

   vectOfStr::iterator vectIter = _torrentListToDownload.begin ();

   int maxChildren = 8;
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
               gtError ("The system *might* run out of disk space before all downloads are complete", NO_EXIT, geneTorrent::DEFAULT_ERROR, 0, "Downloading will continue until less than " + add_suffix (DISK_FREE_WARN_LEVEL) + " is available.");
            }
            else
            {
                   gtError ("The system is running low on disk space.  Closing download client", 97, geneTorrent::DEFAULT_ERROR, 0);
            }
         }

         if (_verbosityLevel > 0) 
         {
            std::cerr << "Status:"  << std::setw(8) << (totalDataDownloaded+xfer > 0 ? add_suffix(totalDataDownloaded+xfer).c_str() : "0 bytes") <<  " downloaded (" << std::fixed << std::setprecision(3) << (100.0*(totalDataDownloaded+xfer)/totalSizeOfDownload) << "% complete) current rate:  " << add_suffix (dlRate).c_str() << "/s" << std::endl;
         }
      }

      vectIter++;
   }
}

int geneTorrent::downloadChild(int childID, int totalChildren, std::string torrentName, FILE *fd)
{
   setupSysLog();

   libtorrent::session torrentSession (*_gtFingerPrint, 0, libtorrent::alert::tracker_notification);
   optimizeSession (torrentSession);
   bindSession (torrentSession);

   std::string uuid = torrentName;
   uuid = uuid.substr (0, uuid.rfind ('.'));
   uuid = getFileName (uuid); 

   libtorrent::add_torrent_params torrentParams;
   torrentParams.save_path = "./";
   torrentParams.allow_rfc1918_connections = true;
   torrentParams.auto_managed = false;

   if (statFileOrDirectory ("./" + uuid) == 0)
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

   if (_verbosityLevel > 3)
   {
      *sysLogGT << log4cpp::Priority::INFO << "Child: " << childID << "  start piece:  " << myStartPiece << "  endPiece: " << myEndPiece-1;
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
            gtError ("download parent process has died, all download children are exiting", 197, geneTorrent::DEFAULT_ERROR);
         }
         exit (197);
      }

      // sleep before reporting status to parent -- monitoring the state of the torrent every 1/2 second
      for (int i=0; i<8 && currentState != libtorrent::torrent_status::seeding && currentState != libtorrent::torrent_status::finished; i++)
      {
         usleep(SLEEP_INTERVAL/8);
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

      if (_verbosityLevel > 3)
      {
         if (torrentStatus.state != libtorrent::torrent_status::queued_for_checking && torrentStatus.state != libtorrent::torrent_status::checking_files)
         {
            std::cerr << "Child " << childID << " " << download_state_str[torrentStatus.state] << "  " << add_suffix (torrentStatus.total_wanted_done).c_str () << "  (" << add_suffix (torrentStatus.download_payload_rate, "/s").c_str () << ")" << std::endl;
         }
      }
      currentState = torrentHandle.status().state;
   }

   exit (0);
}

void geneTorrent::checkAlerts (libtorrent::session &torrSession)
{
   std::deque <libtorrent::alert *> alerts;
   torrSession.pop_alerts (&alerts);   

   for (std::deque<libtorrent::alert *>::iterator dequeIter = alerts.begin(), end(alerts.end()); dequeIter != end; ++dequeIter)
   {
      if (((*dequeIter)->category() & libtorrent::alert::tracker_notification) && ((*dequeIter)->category() & libtorrent::alert::error_notification))
      {
         libtorrent::tracker_error_alert *tea = libtorrent::alert_cast<libtorrent::tracker_error_alert> (*dequeIter);

         if (tea->times_in_row > 1)
         {
            gtError ("Failure communicating with the transactor on URL:  " + tea->url, 214, geneTorrent::DEFAULT_ERROR, 0, tea->error.message());
         }
      }
   }
   alerts.clear();
}

void geneTorrent::performTorrentUpload ()
{
   if (_verbosityLevel > 0)
   {
      std::cerr << "Welcome to GeneTorrent version " << VERSION << ", upload mode." << std::endl; // prints Welcome to GeneTorrent
   }

   processManifestFile ();

   if (_uploadSubmissionURL.size () < 1)
   {
      gtError ("No Submission URL found in manifest file:  " + _manifestFile, 214, geneTorrent::DEFAULT_ERROR);
   }

   if (_uploadUUID.size () < 1)
   {
      gtError ("No server_path (UUID) found in manifest file:  " + _manifestFile, 214, geneTorrent::DEFAULT_ERROR);
   }

   if (_filesToUpload.size () < 1)
   {
      gtError ("No files found in manifest file:  " + _manifestFile, 214, geneTorrent::DEFAULT_ERROR);
   }

   findDataAndSetWorkingDirectory ();
   setPieceSize ();

   std::string torrentFileName = makeTorrent (_uploadUUID, _uploadUUID + GTO_FILE_EXTENSION);

   torrentFileName = submitTorrentToGTExecutive (torrentFileName);

   performGtoUpload (torrentFileName);
}

std::string geneTorrent::submitTorrentToGTExecutive (std::string tmpTorrentFileName)
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
      gtError ("Problem communicating with GeneTorrent Executive while trying to submit metadata for UUID:  " + _uploadUUID, 203, geneTorrent::CURL_ERROR, res, "URL:  " + _uploadSubmissionURL);
   }

   long code;
   res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

   if (res != CURLE_OK)
   {
      curlCleanupOnFailure (realTorrentFileName, gtoFile);
      gtError ("Problem communicating with GeneTorrent Executive while trying to submit metadata for UUID:  " + _uploadUUID, 204, geneTorrent::DEFAULT_ERROR, 0, "URL:  " + _uploadSubmissionURL);
   }

   if (code != 200)
   {
      curlCleanupOnFailure (realTorrentFileName, gtoFile);
      gtError ("Problem communicating with GeneTorrent Executive while trying to submit metadata for UUID:  " + _uploadUUID, 205, geneTorrent::HTTP_ERROR, code, "URL:  " + _uploadSubmissionURL);
   }

   if (_verbosityLevel > VERBOSE_3)
   {
      std::cerr << "Headers received from the client:  '" << curlResponseHeaders << "'" << std::endl << std::endl;
   }

   curl_easy_cleanup (curl);

   fclose (gtoFile);

   return realTorrentFileName;
}

void geneTorrent::findDataAndSetWorkingDirectory ()
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
   {                                                 // if all files are missing presume wrong directory and continue
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
bool geneTorrent::verifyDataFilesExist (vectOfStr &missingFileList)
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

void geneTorrent::setPieceSize ()
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

void geneTorrent::displayMissingFilesAndExit (vectOfStr &missingFiles)
{
   vectOfStr::iterator vectIter = missingFiles.begin ();

   while (vectIter != missingFiles.end ())
   {
      std::cerr << "Error:  " << strerror (errno) << " (errno = " << errno << ")  FileName:  " << _uploadUUID << "/" << *vectIter << std::endl;
      vectIter++;
   }
   gtError ("file(s) listed above were not found (or is (are) not readable)", 82, geneTorrent::DEFAULT_ERROR);
}

int geneTorrent::statFileOrDirectory (std::string dirFile)
{
   time_t dummyArg;

   return statFileOrDirectory (dirFile, dummyArg);
}

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

std::string geneTorrent::makeTorrent (std::string filePath, std::string torrentName)
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

void geneTorrent::processManifestFile ()
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

int64_t geneTorrent::getFreeDiskSpace ()
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

void geneTorrent::getFilesInQueueDirectory (vectOfStr &files)
{
   boost::filesystem::path queueDir (_serverQueuePath);

   boost::regex searchPattern (std::string (".*" + GTO_FILE_EXTENSION));

   for (boost::filesystem::directory_iterator iter (queueDir), end; iter != end; iter++)
   {
      std::string fileName = iter->path().filename().string();
      if (regex_match (fileName, searchPattern))
      {
         files.push_back (_serverQueuePath + '/' + fileName);
      }
   }
}

void geneTorrent::checkSessions ()
{
   geneTorrent::activeSessionRec *workingSessionRec = NULL;

   while (_activeSessions.size () == 0) // Loop until a session is added
   {
      libtorrent::session *workingSession = addActiveSession ();

      if (workingSession)
      {
         workingSessionRec = new geneTorrent::activeSessionRec;
         workingSessionRec->torrentSession = workingSession;

         _activeSessions.push_back (workingSessionRec);
      }
      else
      {
         *sysLogGT << log4cpp::Priority::INFO << "Unable to listen on any ports between " << _portStart << " and " << _portEnd << "(system level ports).  GeneTorrent can not Serve data until at least one port is available.";
         sleep (3); // give OS chance to clean up ports desired by this process
      }
   }

   if (_activeSessions.size () < _maxActiveSessions && workingSessionRec == NULL) // Try one time to add a new session
   {
      libtorrent::session *workingSession = addActiveSession ();

      if (workingSession)
      {
         workingSessionRec = new geneTorrent::activeSessionRec;
         workingSessionRec->torrentSession = workingSession;

         _activeSessions.push_back (workingSessionRec);
      }
   }
}

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

//
//  Managing torrents in Server mode is moderately complex
//
//  SessionRec->
//
//
//
void geneTorrent::runServerMode ()
{
   if (_verbosityLevel > 0)
   {
      std::cerr << "Welcome to GeneTorrent version " << VERSION << ", server mode." << std::endl; // prints Welcome to GeneTorrent
   }

   int cdRet = chdir (_serverDataPath.c_str ());

   if (cdRet != 0 )
   {
      gtError ("Failure changing directory to " + _serverDataPath, 202, ERRNO_ERROR, errno);
   }

   time_t nextMaintTime = time(NULL) + 30;         // next time stamp to perform maintenance -- shorten a bit for startup
   std::set <std::string> activeTorrentCollection; // list of active torrents, tracks if a GTO is being served
                                                   // This list is indirectly duplicated by the list of torrents being served
                                                   // that is maintained inside the list of activeSessions.
                                                   // This list is maintained here for simplicity and speed

   while (1) // Get the collection of .gto files in the queue directory
   {
      vectOfStr filesInQueue;
      getFilesInQueueDirectory (filesInQueue);
      vectOfStr::iterator vectIter = filesInQueue.begin ();

      while (vectIter != filesInQueue.end ()) // check each .gto file to ensure GeneTorrent is "serving" the file
      {
         std::set <std::string>::iterator actTorrentIter;
         actTorrentIter = activeTorrentCollection.find (*vectIter);

         if (actTorrentIter == activeTorrentCollection.end ()) // gto is not in the map of active torrents
         {
            if (addTorrentToServingList (*vectIter))  // Successfully added to the list of active torrents
            {
               activeTorrentCollection.insert (*vectIter);
            }
         } 
         vectIter++;
      }

      time_t timeNow = time(NULL);      

      if (nextMaintTime < timeNow)
      {
         std::list <activeSessionRec *>::iterator listIter = _activeSessions.begin ();
         while (listIter != _activeSessions.end ())
         {
            std::map <std::string, activeTorrentRec *>::iterator mapIter = (*listIter)->mapOfSessionTorrents.begin (); //mapOfSessionTorrents
            while (mapIter != (*listIter)->mapOfSessionTorrents.end ())
            {
               libtorrent::torrent_status torrentStatus = mapIter->second->torrentHandle.status ();
               time_t torrentModTime = 0;
               
               if (statFileOrDirectory (mapIter->first, torrentModTime) < 0)
               {
                  // TODO Handle error condition for failed stat call
                  // for now ignore the error
                  mapIter++;
                  continue;
               }

               if (torrentModTime != mapIter->second->mtime)
               {
                  // torrent file has been updated 
                  // TODO veriy info_hash and update mtime, expiration time, and overTimeAlertIssued (if new expires > timeNow)
                  // log message that expiration time has been extended
                  
               }

               if (timeNow >= mapIter->second->expires)
               {
                  std::vector <libtorrent::peer_info> peers;

                  mapIter->second->torrentHandle.get_peer_info (peers);

                  if (peers.size () == 0)
                  {
                     *sysLogGT << log4cpp::Priority::INFO << "Stop serving:  " << mapIter->first << " with info hash: " << getInfoHash (mapIter->first);
                     (*listIter)->torrentSession->remove_torrent (mapIter->second->torrentHandle);
                     deleteGTOfromQueue (mapIter->first);
                     activeTorrentCollection.erase (mapIter->first);
                     (*listIter)->mapOfSessionTorrents.erase (mapIter++);
                  }
                  else
                  {
                     if (!mapIter->second->overTimeAlertIssued)
                     {
                        *sysLogGT << log4cpp::Priority::INFO << "Overtime serving (" << peers.size () << " actor(s) connected):  " << mapIter->first << " with info hash: " <<  getInfoHash (mapIter->first);
                        mapIter->second->overTimeAlertIssued = true;
                     }
   
                     if (_verbosityLevel > 0)
                     {  
                        std::cerr << std::setw (41) << getFileName (mapIter->first) << " Status: " << server_state_str[torrentStatus.state] << "  expired, but an actor continues to download" << std::endl;
                     }
                     mapIter++;
                  }
               }
               else
               {
                  if (_verbosityLevel > 0)
                  {
                     std::cerr << std::setw (41) << getFileName (mapIter->first) << " Status: " << server_state_str[torrentStatus.state] << "  expires in:  " <<  mapIter->second->expires - time (NULL) << " seconds" << std::endl;
                  }
                  mapIter++;
               }
            }
            listIter++;
         }  // while (...)
      }  // if (nextMaintTime < timeNow)

      if (nextMaintTime < timeNow)
      {
         nextMaintTime = time(NULL) + 60;  // Set the time for the next maintenance window

         if (_verbosityLevel > 0)
         {
            std::cerr << std::endl;
         }
         continue;  // completed a maintenance cycle, skip the 2 second sleep cycle
      }

      usleep (2000000);
   }
}

bool geneTorrent::addTorrentToServingList (std::string pathAndFileName)
{
            checkSessions (); // start or adds sessions if unused session slots exist

            activeSessionRec *workSession = findSession ();
            activeTorrentRec *newTorrRec = new (activeTorrentRec);

            FILE *result = popen (("gtoinfo -x " + pathAndFileName).c_str(), "r");

            if (result == NULL)
            {
               *sysLogGT << log4cpp::Priority::INFO << "Failure running gtoinfo on " << pathAndFileName << " serving using default expiration of 1/1/2037";
               newTorrRec->expires = 2114406000;       // Default to 1/1/2037
            }
            else
            {
               char vBuff[15];
               if (NULL != fgets (vBuff, 15, result))
               {
                  newTorrRec->expires = strtol (vBuff, NULL, 10);
               }
               else
               {
                  *sysLogGT << log4cpp::Priority::INFO << "Failure running gtoinfo on " << pathAndFileName << " or no 'expires on' in GTO, serving using default expiration of 1/1/2037";
                  newTorrRec->expires = 2114406000;       // Default to 1/1/2037
               }
               pclose (result);
            }

            newTorrRec->overTimeAlertIssued = false;

            std::string uuid = pathAndFileName;

            uuid = uuid.substr (0, uuid.rfind ('.'));
            uuid = getFileName (uuid); 

// short term solution to seeder mode issue on uploads
if (statFileOrDirectory ("./" + uuid) != 0)
{
   newTorrRec->torrentParams.seed_mode = false;
}
else
{
   newTorrRec->torrentParams.seed_mode = true;
   newTorrRec->torrentParams.disable_seed_hash = true;
}

            newTorrRec->torrentParams.auto_managed = false;
            newTorrRec->torrentParams.allow_rfc1918_connections = true;

            libtorrent::error_code torrentError;
            newTorrRec->torrentParams.ti = new libtorrent::torrent_info (pathAndFileName, torrentError);

            if (torrentError)
            {
               *sysLogGT << log4cpp::Priority::INFO << "Failure adding " << pathAndFileName << " to Served GTOs, file removed.  Error:  " << torrentError.message () << "(" << torrentError.value () << ")";
               delete newTorrRec;
               deleteGTOfromQueue (pathAndFileName);
               return false;
            }

            newTorrRec->torrentParams.save_path = "./";
            newTorrRec->torrentHandle = workSession->torrentSession->add_torrent (newTorrRec->torrentParams, torrentError);

            if (torrentError)
            {
               *sysLogGT << log4cpp::Priority::INFO << "Failure adding " << pathAndFileName << " to Served GTOs, file removed.  Error:  " << torrentError.message () << "(" << torrentError.value () << ")";
               delete newTorrRec;
               deleteGTOfromQueue (pathAndFileName);
               return false;
            }

            int sslCertSize = newTorrRec->torrentParams.ti->ssl_cert().size();

            if (sslCertSize > 0 && _devMode == false)
            {
               bool status = generateSSLcertAndGetSigned(pathAndFileName, _serverModeCsrSigningUrl, uuid); 

               if (status == false)
               {
                  *sysLogGT << log4cpp::Priority::INFO << "Failure adding " << pathAndFileName << " to Served GTOs, GTO file removed.  Error:  unable to obtain a signed SSL Certificate.";
                  delete newTorrRec;
                  deleteGTOfromQueue (pathAndFileName);
                  return false;
               }
            }

            if (sslCertSize > 0)
            {
               std::string sslCert = _tmpDir + uuid + ".crt";
               std::string sslKey = _tmpDir + uuid + ".key";
            
               newTorrRec->torrentHandle.set_ssl_certificate (sslCert, sslKey, _dhParamsFile);   // no passphrase
            }

            newTorrRec->torrentHandle.resume();

            if (_verbosityLevel > 0)
            {
               std::cerr << "adding " << getFileName (pathAndFileName) << " to files being served" << std::endl;
            }

            *sysLogGT << log4cpp::Priority::INFO << "Begin serving:  " << pathAndFileName << " with info hash: " << getInfoHash(pathAndFileName);
   workSession->mapOfSessionTorrents[pathAndFileName] = newTorrRec;

   return true;
}

geneTorrent::activeSessionRec *geneTorrent::findSession ()
{
   std::list <activeSessionRec *>::iterator listIter = _activeSessions.begin ();
   unsigned int torrentsBeingServed = (*listIter)->mapOfSessionTorrents.size ();
   geneTorrent::activeSessionRec *workingSessionRec = *listIter;

   while (listIter != _activeSessions.end ())
   {
      if (torrentsBeingServed > (*listIter)->mapOfSessionTorrents.size ())
      {
         torrentsBeingServed = (*listIter)->mapOfSessionTorrents.size ();
         workingSessionRec = *listIter;
      }
      listIter++;
   }

   return workingSessionRec;;
}

void geneTorrent::deleteGTOfromQueue (std::string fileName)
{
   int ret = unlink (fileName.c_str ()); 
   if (ret != 0)
   {
      gtError ("Unable to remove ", NO_EXIT, geneTorrent::ERRNO_ERROR, errno);
   }
}

libtorrent::session *geneTorrent::addActiveSession ()
{
   libtorrent::session *sessionNew = new libtorrent::session (*_gtFingerPrint, 0);
   optimizeSession (sessionNew);
   bindSession (sessionNew);

   int portUsed = sessionNew->listen_port ();

   if (portUsed < _portStart + _exposedPortDelta || portUsed > _portEnd + _exposedPortDelta)
   {
      delete sessionNew;
      return NULL;
   }

   return sessionNew;
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

/*
   libtorrent::sha1_hash const& info_hash = torrentInfo.info_hash();

   std::ostringstream infoHash;
   infoHash << info_hash;

   //return infoHash.str();
*/
   return getInfoHash (&torrentInfo);
}

std::string geneTorrent::getInfoHash (libtorrent::torrent_info *torrentInfo)
{
   libtorrent::sha1_hash const& info_hash = torrentInfo->info_hash();

   std::ostringstream infoHash;
   infoHash << info_hash;

   return infoHash.str();
}

bool geneTorrent::generateSSLcertAndGetSigned(std::string torrentFile, std::string signUrl, std::string torrentUUID)
{
   std::string infoHash = getInfoHash(torrentFile);

   if (infoHash.size() < 20)
   {
      return false;
   }

   return acquireSignedCSR (infoHash, signUrl, torrentUUID);
}

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
      std::cerr << "Headers received from the client:  '" << curlResponseHeaders << "'" << std::endl << std::endl;
   }

   curl_easy_cleanup (curl);

   fclose (signedCert);

   return true;
}

void geneTorrent::validateAndCollectSizeOfTorrents (uint64_t &totalBytes, int &totalFiles, int &totalGtos)
{
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
}

void geneTorrent::performGtoUpload (std::string torrentFileName)
{
   if (_verbosityLevel > 0)
   {
      std::cerr << "Sending " << torrentFileName << std::endl;
   }

   libtorrent::session torrentSession (*_gtFingerPrint, 0);
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

   if (sslCertSize > 0 && _devMode == false)
   {
      generateSSLcertAndGetSigned(torrentFileName, _serverModeCsrSigningUrl, uuid);
   }

   if (sslCertSize > 0)
   {
      std::string sslCert = _tmpDir + uuid + ".crt";
      std::string sslKey = _tmpDir + uuid + ".key";
            
      torrentHandle.set_ssl_certificate (sslCert, sslKey, _dhParamsFile);
   }

   torrentHandle.resume();

   time_t cycleTime = 2147483647;
   bool cycleTimerSet = false;
   while (time (NULL) < cycleTime)
   {
      libtorrent::session_status sessionStatus = torrentSession.status ();
      libtorrent::torrent_status torrentStatus = torrentHandle.status ();

      if (torrentStatus.total_payload_upload >= torrentParams.ti->total_size () && cycleTimerSet == false)
      {
         cycleTime = time (NULL) + 15;
         cycleTimerSet = true;
      }

      if (_verbosityLevel > 0)
      {
         char str[500];

         if (torrentStatus.state != libtorrent::torrent_status::queued_for_checking && torrentStatus.state != libtorrent::torrent_status::checking_files)
         {
            double percentComplete = torrentStatus.total_payload_upload / (torrentParams.ti->total_size () * 1.0) * 100.0;
            snprintf (str, sizeof(str), "%% Complete: %5.1f  Status:  %-13s  downloaded:  %s (%s) uploaded:  %s (%s)", (percentComplete > 100.0000000 ? 100.0 : percentComplete), upload_state_str[torrentStatus.state], add_suffix (torrentStatus.total_download).c_str (), add_suffix (torrentStatus.download_rate, "/s").c_str (), add_suffix (torrentStatus.total_upload).c_str (), add_suffix (torrentStatus.upload_rate, "/s").c_str ());
            std::cerr << str << std::endl;
         }
      }
      sleep (5);
   }
}

// do not include files that are not present in _filesToUpload
bool geneTorrent::fileFilter (std::string const fileName)
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
bool geneTorrent::file_filter (boost::filesystem::path const& filename)
{
   std::string fileName = filename.filename().string();

   if (fileName[0] == '.')
      return false;

   geneTorrent *myThis = (geneTorrent *) geneTorr;
   return myThis->fileFilter (fileName);
}
