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
 * gtAlerts.cpp
 *
 *  Created on: Aug 15, 2011
 *      Author: donavan
 */

#define _DARWIN_C_SOURCE 1

#include <config.h>
/*
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
*/

#include "libtorrent/alert_types.hpp"

/*
#include <xqilla/xqilla-simple.hpp>

#include <curl/curl.h>
*/

#include "geneTorrent.h"

/*
#include "stringTokenizer.h"
#include "gtDefs.h"
#include "tclapOutput.h"
#include "geneTorrentUtils.h"
*/
#include "gtLog.h"
#include "loggingmask.h"

void geneTorrent::checkAlerts (libtorrent::session &torrSession)
{
   std::deque <libtorrent::alert *> alerts;
   torrSession.pop_alerts (&alerts);   

   for (std::deque<libtorrent::alert *>::iterator dequeIter = alerts.begin(), end(alerts.end()); dequeIter != end; ++dequeIter)
   {
      // Leaving this code in for now -- it existed prior to using alerts for logging, this needs to be fixed with a dns loookup at startup to verify the tracker is resolvable.
      if (((*dequeIter)->category() & libtorrent::alert::tracker_notification) && ((*dequeIter)->category() & libtorrent::alert::error_notification))
      {
         libtorrent::tracker_error_alert *tea = libtorrent::alert_cast<libtorrent::tracker_error_alert> (*dequeIter);

         if (tea->times_in_row > 2)
         {
            gtError ("Failure communicating with the transactor on URL:  " + tea->url, 214, geneTorrent::DEFAULT_ERROR, 0, tea->error.message());
         }
      }

      bool haveError = (*dequeIter)->category() & libtorrent::alert::error_notification;

      switch ((*dequeIter)->category() & ~libtorrent::alert::error_notification)
      {
         case libtorrent::alert::peer_notification:
         {
            processPeerNotification (haveError, *dequeIter);
         } break;

         case libtorrent::alert::storage_notification:
         {
            processStorageNotification (haveError, *dequeIter);
         } break;

         case libtorrent::alert::tracker_notification:
         {
            processTrackerNotification (haveError, *dequeIter);
         } break;

         case libtorrent::alert::debug_notification:
         {
            processDebugNotification (haveError, *dequeIter);
         } break;

         case libtorrent::alert::status_notification:
         {
            processStatusNotification (haveError, *dequeIter);
         } break;

         case libtorrent::alert::progress_notification:
         {
            processProgressNotification (haveError, *dequeIter);
         } break;

         case libtorrent::alert::ip_block_notification:
         {
            processIpBlockNotification (haveError, *dequeIter);
         } break;

         case libtorrent::alert::performance_warning:
         {
            processPerformanceWarning (haveError, *dequeIter);
         } break;

         case libtorrent::alert::stats_notification:
         {
            processStatNotification (haveError, *dequeIter);
         } break;

         case libtorrent::alert::port_mapping_notification:
         case libtorrent::alert::dht_notification:
         case libtorrent::alert::rss_notification:
         default:
         {
            Log (haveError, "Unknown alert category %08x encountered in checkAlerts() [%s : %s]", (*dequeIter)->category(), (*dequeIter)->what(), (*dequeIter)->message().c_str());
         } break;
      }
   }
   alerts.clear();
}

void geneTorrent::setGtoNameAndInfoHash (libtorrent::torrent_alert *alert, std::string &gtoName, std::string &infoHash)
{
   if (alert->handle.is_valid())
   {
      gtoName = alert->handle.name();
      char msg[41];
      libtorrent::to_hex((char const*)&(alert->handle).info_hash()[0], 20, msg);
      infoHash = msg;
   }
   else
   {
      gtoName = "unknown";
      infoHash = "unknown";
   }
}

void geneTorrent::processUnimplementedAlert (bool haveError, libtorrent::alert *alrt)
{
   if (_logMask & LOG_UNIMPLEMENTED_ALERTS)
   {
      Log (haveError, "%s : %s (%08x)", alrt->what(), alrt->message().c_str(), alrt->category() );
   }
}

void geneTorrent::processPeerNotification (bool haveError, libtorrent::alert *alrt)
{
   switch (alrt->type())
   {
      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}

void geneTorrent::processDebugNotification (bool haveError, libtorrent::alert *alrt)
{
   switch (alrt->type())
   {
      case libtorrent::peer_connect_alert::alert_type:
      {
         if (!(_logMask & LOG_DEBUG_NOTIFICATION))
         {
            break;
         }

         libtorrent::peer_connect_alert *pca =  libtorrent::alert_cast<libtorrent::peer_connect_alert> (alrt);

         //std::string peerIPport; 
         //std::string peerID;

         std::string gtoName;
         std::string infoHash;
         setGtoNameAndInfoHash (pca, gtoName, infoHash);

         libtorrent::error_code ec;     // unused, but the conversion routine needs an argument

         if (pca->pid[0] != '\0')       // pid is a typedef from a big_number class as a string of 20 nulls if not assigned a proper pid, thus size() functions fail to report an empty string.
         {
            Log (haveError, "connecting to %s:%d (ID %s), gto: %s, infohash: %s", pca->ip.address().to_string(ec).c_str(), pca->ip.port(), pca->pid.to_string().c_str(), gtoName.c_str(), infoHash.c_str());
         }
         else
         {
            Log (haveError, "connecting to %s:%d gto: %s, infohash: %s", pca->ip.address().to_string(ec).c_str(), pca->ip.port(), gtoName.c_str(), infoHash.c_str());
         }
      } break;

      case libtorrent::peer_disconnected_alert::alert_type:
      {
         if (!(_logMask & LOG_DEBUG_NOTIFICATION))
         {
            break;
         }

         libtorrent::peer_disconnected_alert *pda =  libtorrent::alert_cast<libtorrent::peer_disconnected_alert> (alrt);

         std::string gtoName;
         std::string infoHash;
         //std::string peerIPport; 
         //std::string peerID;

         if (pda->handle.is_valid())
         {
            gtoName = pda->handle.name();
            char msg[41];
            libtorrent::to_hex((char const*)&(pda->handle).info_hash()[0], 20, msg);
            infoHash = msg;
         }
         else
         {
            gtoName = "unknown";
            infoHash = "unknown";
         }

         libtorrent::error_code ec;      // unused, but the conversion routine needs an argument

         if (pda->pid[0] != '\0')        // pid is a typedef from a big_number class as a string of 20 nulls if not assigned a proper pid, thus size() functions fail to report an empty string.
         {
            Log (haveError, "disconnecting from %s:%d (ID %s), reason: %s, gto: %s, infohash: %s", pda->ip.address().to_string(ec).c_str(), pda->ip.port(), pda->pid.to_string().c_str(), pda->error.message().c_str(), gtoName.c_str(), infoHash.c_str());
         }
         else
         {
            Log (haveError, "disconnecting from %s:%d reason: %s, gto: %s, infohash: %s", pda->ip.address().to_string(ec).c_str(), pda->ip.port(), pda->error.message().c_str(), gtoName.c_str(), infoHash.c_str());
         }
      } break;

      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}

void geneTorrent::processStorageNotification (bool haveError, libtorrent::alert *alrt)
{
   switch (alrt->type())
   {
      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}

void geneTorrent::processStatNotification (bool haveError, libtorrent::alert *alrt)
{
   switch (alrt->type())
   {
      case libtorrent::stats_alert::alert_type:
      {
         if (!(_logMask & LOG_STATS_NOTIFICATION))
         {
            break;
         }

         libtorrent::stats_alert *statsAlert =  libtorrent::alert_cast<libtorrent::stats_alert> (alrt);

         std::string gtoName;
         std::string infoHash;

         setGtoNameAndInfoHash (statsAlert, gtoName, infoHash);

         Log (haveError, "%s, gto:  %s, infohash:  %s", statsAlert->message().c_str(), gtoName, infoHash );
         //Log (haveError, "connecting to %s:%d gto: %s, infohash: %s", pca->ip.address().to_string(ec).c_str(), pca->ip.port(), gtoName.c_str(), infoHash.c_str());

      } break;

      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}

void geneTorrent::processPerformanceWarning (bool haveError, libtorrent::alert *alrt)
{
   switch (alrt->type())
   {
      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}

void geneTorrent::processIpBlockNotification (bool haveError, libtorrent::alert *alrt)
{
   switch (alrt->type())
   {
      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}

void geneTorrent::processProgressNotification (bool haveError, libtorrent::alert *alrt)
{
   switch (alrt->type())
   {
      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}

void geneTorrent::processTrackerNotification (bool haveError, libtorrent::alert *alrt)
{
   switch (alrt->type())
   {
      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}

void geneTorrent::processStatusNotification (bool haveError, libtorrent::alert *alrt)
{
   switch (alrt->type())
   {
      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}
