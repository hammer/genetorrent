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
 * gtLog.cpp
 *
 *  Created on: January 19, 2012
 *      Author: donavan
 */

#include "config.h"

#include <errno.h> 
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>
#include <sstream>

#include "gtLog.h" 

gtLogger *GlobalLog = NULL;
int gtLogger::s_global_refcnt = 0;

inline const char *filebase(const char *file)
{
   const char * filebase;
   if ((filebase = strrchr(file, '/')) == NULL)
      filebase = file;
   else
      filebase++;

   return filebase;
}

bool gtLogger::create_globallog(std::string progName, std::string log, int childID) 
{
   if (GlobalLog == NULL)
   {
      GlobalLog = new gtLogger(progName, log, childID);
   }

   s_global_refcnt++;

   return GlobalLog->logToStdErr();
}

void gtLogger::delete_globallog()
{
   if (--s_global_refcnt == 0) 
   {
      delete GlobalLog;
      GlobalLog = NULL;
   }
}

// priority determines if messages are sent to stderr if logging to a file, syslog, or none
gtLogger::gtLogger (std::string progName, std::string log, int childID) : m_mode (gtLoggerOutputNone), m_fd (NULL), m_last_timestamp (0)
{
   m_progname = strdup (progName.c_str());
   m_filename = strdup (log.c_str());

   // m_filename is one of "none", "syslog", "stdout", "stderr", or a filename
   if (!strcmp(m_filename, "none")) 
   {
      m_mode = gtLoggerOutputNone;
   } 
   else if (!strcmp(m_filename, "stdout")) 
   {
      m_fd = stdout;
      m_mode = gtLoggerOutputStdout;
   } 
   else if (!strcmp(m_filename, "stderr")) 
   {
      m_fd = stderr;
      m_mode = gtLoggerOutputStderr;
   }
   else if (!strcmp(m_filename, "syslog")) 
   {
      m_mode = gtLoggerOutputSyslog;
      openlog (m_progname, LOG_PID, LOG_LOCAL0);
   }
   else 
   {
      // GeneTorrent Download child processes have their own log, childID is inserted after the last dot (.) in the specified filename
      // or the childID is appended to the file name if no dot (.) is present in the filename.
      // The parent download process sends output to filename
      if (childID > 0)   
      {
         std::ostringstream outbuff;
         std::string work=m_filename;
         free (m_filename);

         size_t pos = work.rfind ('.');
  
         if (std::string::npos != pos)
         {
            outbuff << work.substr(0,pos) << '.' << childID << work.substr(pos);
         }
         else
         {
            outbuff << work << '.' << childID;
         }

         m_filename = strndup (outbuff.str().c_str(), outbuff.str().size());
      }

      m_fd = fopen(m_filename, "w");
      if (m_fd == NULL) 
      {
         m_mode = gtLoggerOutputNone;
         fprintf(stderr, "Error opening log file %s\n", m_filename);
         fprintf(stderr, "Error %s\n", strerror(errno));
         exit(1);
      }

      m_mode = gtLoggerOutputFile;
   }

   time_t clocktime;
   time(&clocktime);

   if (m_mode == gtLoggerOutputFile) 
   {
      // Write a log header
      fprintf(m_fd, "Log file initiated at %s", ctime(&clocktime));
      fprintf(m_fd, "Process id: %d\n", getpid());
      fputs("=============================================================\n", m_fd);
      fflush(m_fd);
   }
}

gtLogger::~gtLogger()
{
   time_t clocktime;

   // Write a log footer
   if (m_mode == gtLoggerOutputFile) 
   {
      fputs("=============================================================\n", m_fd);
      time(&clocktime);
      fprintf(m_fd, "Log file closed normally at %s", ctime(&clocktime));
      fclose(m_fd);
      m_fd = NULL;
   }

   free(m_filename);
   free(m_progname);

   if (GlobalLog == this)
      GlobalLog = NULL;
}

void gtLogger::__Log (gtLogLevel priority, const char *file, int line, const char *fmt, ...)
{
   if (PRIORITY_HIGH == priority && ( m_mode == gtLoggerOutputNone || m_mode == gtLoggerOutputSyslog || m_mode == gtLoggerOutputFile))
   {
      std::string format = "Error:  " + std::string(fmt) + "\n";
      va_list ap;
      va_start (ap, fmt);
      vfprintf(stderr, format.c_str(), ap);
      va_end(ap);
   }

   if (m_mode == gtLoggerOutputNone) 
      return;

   char buffer[1024];

   if (m_mode != gtLoggerOutputSyslog) 
   {
      char timebuf[1024];
      struct timeval now;
      struct tm time_tm;

      gettimeofday(&now,  NULL);
      time_t nowSec = now.tv_sec;
      localtime_r(&nowSec, &time_tm);
      strftime(timebuf, sizeof(timebuf), "%m/%d %H:%M:%S", &time_tm);

      switch (priority)
      {
         case PRIORITY_HIGH:
         {
            snprintf(buffer, sizeof(buffer), "%s.%03d Error:  %s\n", timebuf, static_cast<int>(now.tv_usec/1000), fmt);
         } break;

         case PRIORITY_NORMAL:      // fall-through

         case PRIORITY_DEBUG:
         {
            snprintf(buffer, sizeof(buffer), "%s.%03d %s\n", timebuf, static_cast<int>(now.tv_usec/1000), fmt);
         } break;
      }
   }
   else
   {
      switch (priority)
      {
         case PRIORITY_HIGH:
         {
            snprintf(buffer, sizeof(buffer), "Error:  %s", fmt);
         } break;

         case PRIORITY_NORMAL:      // fall-through

         case PRIORITY_DEBUG:
         {
            snprintf(buffer, sizeof(buffer), "%s", fmt);
         } break;
      }
   }

   va_list ap;
   va_start (ap, fmt);

   if (m_mode != gtLoggerOutputSyslog && m_fd != NULL)
   {
      vfprintf(m_fd, buffer, ap);
   }
   else
   {
      char sysLogBuffer[2048];
      vsnprintf(sysLogBuffer, sizeof(sysLogBuffer), buffer, ap);
      switch (priority)
      {
         case PRIORITY_HIGH:
         {
            syslog (LOG_ALERT, "%s", sysLogBuffer);
         } break;

         case PRIORITY_NORMAL:
         {
            syslog (LOG_INFO, "%s", sysLogBuffer);
         } break;

         case PRIORITY_DEBUG:
         {
            syslog (LOG_DEBUG, "%s", sysLogBuffer);
         } break;
      }
   }

   va_end(ap);

   if (m_mode != gtLoggerOutputSyslog && m_fd != NULL)
   {
      fflush(m_fd);
   }
}
