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
 * gtServer.cpp
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

#include "gtServer.h"
#include "stringTokenizer.h"
#include "gtDefs.h"
#include "geneTorrentUtils.h"
#include "gtLog.h"
#include "loggingmask.h"

static char const* server_state_str[] = {
   "checking (q)",                    // queued_for_checking,
   "checking",                        // checking_files,
   "dl metadata",                     // downloading_metadata,
   "receiving",                       // downloading,
   "finished",                        // finished,
   "serving",                         // seeding,
   "allocating",                      // allocating,
   "checking (r)"                     // checking_resume_data
};

extern void *geneTorrCallBackPtr; 

gtServer::gtServer (boost::program_options::variables_map &vm) : gtBase (vm, SERVER_MODE), _serverQueuePath (""), _serverDataPath (""), _serverModeCsrSigningUrl (""), _activeSessions (), _maxActiveSessions (_portEnd - _portStart + 1)
{
   pcfacliServer (vm);
   pcfacliQueue (vm);
   pcfacliSecurityAPI (vm);

   checkCredentials ();

   _startUpComplete = true;
}

void gtServer::pcfacliServer (boost::program_options::variables_map &vm)
{
   _serverDataPath = sanitizePath (vm[SERVER_CLI_OPT].as<std::string>());

   if (_serverDataPath.size() == 0)
   {
      commandLineError ("command line or config file contains no value for '" + SERVER_CLI_OPT + "'");
   }

   if (statFileOrDirectory (_serverDataPath) != 0)
   {
      commandLineError ("unable to opening directory '" + _serverDataPath + "'");
   }
}

void gtServer::pcfacliQueue (boost::program_options::variables_map &vm)
{
   if (vm.count (QUEUE_CLI_OPT) < 1)
   {
      commandLineError ("Must include a queue path when operating in server mode, -c or --" + QUEUE_CLI_OPT);
   }

   _serverQueuePath = sanitizePath (vm[QUEUE_CLI_OPT].as<std::string>());

   if (_serverQueuePath.size() == 0)
   {
      commandLineError ("command line or config file contains no value for '" + QUEUE_CLI_OPT + "'");
   }

   if (statFileOrDirectory (_serverQueuePath) != 0)
   {
      commandLineError ("unable to opening directory '" + _serverQueuePath + "'");
   }
}

void gtServer::pcfacliSecurityAPI (boost::program_options::variables_map &vm)
{
   if (vm.count (SECURITY_API_CLI_OPT) < 1)
   {
      commandLineError ("Must include a full URL to the security API services in server mode, --" + SECURITY_API_CLI_OPT);
   }

   _serverModeCsrSigningUrl = vm[SECURITY_API_CLI_OPT].as<std::string>();

   if (_serverModeCsrSigningUrl.size() == 0)
   {
      commandLineError ("command line or config file contains no value for '" + SECURITY_API_CLI_OPT + "'");
   }
}

void gtServer::getFilesInQueueDirectory (vectOfStr &files)
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

void gtServer::checkSessions ()
{
   gtServer::activeSessionRec *workingSessionRec = NULL;

   while (_activeSessions.size () == 0) // Loop until a session is added
   {
      libtorrent::session *workingSession = addActiveSession ();

      if (workingSession)
      {
         workingSessionRec = new gtServer::activeSessionRec;
         workingSessionRec->torrentSession = workingSession;

         _activeSessions.push_back (workingSessionRec);
      }
      else
      {
         Log (PRIORITY_HIGH, "Unable to listen on any ports between %d and %d (system level ports).  GeneTorrent can not Serve data until at least one port is available.", _portStart, _portEnd);
         sleep (3); // give OS chance to clean up ports desired by this process
      }
   }

   if (_activeSessions.size () < _maxActiveSessions && workingSessionRec == NULL) // Try one time to add a new session
   {
      libtorrent::session *workingSession = addActiveSession ();

      if (workingSession)
      {
         workingSessionRec = new gtServer::activeSessionRec;
         workingSessionRec->torrentSession = workingSession;

         _activeSessions.push_back (workingSessionRec);
      }
   }
}

//
//  Managing torrents in Server mode is moderately complex
//
//  SessionRec->
//
//
//
void gtServer::run ()
{
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

         if (actTorrentIter == activeTorrentCollection.end ()) // gto is not in the set of active torrents
         {
            if (addTorrentToServingList (*vectIter))  // Successfully added to a serving session
            {
               activeTorrentCollection.insert (*vectIter);
            }
         } 
         vectIter++;
      }

      time_t timeNow = time(NULL);      

      if (nextMaintTime <= timeNow)
      {
         nextMaintTime = time(NULL) + 60;  // Set the time for the next maintenance window

         servedGtosMaintenance (timeNow, activeTorrentCollection);

         if (_verbosityLevel > 0)
         {
            screenOutput ("");
         }
         continue;  // completed a maintenance cycle, skip the 2 second sleep cycle
      }

      processServerModeAlerts();
   }
}

void gtServer::processServerModeAlerts ()
{
   libtorrent::ptime endMonitoring = libtorrent::time_now_hires() + libtorrent::seconds(2);

   while (libtorrent::time_now_hires() < endMonitoring)
   {
      std::list <activeSessionRec *>::iterator listIter = _activeSessions.begin ();

      while (listIter != _activeSessions.end ())
      {
         checkAlerts (*(*listIter)->torrentSession);
         listIter++;
      }
      usleep(ALERT_CHECK_PAUSE_INTERVAL);
   }
}

void gtServer::servedGtosMaintenance (time_t timeNow, std::set <std::string> &activeTorrents)
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
            // The torrent has disappeared, stop serving it.
            Log (PRIORITY_NORMAL, " Stop serving:  GTO disappeared from queue:  %s info hash:  %s",  mapIter->first.c_str(), mapIter->second->infoHash.c_str());

            (*listIter)->torrentSession->remove_torrent (mapIter->second->torrentHandle);
            activeTorrents.erase (mapIter->first);
            (*listIter)->mapOfSessionTorrents.erase (mapIter++);

            continue;
         }

         if (torrentModTime != mapIter->second->mtime)   // Has the GTO on disk changed
         {
            if (getInfoHash (mapIter->first) != mapIter->second->infoHash)
            {
               Log (PRIORITY_HIGH, "Stop serving:  GTO InfoHash Changed while serving:  %s info hash:  %s (new GTO infoHash %s will not be served)", mapIter->first.c_str(), mapIter->second->infoHash.c_str(), getInfoHash (mapIter->first).c_str()); 

               (*listIter)->torrentSession->remove_torrent (mapIter->second->torrentHandle);
               deleteGTOfromQueue (mapIter->first);
               activeTorrents.erase (mapIter->first);
               (*listIter)->mapOfSessionTorrents.erase (mapIter++);

               continue;
            }
           
            mapIter->second->mtime = torrentModTime;
            mapIter->second->expires = getExpirationTime (mapIter->first);

            if (timeNow <  mapIter->second->expires)
            {
               mapIter->second->overTimeAlertIssued = false;
            }

            Log (PRIORITY_NORMAL, "Expiration Update:  GTO %s info hash:  %s has a new expiration time %d", mapIter->first.c_str(), mapIter->second->infoHash.c_str(), mapIter->second->expires);
         }

         // if an upload torrent and the current state is seeding, set the overtime flag to true on the first obversation of this stats
         // on the 2nd observation of this state, the gto will removed from the upload queue and removed from seeding
         // This gives the upload plenty of time to recognize that the upload has completed (I.E., the upload client recognizes
         // two seeders are present due to tracker scraping
         if (mapIter->second->downloadGTO == false && mapIter->second->torrentHandle.status().state == libtorrent::torrent_status::seeding)
         {
std::cerr << "inside with time = " << time(NULL) << std::endl;
            if (!mapIter->second->overTimeAlertIssued)   // first pass set this true
            {
               mapIter->second->overTimeAlertIssued = true;
               if (_verbosityLevel > 0)
               {
                  screenOutput (std::setw (41) << getFileName (mapIter->first) << " Status: " << server_state_str[torrentStatus.state] << "  expires in approximately:  00:01:00.");
               }
               mapIter++;
            }
            else                                         // second pass, remove the torrent from serviing
            {
               Log (PRIORITY_NORMAL, "Stop serving:  upload complete %s info hash:  %s", mapIter->first.c_str(), getInfoHash (mapIter->first).c_str());

               (*listIter)->torrentSession->remove_torrent (mapIter->second->torrentHandle);
               deleteGTOfromQueue (mapIter->first);
               activeTorrents.erase (mapIter->first);
               (*listIter)->mapOfSessionTorrents.erase (mapIter++);
            }

            continue;
         }

         if (timeNow >= mapIter->second->expires)       // This GTO is expired
         {
            std::vector <libtorrent::peer_info> peers;

            mapIter->second->torrentHandle.get_peer_info (peers);

            if (peers.size () == 0)
            {
               Log (PRIORITY_NORMAL, "Stop serving:  Expiring %s info hash:  %s", mapIter->first.c_str(), getInfoHash (mapIter->first).c_str());

               (*listIter)->torrentSession->remove_torrent (mapIter->second->torrentHandle);
               deleteGTOfromQueue (mapIter->first);
               activeTorrents.erase (mapIter->first);
               (*listIter)->mapOfSessionTorrents.erase (mapIter++);
            }
            else
            {
               if (!mapIter->second->overTimeAlertIssued)
               {
                  Log (PRIORITY_NORMAL, "Overtime serving:  %s info hash:  %s (%d actor(s) connected)", mapIter->first.c_str(), getInfoHash (mapIter->first).c_str(), peers.size());
                  mapIter->second->overTimeAlertIssued = true;
               }
   
               if (_verbosityLevel > 0)
               {
                  screenOutput (std::setw (41) << getFileName (mapIter->first) << " Status: " << server_state_str[torrentStatus.state] << "  expired, but an actors continue to download");
               }
               mapIter++;
            }
         }
         else
         {
            if (_verbosityLevel > 0)
            {
                  screenOutput (std::setw (41) << getFileName (mapIter->first) << " Status: " << server_state_str[torrentStatus.state] << "  expires in approximately:  " <<  durationToStr(mapIter->second->expires - time (NULL)) << ".");
            }
            mapIter++;
         }
      }
      listIter++;
   }
}

bool gtServer::isDownloadModeGetFromGTO (std::string torrentPathAndFileName)
{
   bool dlMode = false;

   FILE *data = popen (("gtoinfo --get-key gt_download_mode " + torrentPathAndFileName).c_str(), "r");

   if (data != NULL)
   {
      char vBuff[1024];

      if (NULL != fgets (vBuff, 1024, data))
      {
         std::string result = vBuff;

         std::size_t foundPos;
 
         if (std::string::npos != (foundPos = result.find ('\n')))
         {
            result.erase (foundPos);
         }

         if (boost::iequals (PYTHON_TRUE, result))
         {
            dlMode = true;
         }
      }
      else
      {
         Log (PRIORITY_HIGH, "Failure running gtoinfo on %s or no 'gt_download_mode' in GTO, defaulting to upload mode", torrentPathAndFileName.c_str());
      }
      pclose (data);
   }
   else
   {
      Log (PRIORITY_HIGH, "Failure running gtoinfo on %s, defaulting to upload mode", torrentPathAndFileName.c_str());
   }

   return dlMode;       
}

bool gtServer::addTorrentToServingList (std::string pathAndFileName)
{
   activeSessionRec *workSession = findSession ();

   activeTorrentRec *newTorrRec = new (activeTorrentRec);

   newTorrRec->expires = getExpirationTime (pathAndFileName);
   newTorrRec->overTimeAlertIssued = false;

   time_t torrentModTime = 0;
   if (statFileOrDirectory (pathAndFileName, torrentModTime) < 0)
   {
      Log (PRIORITY_HIGH, "Failure adding %s to Served GTOs, GTO file removed.  Error:  %s (%d)", pathAndFileName.c_str(), strerror (errno), errno);
      delete newTorrRec;
      deleteGTOfromQueue (pathAndFileName);
      return false;
   }

   newTorrRec->mtime = torrentModTime;
   newTorrRec->infoHash = getInfoHash (pathAndFileName);

   std::string uuid = pathAndFileName;

   uuid = uuid.substr (0, uuid.rfind ('.'));
   uuid = getFileName (uuid); 

   if (isDownloadModeGetFromGTO (pathAndFileName))
   {
      newTorrRec->torrentParams.seed_mode = true;
      newTorrRec->torrentParams.disable_seed_hash = true;
      newTorrRec->downloadGTO = true;
   }
   else
   {
      newTorrRec->downloadGTO = false;
   }

   newTorrRec->torrentParams.auto_managed = false;
   newTorrRec->torrentParams.allow_rfc1918_connections = true;
   newTorrRec->torrentParams.save_path = "./";

   libtorrent::error_code torrentError;
   newTorrRec->torrentParams.ti = new libtorrent::torrent_info (pathAndFileName, torrentError);

   if (torrentError)
   {
      Log (PRIORITY_HIGH, "Failure adding %s to Served GTOs, GTO file removed.  Error: %s (%d)", pathAndFileName.c_str(), torrentError.message().c_str(), torrentError.value ());
      delete newTorrRec;
      deleteGTOfromQueue (pathAndFileName);
      return false;
   }

   newTorrRec->torrentHandle = workSession->torrentSession->add_torrent (newTorrRec->torrentParams, torrentError);

   if (torrentError)
   {
      Log (PRIORITY_HIGH, "Failure adding %s to Served GTOs, GTO file removed.  Error: %s (%d)", pathAndFileName.c_str(), torrentError.message().c_str(), torrentError.value ());
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
         Log (PRIORITY_HIGH, "Failure adding %s to Served GTOs, GTO file removed.  Error:  unable to obtain a signed SSL Certificate.", pathAndFileName.c_str());
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
      screenOutput ("adding " << getFileName (pathAndFileName) << " to files being served");
   }

   Log (PRIORITY_NORMAL, "Begin serving:  %s info hash:  %s expires:  %d (%s)", pathAndFileName.c_str(), newTorrRec->infoHash.c_str(), newTorrRec->expires, (newTorrRec->torrentParams.seed_mode == true ? "download" : "upload"));
   workSession->mapOfSessionTorrents[pathAndFileName] = newTorrRec;

   return true;
}

gtServer::activeSessionRec *gtServer::findSession ()
{
   checkSessions (); // start or adds sessions if unused session slots exist

   std::list <activeSessionRec *>::iterator listIter = _activeSessions.begin ();
   unsigned int torrentsBeingServed = (*listIter)->mapOfSessionTorrents.size ();
   gtServer::activeSessionRec *workingSessionRec = *listIter;

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

void gtServer::deleteGTOfromQueue (std::string fileName)
{
   int ret = unlink (fileName.c_str ()); 
   if (ret != 0)
   {
      gtError ("Unable to remove ", NO_EXIT, gtServer::ERRNO_ERROR, errno);
   }
}

libtorrent::session *gtServer::addActiveSession ()
{
   libtorrent::session *sessionNew = new libtorrent::session (*_gtFingerPrint, 0, libtorrent::alert::all_categories);
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

time_t gtServer::getExpirationTime (std::string torrentPathAndFileName)
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