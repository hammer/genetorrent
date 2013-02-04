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
 * geneTorrent.cpp
 *
 *  Created on: Aug 15, 2011
 *      Author: donavan
 */

#define _DARWIN_C_SOURCE 1

#include "gt_config.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdio>
#include <algorithm>

#ifdef __CYGWIN__
#include <sys/cygwin.h>
#endif /* __CYGWIN__ */

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

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

gtBase::gtBase (gtBaseOpts &opts, opMode mode):
   _progName (opts.m_progName),
   _verbosityLevel (VERBOSE_1), 
   _devMode (false),
   _tmpDir (""), 
   _startUpComplete (false),
   _operatingMode (mode), 
   _successfulTrackerComms (false),

   // Protected members obtained from CLI or CFG.
   _addTimestamps (opts.m_addTimestamps),
   _allowedServersSet (opts.m_allowedServersSet),
   _authToken (""),
   _curlVerifySSL (opts.m_curlVerifySSL),
   _exposedPortDelta (opts.m_exposedPortDelta),
   _inactiveTimeout (opts.m_inactiveTimeout),
   _ipFilter (opts.m_ipFilter),
   _logDestination (opts.m_logDestination),
   _logToStdErr (opts.m_logToStdErr),
   _portEnd (opts.m_portEnd),
   _portStart (opts.m_portStart),
   _rateLimit (opts.m_rateLimit),
   _use_null_storage (opts.m_use_null_storage),
   _use_zero_storage (opts.m_use_zero_storage),

   // Private members obtained from CLI or CFG.
   _bindIP (opts.m_bindIP),
   _confDir (opts.m_confDir),
   _exposedIP (opts.m_exposedIP),
   _logMask (opts.m_logMask),
   _peerTimeout (opts.m_peerTimeout)
{
   geneTorrCallBackPtr = (void *) this;          // Set the global geneTorr pointer that allows fileFilter callbacks from libtorrent

   pthread_mutex_init (&callBackLoggerLock, NULL);

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

   _verbosityLevel = global_verbosity;

   _dhParamsFile = _confDir + "/" + DH_PARAMS_FILE;
   if (statFile (_dhParamsFile) != 0)
   {
      gtError ("Failure opening SSL DH Params file:  " + _dhParamsFile, 202, ERRNO_ERROR, errno);
   }

   OpenSSL_add_all_algorithms();
   ERR_load_crypto_strings();
   initSSLattributes();

   if (4096 != RAND_load_file("/dev/urandom", 4096))
   {
      gtError ("Failure opening /dev/urandom to prime the OpenSSL PRNG", 202);
   }

   if (!_devMode)
   {
      mkTempDir();
   }

   loadCredentialFile (opts.m_credentialPath);

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

   _gtFingerPrint = new libtorrent::fingerprint (gtTag.c_str(), strtol (strToken.getToken (1).c_str (), NULL, 10), strtol (strToken.getToken (2).c_str (), NULL, 10), strtol (strToken.getToken (3).c_str (), NULL, 10), 0);
}

std::string gtBase::version_str = VERSION;

void gtBase::startUpMessage (std::string app_name)
{
   std::ostringstream msg;

   if (_devMode)
   {
      msg << "[devModeOverride] ";
   }

   msg << "Starting " << app_name << "-" << gtBase::version_str;

   Log (PRIORITY_NORMAL, "%s (using tmpDir = %s)", msg.str().c_str(), _tmpDir.c_str());

   screenOutput ("Welcome to " << app_name << "-" << gtBase::version_str << ".",
      VERBOSE_1);
}

void gtBase::initSSLattributes ()
{
   // If you add entries here you must adjust  CSR_ATTRIBUTE_ENTRY_COUNT in gtDefs.h
   attributes[0].key = "countryName";
   attributes[0].value = "US";

   attributes[1].key = "stateOrProvinceName";
   attributes[1].value = "CA";

   attributes[2].key = "localityName";
   attributes[2].value = "San Jose";

   attributes[3].key = "organizationName";
   attributes[3].value = "ploaders, Inc";

   attributes[4].key = "organizationalUnitName";
   attributes[4].value = "staff";

   attributes[5].key = "commonName";
   attributes[5].value = "www.uploadersinc.com";

   attributes[6].key = "emailAddress";
   attributes[6].value = "root@uploadersinc.com";
   // If you add entries here you must adjust  CSR_ATTRIBUTE_ENTRY_COUNT in gtDefs.h
}

// 
void gtBase::setTempDir ()     
{
   // Setup the static part of the Temp dir used to store ssl bits
   std::ostringstream pathPart;
   pathPart << "/GeneTorrent-" << getuid() << "-" << std::setfill('0') << std::setw(6) << getpid() << "-" << time(NULL);

   // On Uniux, returns environment's TMPDIR, TMP, TEMP, or TEMPDIR,
   //    or, failing that, /tmp
   // On Windows, return windows temp directory
   boost::filesystem::path p;
   try
   {
      p = boost::filesystem::temp_directory_path();
   }
   catch (boost::filesystem::filesystem_error e)
   {
      std::ostringstream errorStr;
      errorStr << "Could not find temp directory.  Set the TMPDIR "
         "environment variable to specify a temp directory for GeneTorrent. "
         "Error: ";
      errorStr << e.what ();

      gtError (errorStr.str (), 66);
   }

   std::string tempPath = p.string();

   _tmpDir = sanitizePath (tempPath + pathPart.str ());
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
std::string gtBase::loadCSRfile (std::string csrFileName)
{
   std::string fileContent = "";
   
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
gtBase::~gtBase ()
{
   cleanupTmpDir ();
   delete _gtFingerPrint;
   gtLogger::delete_globallog();
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

      if (global_gtAgentMode)
      {
         std::cout << logMessage.str() << std::endl;
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
void gtBase::cleanupTmpDir()
{
   if (! _startUpComplete)
      return;

   if (!_devMode)
   {
      try
      {
         boost::filesystem::remove_all(_tmpDir);
      }
      catch (boost::filesystem::filesystem_error e)
      {
         Log (PRIORITY_NORMAL, "Failed to clean up temp directory");
      }
   }
}

libtorrent::session * gtBase::makeTorrentSession ()
{
   libtorrent::session *torrentSession = NULL;

   try
   {
      torrentSession = new libtorrent::session(*_gtFingerPrint, 0, libtorrent::alert::all_categories);
      optimizeSession (torrentSession);
      bindSession (torrentSession);
   }
   catch (boost::system::system_error e)  // thrown by boost::asio if a pthread_create
                                          // fails due to user/system process limits or OOM
   {
      gtError ("torrent session initialization error: " + std::string(e.what()), ERROR_NO_EXIT, DEFAULT_ERROR);
   }

   return torrentSession;
}

// 
void gtBase::bindSession (libtorrent::session *torrentSession)
{
   libtorrent::error_code torrentError;

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

   if (_bindIP.size () > 0)
   {
      torrentSession->listen_on (std::make_pair (_portStart, _portEnd), torrentError, _bindIP.c_str (), 0);
   }
   else
   {
      torrentSession->listen_on (std::make_pair (_portStart, _portEnd), torrentError, NULL, 0);
   }

   if (torrentError)
   {
      Log (PRIORITY_NORMAL, "failure to set listen address or port in torrent session. (code %d) %s",
         torrentError.value(), torrentError.message().c_str());
   }

   // the ip address sent to the tracker with the announce
   if (_exposedIP.size () > 0) 
   {
      settings.announce_ip = _exposedIP;
   }

   if (_peerTimeout > 0)
      settings.peer_timeout = _peerTimeout;

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

   if (_operatingMode != SERVER_MODE && _allowedServersSet)
      settings.apply_ip_filter_to_trackers = true;
   else
      settings.apply_ip_filter_to_trackers = false;

   settings.no_atime_storage = false;
   settings.max_queued_disk_bytes = 256 * 1024 * 1024;

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

   settings.alert_queue_size = 10000;

   if (_operatingMode == SERVER_MODE)
   {
      settings.send_buffer_watermark = 256 * 1024 * 1024;

      // put 1.5 seconds worth of data in the send buffer this gives the disk I/O more heads-up on disk reads, and can maximize throughput
      settings.send_buffer_watermark_factor = 150;

      // don't retry peers if they fail once. Let them connect to us if they want to
      settings.max_failcount = 1;
   }

   torrentSession->set_settings (settings);

   if (_bindIP.size() || _exposedIP.size())
   {
      if (_bindIP.size())
      {
         try
         {
            _ipFilter.add_rule (boost::asio::ip::address::from_string(_bindIP), boost::asio::ip::address::from_string(_bindIP), libtorrent::ip_filter::blocked);
         }
         catch (boost::system::system_error e)
         {  
            std::ostringstream messageBuff;
            messageBuff << "invalid '--bind-ip' address of:  " << _bindIP << " caused an exception:  " << e.what();
            Log (PRIORITY_HIGH, "%s", messageBuff.str().c_str());
            exit(98);
         }
      }

      if (_exposedIP.size())
      {
         try
         {
            _ipFilter.add_rule (boost::asio::ip::address::from_string(_exposedIP), boost::asio::ip::address::from_string(_exposedIP), libtorrent::ip_filter::blocked);
         }
         catch (boost::system::system_error e)
         {  
            std::ostringstream messageBuff;
            messageBuff << "invalid '--advertised-ip' address of:  " << _exposedIP << " caused an exception:  " << e.what();
            Log (PRIORITY_HIGH, "%s", messageBuff.str().c_str());
            exit(98);
         }
      }

   }

   torrentSession->set_ip_filter(_ipFilter);
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



void gtBase::processSSLError (std::string message)
{
   std::string errorMessage = message;

   unsigned long sslError;
   char sslErrorBuf[150];

   while (0 != ( sslError = ERR_get_error()))
   {
      ERR_error_string_n (sslError, sslErrorBuf, sizeof (sslErrorBuf));
      errorMessage += sslErrorBuf + std::string (", ");
   }

   if (_operatingMode != SERVER_MODE)
   {
      gtError (errorMessage, SSL_ERROR_EXIT_CODE);
   }
   else
   {
      gtError (errorMessage, ERROR_NO_EXIT);
   }
}

// 
bool gtBase::generateCSR (std::string uuid)
{
   RSA *rsaKey;

   // Generate RSA Key
   rsaKey =  RSA_generate_key(RSA_KEY_SIZE, RSA_F4, NULL, NULL);

   if (NULL == rsaKey)
   {
      processSSLError ("Failure generating OpenSSL Key:  ");   // if this returns server mode is active and we bail on this attempt
      return false;
   }

   EVP_PKEY *pKey;

   // Initialize private key store
   pKey = EVP_PKEY_new();

   if (NULL == pKey)
   {
      RSA_free(rsaKey);
      processSSLError ("Failure initializing OpenSSL EVP object:  ");   // if this returns server mode is active and we bail on this attempt
      return false;
   }    

   // Add key to private key store
   if (!(EVP_PKEY_set1_RSA (pKey, rsaKey)))
   {
      EVP_PKEY_free (pKey);
      RSA_free (rsaKey);
      processSSLError ("Failure adding OpenSSL key to EVP object:  ");   // if this returns server mode is active and we bail on this attempt
      return false;
   }    

   X509_REQ *csr;

   // Allocate a CSR
   csr = X509_REQ_new();

   if (NULL == csr)
   {
      EVP_PKEY_free (pKey);
      RSA_free(rsaKey);
      processSSLError ("Failure allocating OpenSSL CSR:  ");   // if this returns server mode is active and we bail on this attempt
      return false;
   }    

   // set the public key part of the SCR
   if (!(X509_REQ_set_pubkey (csr, pKey)))
   {
      X509_REQ_free (csr); 
      EVP_PKEY_free (pKey);
      RSA_free (rsaKey);
      processSSLError ("Failure adding public key to OpenSSL CSR:  ");   // if this returns server mode is active and we bail on this attempt
      return false;
   }    
  
   // allocate subject attribute structure 
   X509_NAME *subject;

   subject = X509_NAME_new();

   if (NULL == subject)
   {
      X509_REQ_free (csr); 
      EVP_PKEY_free (pKey);
      RSA_free(rsaKey);
      processSSLError ("Failure allocating OpenSSL X509 Name Structure:  ");   // if this returns server mode is active and we bail on this attempt
      return false;
   }    

   // Add attributes to subject
   for (int i = 0; i < CSR_ATTRIBUTE_ENTRY_COUNT; i++)
   {
      if (!X509_NAME_add_entry_by_txt(subject, attributes[i].key.c_str(), MBSTRING_ASC, (const unsigned char *)attributes[i].value.c_str(), -1, -1, 0))
      {
         X509_NAME_free (subject);
         X509_REQ_free (csr); 
         EVP_PKEY_free (pKey);
         RSA_free(rsaKey);
         processSSLError ("Failure adding " + attributes[i].key + " to OpenSSL X509 Name Structure:  ");   // if this returns server mode is active and we bail on this attempt
         return false;
      }
   }

   // Add the subject to the CSR
   if (!(X509_REQ_set_subject_name(csr, subject)))
   {
      X509_NAME_free (subject);
      X509_REQ_free (csr); 
      EVP_PKEY_free (pKey);
      RSA_free(rsaKey);
      processSSLError ("Failure adding X509 Name Structure to CSR:  ");   // if this returns server mode is active and we bail on this attempt
      return false;
   }

   EVP_MD *digest;

   digest = (EVP_MD *)EVP_sha1();

   if (NULL == digest)
   {
      X509_NAME_free (subject);
      X509_REQ_free (csr); 
      EVP_PKEY_free (pKey);
      RSA_free(rsaKey);
      processSSLError ("Failure allocating OpenSSL sha1 digest:  ");   // if this returns server mode is active and we bail on this attempt
      return false;
   }

   if (!(X509_REQ_sign(csr, pKey, digest)))
   {
      X509_NAME_free (subject);
      X509_REQ_free (csr); 
      EVP_PKEY_free (pKey);
      RSA_free(rsaKey);
      processSSLError ("Failure creating OpenSSL CSR:  ");   // if this returns server mode is active and we bail on this attempt
      return false;
   }

   FILE *outputFile;
   std::string csrPathAndFile = _tmpDir + uuid + ".csr";

   if (NULL == (outputFile = fopen(csrPathAndFile.c_str(), "w")))
   {
      X509_NAME_free (subject);
      X509_REQ_free (csr); 
      EVP_PKEY_free (pKey);
      RSA_free(rsaKey);
      processSSLError ("Failure opening " + csrPathAndFile + " for output.  Unable to write OpenSSL CSR.");   // if this returns server mode is active and we bail on this attempt
      return false;
   }
 
   if (PEM_write_X509_REQ(outputFile, csr) != 1)
   {
      fclose(outputFile);
      X509_NAME_free (subject);
      X509_REQ_free (csr); 
      EVP_PKEY_free (pKey);
      RSA_free(rsaKey);
      processSSLError ("Failure writing OpenSSL CSR to " + csrPathAndFile + ":  ");   // if this returns server mode is active and we bail on this attempt
      return false;
   }
   fclose(outputFile);

   std::string pKeyPathAndFile = _tmpDir + uuid + ".key";

   if (NULL == (outputFile = fopen(pKeyPathAndFile.c_str(), "w")))
   {
      X509_NAME_free (subject);
      X509_REQ_free (csr); 
      EVP_PKEY_free (pKey);
      RSA_free(rsaKey);
      processSSLError ("Failure opening " + pKeyPathAndFile + " for output.  Unable to write OpenSSL private key.");   // if this returns server mode is active and we bail on this attempt
      return false;
   }

   if (PEM_write_PrivateKey(outputFile, pKey, NULL, NULL, 0, 0, NULL) != 1)
   {
      fclose(outputFile);
      X509_NAME_free (subject);
      X509_REQ_free (csr); 
      EVP_PKEY_free (pKey);
      RSA_free(rsaKey);
      processSSLError ("Failure writing OpenSSL Private Key to " + pKeyPathAndFile + ":  ");   // if this returns server mode is active and we bail on this attempt
      return false;
   }
   fclose(outputFile);

   // Clean up memory allocated
   X509_NAME_free (subject);
   X509_REQ_free (csr); 
   EVP_PKEY_free (pKey);
   RSA_free (rsaKey);

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

FILE *gtBase::createCurlTempFile (std::string &tempFilePath)
{
   std::string t = _tmpDir + "gt-curl-response-XXXXXX";
   char tmpname[4096];
   FILE *curl_stderr_fp;

   strncpy (tmpname, t.c_str(), t.size() + 1);        // extra byte includes the NULL which eliminates the needs to bzero (tmpname, ....)

   int curl_stderr = mkstemp (tmpname);
   if (curl_stderr < 0)
   {
      Log (PRIORITY_HIGH, "Failed to create CURL temp file:  %s [%s (%d)]", tmpname, strerror (errno), errno);
      return NULL;
   }

   // fdopen Returns NULL on error
   curl_stderr_fp = fdopen (curl_stderr, "w+");

   tempFilePath = std::string (tmpname);
   return curl_stderr_fp;
}

void gtBase::finishCurlTempFile (FILE *curl_stderr_fp, std::string tempFilePath)
{
   if (curl_stderr_fp == NULL)
   {
      Log (PRIORITY_HIGH, "finishCurlTempFile called with NULL file pointer.");
      return;
   }

   if (fseek (curl_stderr_fp, 0, SEEK_END) < 0)
   {
      Log (PRIORITY_HIGH, "Failed to seek in CURL temp file.");
      return;
   }

   long size = ftell (curl_stderr_fp);
   rewind (curl_stderr_fp);

   char *curlMessage = (char *) malloc (size + 1);
   if (curlMessage == NULL)
   {
      fclose (curl_stderr_fp);
      if (unlink (tempFilePath.c_str()) < 0)
      {
         Log (PRIORITY_HIGH, "Failed to delete CURL temp file.");
      }
   }

   int count = fread(curlMessage, 1, size, curl_stderr_fp);
   // Add null-terminator
   curlMessage[size] = '\0';

   if (count)
   {
      screenOutput ("CURL library diagnostic information: " <<
         curlMessage << std::endl, VERBOSE_2);
   }
   else
   {
      Log (PRIORITY_HIGH, "Failed to read CURL temp file.");
   }

   free (curlMessage);

   fclose (curl_stderr_fp);
   if (unlink (tempFilePath.c_str()) < 0)
   {
      Log (PRIORITY_HIGH, "Failed to delete CURL temp file.");
   }
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

   checkIPFilter (CSRSignURL);

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

   if (!_curlVerifySSL)
   {
      curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 0);
      curl_easy_setopt (curl, CURLOPT_SSL_VERIFYHOST, 0);
   }

   curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, errorBuffer);
   curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, NULL);
   curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION, &curlCallBackHeadersWriter);
   curl_easy_setopt (curl, CURLOPT_MAXREDIRS, 15);
   curl_easy_setopt (curl, CURLOPT_WRITEDATA, signedCert);
   curl_easy_setopt (curl, CURLOPT_WRITEHEADER, &curlResponseHeaders);
   curl_easy_setopt (curl, CURLOPT_NOSIGNAL, (long)1);
   curl_easy_setopt (curl, CURLOPT_POST, (long)1);
#ifdef __CYGWIN__
	std::string winInst = getWinInstallDirectory () + "/cacert.pem";
	curl_easy_setopt (curl, CURLOPT_CAINFO, winInst.c_str ());
#endif /* __CYGWIN__ */

   struct curl_httppost *post=NULL;
   struct curl_httppost *last=NULL;

   curl_formadd (&post, &last, CURLFORM_COPYNAME, "token", CURLFORM_COPYCONTENTS, _authToken.c_str(), CURLFORM_END);
   curl_formadd (&post, &last, CURLFORM_COPYNAME, "cert_req", CURLFORM_COPYCONTENTS, csrData.c_str(), CURLFORM_END);
   curl_formadd (&post, &last, CURLFORM_COPYNAME, "info_hash", CURLFORM_COPYCONTENTS, info_hash.c_str(), CURLFORM_END);

   curl_easy_setopt (curl, CURLOPT_HTTPPOST, post);

   // CGHUBDEV-22: Set CURL timeouts to 20 seconds
   int timeoutVal = 20;
   int connTime = 20;

   curl_easy_setopt (curl, CURLOPT_URL, CSRSignURL.c_str());
   curl_easy_setopt (curl, CURLOPT_TIMEOUT, timeoutVal);
   curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, connTime);

   std::string tmppath;
   FILE *curl_stderr_fp = createCurlTempFile(tmppath);

   if (curl_stderr_fp)
   {
      curl_easy_setopt (curl, CURLOPT_VERBOSE, 1);
      curl_easy_setopt (curl, CURLOPT_STDERR, curl_stderr_fp);
   }
   else if (_verbosityLevel > VERBOSE_2)
   {
      curl_easy_setopt (curl, CURLOPT_VERBOSE, 1);
   }

   CURLcode res;
   int retries = 5;

   while (retries)
   {
      res = curl_easy_perform (curl);

      if (res != CURLE_SSL_CONNECT_ERROR && res != CURLE_OPERATION_TIMEDOUT)
      {
         // Only retry on SSL connect errors or timeouts in case the other end is temporarily overloaded.
         break;
      }

      // Give the other end time to become less loaded.
      sleep (2);

      retries--;

      if (retries)
      {
         screenOutput ("Retrying CSR signing for UUID: " + uuid, VERBOSE_1);
      }
   }

   fclose (signedCert);

   bool successfulPerform = processCurlResponse (curl, res, certFileName, CSRSignURL, uuid, "Problem communicating with GeneTorrent Executive while attempting a CSR signing transaction for UUID:", retries);

   curl_formfree (post);
   curl_easy_cleanup (curl);

   finishCurlTempFile (curl_stderr_fp, tmppath);
   screenOutput ("Headers received from the client:  '" << curlResponseHeaders << "'" << std::endl, VERBOSE_2);

   if (!successfulPerform)
   {
      if (_operatingMode != SERVER_MODE)
      {
         exit (1);
      }
   }

   return successfulPerform;
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

void gtBase::removeFile (std::string fileName)
{
   int ret = unlink (fileName.c_str ());

   if (ret != 0)
   {
      gtError ("Unable to remove ", NO_EXIT, gtBase::ERRNO_ERROR, errno);
   }
}

std::string gtBase::makeTimeStamp ()
{
   const int BUFF_SIZE = 25;
   char buffer[BUFF_SIZE];
   char tail[BUFF_SIZE];
   char secBuff[BUFF_SIZE];
   struct timeval tp;

   gettimeofday (&tp, NULL);

   snprintf (secBuff, BUFF_SIZE, ".%03d", int (tp.tv_usec/1000));

   struct tm newTime;

   localtime_r (&tp.tv_sec, &newTime);
   strftime (buffer, BUFF_SIZE, "%m/%d-%T", &newTime);
   strftime (tail, BUFF_SIZE, "%z", &newTime);

   return buffer + std::string (secBuff) + tail;
}

bool gtBase::processHTTPError (int errorCode, std::string fileWithErrorXML, int retryCount, int optionalExitCode)
{
   XQilla xqilla;

   try
   {
      AutoDelete <XQQuery> query (xqilla.parse (X("//CGHUB_error/usermsg/text()|//CGHUB_error/effect/text()|//CGHUB_error/remediation/text()")));
      AutoDelete <DynamicContext> context (query->createDynamicContext ());

      Sequence seq = context->resolveDocument (X(fileWithErrorXML.c_str()));

      if (!seq.isEmpty () && seq.first()->isNode ())
      {
         context->setContextItem (seq.first ());
         context->setContextPosition (1);
         context->setContextSize (1);
      }
      else
      {
         throw ("Empty set, likely invalid xml");
      }

      Result result = query->execute (context);
      Item::Ptr item;
   
      item = result->next (context);

      if (!item)
      {
         throw ("Empty item, no matching xml nodes");
      }

      std::string userMsg = UTF8(item->asString(context));
      item = result->next (context);

      if (!item)
      {
         throw ("Empty item, no matching xml nodes");
      }

      std::string effect = UTF8(item->asString(context));
      item = result->next (context);

      if (!item)
      {
         throw ("Empty item, no matching xml nodes");
      }

      std::string remediation = UTF8(item->asString(context));

      if (!(userMsg.size() && effect.size() && remediation.size()))
      {
         throw ("Incomplete message set.");
      }

      std::ostringstream logMessage;

      logMessage << userMsg << "  " << effect << "  " << remediation << std::endl;

      if (retryCount > 0)
      { 
         Log (PRIORITY_HIGH, "%s", logMessage.str().c_str());
      }
      else
      { 
         gtError ("Error:  " + logMessage.str(), 203);
      }

      if (GTO_FILE_DOWNLOAD_EXTENSION == fileWithErrorXML.substr (fileWithErrorXML.size() - 1))
      {
         removeFile (fileWithErrorXML);
      }
      
   }
   catch (...)
   {
      // Catch any error from parsing and return false to indicate processing is not complete, e.g., no xml, invalid xml, etc.
      if (GTO_FILE_DOWNLOAD_EXTENSION == fileWithErrorXML.substr (fileWithErrorXML.size() - 1))
      {
         std::string newFileName = fileWithErrorXML.substr (0, fileWithErrorXML.size() - 1) + GTO_ERROR_DOWNLOAD_EXTENSION;

         int result = rename (fileWithErrorXML.c_str(), newFileName.c_str());
         if (0 != result)
         {
            gtError ("Unable to rename " + fileWithErrorXML + " to " + newFileName, 203, ERRNO_ERROR, errno);
         }

         std::ostringstream logMessage;
         logMessage << "error processing failure with file:  " << newFileName <<  ", review the contents of the file.";
         Log (PRIORITY_HIGH, "%s", logMessage.str().c_str());

         std::ifstream errorFile;
         errorFile.open (newFileName.c_str ());
         while (errorFile.good ())
         {
            char line[200];
             errorFile.getline (line, sizeof (line) - 1);
            std::cerr << line << std::endl;
         }
         errorFile.close ();

      }
      return false;
   }

   if (ERROR_NO_EXIT == optionalExitCode)
   {
      return true;
   }

   exit (optionalExitCode);
}
 
bool gtBase::processCurlResponse (CURL *curl, CURLcode result, std::string fileName, std::string url, std::string uuid, std::string defaultMessage, int retryCount)
{
   if (result != CURLE_OK)
   {
      if (GTO_FILE_DOWNLOAD_EXTENSION == fileName.substr (fileName.size() -1))
      {
         removeFile (fileName);
      }
      
      gtError (defaultMessage + uuid, ERROR_NO_EXIT, gtBase::CURL_ERROR, result, "URL:  " + url);
      return false;
   }

   long code;
   result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

   if (result != CURLE_OK)
   {
      if (GTO_FILE_DOWNLOAD_EXTENSION == fileName.substr (fileName.size() -1))
      {
         removeFile (fileName);
      }
      
      gtError (defaultMessage + uuid, ERROR_NO_EXIT, gtBase::DEFAULT_ERROR, 0, "URL:  " + url);
      return false;
   }

// TODO, use content type
   if (code != 200)
   {
      // returns true if successfully used the XML in the file,
      // otherwise log generic error with GTError
      if (!processHTTPError (code, fileName, retryCount, ERROR_NO_EXIT))
      {
         gtError (defaultMessage + uuid, ERROR_NO_EXIT, gtBase::HTTP_ERROR, code, "URL:  " + url);
      }
      return false;   // return false to indicate failed curl transaction
   }
   return true;    // success curl transaction
}

// Small wrapper around time () to initialize
// or update a time_t struct
time_t gtBase::timeout_update (time_t *timer)
{
   return time (timer);
}

// Check whether timeout is expired
// It is expired if all of the following:
//    this->_inactiveTimeout > 0
//    timer != NULL
//    time elapsed since last timer update
//       is > _inactiveTimeout (* 60 for seconds)
bool gtBase::timeout_check_expired (time_t *timer)
{
   if (_inactiveTimeout <= 0 || !timer)
      return false;

   if (time (NULL) - *timer > _inactiveTimeout * 60)
      return true;

   return false;
}

std::string gtBase::authTokenFromURI (std::string url)
{

   char errorBuffer[CURL_ERROR_SIZE + 1] = {'\0'};

   std::string curlResponseHeaders = "";
   std::string curlResponseData = "";

   checkIPFilter (url);

   CURL *curl;
   curl = curl_easy_init ();

   if (!curl)
      gtError ("libCurl initialization failure", 201);

   if (!_curlVerifySSL)
   {
      curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 0);
      curl_easy_setopt (curl, CURLOPT_SSL_VERIFYHOST, 0);
   }

   curl_easy_setopt (curl, CURLOPT_ERRORBUFFER, errorBuffer);
   curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, &curlCallBackHeadersWriter);
   curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION, &curlCallBackHeadersWriter);
   curl_easy_setopt (curl, CURLOPT_MAXREDIRS, 15);
   curl_easy_setopt (curl, CURLOPT_WRITEDATA, &curlResponseData);
   curl_easy_setopt (curl, CURLOPT_WRITEHEADER, &curlResponseHeaders);
   curl_easy_setopt (curl, CURLOPT_NOSIGNAL, (long)1);
   curl_easy_setopt (curl, CURLOPT_HTTPGET, (long)1);
#ifdef __CYGWIN__
	std::string winInst = getWinInstallDirectory () + "/cacert.pem";
	curl_easy_setopt (curl, CURLOPT_CAINFO, winInst.c_str ());
#endif /* __CYGWIN__ */

   // CGHUBDEV-22: Set CURL timeouts to 20 seconds
   int timeoutVal = 20;
   int connTime = 20;

   curl_easy_setopt (curl, CURLOPT_URL, url.c_str());
   curl_easy_setopt (curl, CURLOPT_TIMEOUT, timeoutVal);
   curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, connTime);

   std::string tmppath;
   FILE *curl_stderr_fp = createCurlTempFile(tmppath);

   if (curl_stderr_fp)
   {
      curl_easy_setopt (curl, CURLOPT_VERBOSE, 1);
      curl_easy_setopt (curl, CURLOPT_STDERR, curl_stderr_fp);
   }
   else if (_verbosityLevel > VERBOSE_2)
   {
      curl_easy_setopt (curl, CURLOPT_VERBOSE, 1);
   }

   CURLcode res;
   long code = -1;

   res = curl_easy_perform (curl);
   curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

   if (res != CURLE_OK || code < 200 || code >= 300 ||
      curlResponseData.size() < 1)
   {
      std::ostringstream errormsg;
      errormsg << "Failed to download authentication token from provided URI.";
      if (code != 0)
         errormsg << " Response code = " << code << ".";
      if (strlen(errorBuffer))
         errormsg << " Error = " << errorBuffer;
      gtError (errormsg.str(), 65);
   }

   curl_easy_cleanup (curl);

   finishCurlTempFile (curl_stderr_fp, tmppath);
   screenOutput ("Headers received from the client:  '" << curlResponseHeaders << "'" << std::endl, VERBOSE_2);

   return curlResponseData;
}

// Checks whether a given (WSI) URL resolves to an IP address that is allowed
// by the filter, and exits with a command line error if not
void gtBase::checkIPFilter (std::string url)
{
   if (!_allowedServersSet)
      return;

   const std::string proto = "://";
   std::string hostName;
   size_t hostStart, hostEnd;
   if ((hostStart = url.find (proto)) != std::string::npos)
   {
      hostStart += proto.size();
      if ((hostEnd = url.find ("/", hostStart + 1)) !=
         std::string::npos)
         hostName = url.substr(hostStart, hostEnd - hostStart);
   }

   if (hostName.size() == 0)
      gtError ("Bad URL given for WSI call. Could not extract hostname"
               " from URL: " + url + ".", 59);

   boost::system::error_code ec;
   boost::asio::io_service io_service;
   boost::asio::ip::tcp::resolver resolver(io_service);
   boost::asio::ip::tcp::resolver::query query(hostName, "");
   for(boost::asio::ip::tcp::resolver::iterator i = resolver.resolve(query, ec);
                               i != boost::asio::ip::tcp::resolver::iterator();
                               ++i)
   {
      if (ec)
         continue;
       boost::asio::ip::tcp::endpoint end = *i;
       if (_ipFilter.access (end.address()))
          gtError ("IP address of server in WSI call is outside of"
                   " the allowed server range on this system.  Host: "
                   + hostName, 59);
   }
}

void gtBase::loadCredentialFile (std::string credsPathAndFile)
{
   if (credsPathAndFile.size() == 0)
      return;

   if (credsPathAndFile.find("http://")  == 0 ||
       credsPathAndFile.find("https://") == 0 ||
       credsPathAndFile.find("ftp://")   == 0 ||
       credsPathAndFile.find("ftps://")  == 0)
   {
      _authToken = authTokenFromURI (credsPathAndFile);
      return;
   }

   std::ifstream credFile;

   credFile.open (credsPathAndFile.c_str(), std::ifstream::in);

   if (!credFile.good ())
   {
      gtError ("credentials file not found (or is not readable):  "
               + credsPathAndFile, 55);
   }

   try
   {
      credFile >> _authToken;
   }
   catch (...)
   {
      gtError ("credentials file not found (or is not readable):  "
               + credsPathAndFile, 56);
   }

   credFile.close ();
}
