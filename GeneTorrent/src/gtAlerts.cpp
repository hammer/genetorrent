/*                                           -*- mode: c++; tab-width: 2; -*-
 * $Id$
 *
 * Copyright (c) 2012, Annai Systems, Inc.
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
 *  Created on: Jan 23, 2012
 *      Author: donavan
 */

#define _DARWIN_C_SOURCE 1

#include <config.h>
/*
#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/peer_info.hpp"
#include "libtorrent/ip_filter.hpp"
*/

#include "libtorrent/alert_types.hpp"

#include "gtBase.h"

#include "gtLog.h"
#include "loggingmask.h"

void gtBase::checkAlerts (libtorrent::session &torrSession)
{
   std::deque <libtorrent::alert *> alerts;
   torrSession.pop_alerts (&alerts);   

   for (std::deque<libtorrent::alert *>::iterator dequeIter = alerts.begin(), end(alerts.end()); dequeIter != end; ++dequeIter)
   {
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

void gtBase::getGtoNameAndInfoHash (libtorrent::torrent_alert *alert, std::string &gtoName, std::string &infoHash)
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

void gtBase::processUnimplementedAlert (bool haveError, libtorrent::alert *alrt)
{
   if (_logMask & LOG_UNIMPLEMENTED_ALERTS)
   {
      Log (haveError, "%s : %s (%08x)", alrt->what(), alrt->message().c_str(), alrt->category() );
   }
}

void gtBase::processPeerNotification (bool haveError, libtorrent::alert *alrt)
{
   if (!(_logMask & LOG_PEER_NOTIFICATION))
   {
      return;
   }

   std::string gtoName;
   std::string infoHash;

   switch (alrt->type())
   {
      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}

void gtBase::processDebugNotification (bool haveError, libtorrent::alert *alrt)
{
   if (!(_logMask & LOG_DEBUG_NOTIFICATION))
   {
      return;
   }

   std::string gtoName;
   std::string infoHash;
   libtorrent::error_code ec;     // unused, but the conversion routine needs an argument

   switch (alrt->type())
   {
      case libtorrent::peer_connect_alert::alert_type:
      {
         libtorrent::peer_connect_alert *pca =  libtorrent::alert_cast<libtorrent::peer_connect_alert> (alrt);

         getGtoNameAndInfoHash (pca, gtoName, infoHash);

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
         libtorrent::peer_disconnected_alert *pda =  libtorrent::alert_cast<libtorrent::peer_disconnected_alert> (alrt);

         getGtoNameAndInfoHash (pda, gtoName, infoHash);

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

void gtBase::processStorageNotification (bool haveError, libtorrent::alert *alrt)
{
   if (!(_logMask & LOG_STORAGE_NOTIFICATION))
   {
      return;
   }

   std::string gtoName;
   std::string infoHash;

   switch (alrt->type())
   {
      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}

void gtBase::processStatNotification (bool haveError, libtorrent::alert *alrt)
{
   if (!(_logMask & LOG_STATS_NOTIFICATION))
   {
      return;
   }

   std::string gtoName;
   std::string infoHash;

   switch (alrt->type())
   {
      case libtorrent::stats_alert::alert_type:
      {
         libtorrent::stats_alert *statsAlert =  libtorrent::alert_cast<libtorrent::stats_alert> (alrt);

         getGtoNameAndInfoHash (statsAlert, gtoName, infoHash);

         Log (haveError, "%s, gto:  %s, infohash:  %s", statsAlert->message().c_str(), gtoName.c_str(), infoHash.c_str());
      } break;

      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}

void gtBase::processPerformanceWarning (bool haveError, libtorrent::alert *alrt)
{
   if (!(_logMask & LOG_PERFORMANCE_WARNING))
   {
      return;
   }

   std::string gtoName;
   std::string infoHash;

   switch (alrt->type())
   {
      case libtorrent::performance_alert::alert_type:
      {
         libtorrent::performance_alert *perfAlert =  libtorrent::alert_cast<libtorrent::performance_alert> (alrt);

         getGtoNameAndInfoHash (perfAlert, gtoName, infoHash);

         Log (haveError, "%s, gto:  %s, infohash:  %s", perfAlert->message().c_str(), gtoName.c_str(), infoHash.c_str());
      } break;
      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}

void gtBase::processIpBlockNotification (bool haveError, libtorrent::alert *alrt)
{
   if (!(_logMask & LOG_IP_BLOCK_NOTIFICATION))
   {
      return;
   }

   std::string gtoName;
   std::string infoHash;

   switch (alrt->type())
   {
      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}

void gtBase::processProgressNotification (bool haveError, libtorrent::alert *alrt)
{
   if (!(_logMask & LOG_PROGRESS_NOTIFICATION))
   {
      return;
   }

   std::string gtoName;
   std::string infoHash;

   switch (alrt->type())
   {
      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}

void gtBase::processTrackerNotification (bool haveError, libtorrent::alert *alrt)
{
// DJN mandatory TODO, every trackerNotification must be implemented to proper track the _successfulTrackerComms flag
// this entails filtering out error and non error notifications and behaving accordingly depending on the
// _logMask, haveError, and the tracker notification type

   if ((!(_logMask & LOG_TRACKER_NOTIFICATION)) && !haveError)
   {
      return;
   }

   std::string gtoName;
   std::string infoHash;

   switch (alrt->type())
   {
      case libtorrent::tracker_error_alert::alert_type:
      {
         libtorrent::tracker_error_alert *tea = libtorrent::alert_cast<libtorrent::tracker_error_alert> (alrt);

         getGtoNameAndInfoHash (tea, gtoName, infoHash);

         Log (haveError, "%s, gto:  %s, infohash:  %s", tea->message().c_str(), gtoName.c_str(), infoHash.c_str());

         if ((_operatingMode != SERVER_MODE ) && (tea->times_in_row > 2) && (_successfulTrackerComms == false))
         {
            gtError ("problem communicating with the transactor (3 times at startup) on URL:  " + tea->url, 214, gtBase::DEFAULT_ERROR, 0, tea->error.message());
         }
      } break;

      case libtorrent::tracker_warning_alert::alert_type:
      {
processUnimplementedAlert (haveError, alrt);
      } break;

      case libtorrent::scrape_failed_alert::alert_type:
      {
processUnimplementedAlert (haveError, alrt);
      } break;

      case libtorrent::scrape_reply_alert::alert_type:
      {
processUnimplementedAlert (haveError, alrt);
      } break;

      case libtorrent::tracker_reply_alert::alert_type:
      {
         _successfulTrackerComms = true;

         if ((!(_logMask & LOG_TRACKER_NOTIFICATION)) && !haveError)
         {
            return;
         }

         libtorrent::tracker_reply_alert *tra = libtorrent::alert_cast<libtorrent::tracker_reply_alert> (alrt);
         getGtoNameAndInfoHash (tra, gtoName, infoHash);
         Log (haveError, "%s, gto:  %s, infohash:  %s", tra->message().c_str(), gtoName.c_str(), infoHash.c_str());

      } break;

      case libtorrent::tracker_announce_alert::alert_type:
      {
         if ((!(_logMask & LOG_TRACKER_NOTIFICATION)) && !haveError)
         {
            return;
         }

         libtorrent::tracker_announce_alert *taa = libtorrent::alert_cast<libtorrent::tracker_announce_alert> (alrt);
         getGtoNameAndInfoHash (taa, gtoName, infoHash);
         Log (haveError, "%s, gto:  %s, infohash:  %s", taa->message().c_str(), gtoName.c_str(), infoHash.c_str());

      } break;

      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}

void gtBase::processStatusNotification (bool haveError, libtorrent::alert *alrt)
{
   if (!(_logMask & LOG_STATUS_NOTIFICATION))
   {
      return;
   }

   std::string gtoName;
   std::string infoHash;

   switch (alrt->type())
   {
      default:
      {
         processUnimplementedAlert (haveError, alrt);
      } break;
   }   
}
