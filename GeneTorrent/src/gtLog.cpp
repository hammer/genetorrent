/*
 * A simple way to log debugging output
 *
 * Author: Howdy Pierce, howdy@cardinalpeak.com
 *
 * Copyright (c) 2002, 2004-2005, Cardinal Peak, LLC
 *   http://www.cardinalpeak.com
 */

#include "config.h"

#include <errno.h> 
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <iostream>

#include "gtLog.h" 

CPLog *GlobalLog = NULL;
int CPLog::s_global_refcnt = 0;

inline const char *filebase(const char *file)
{
   const char * filebase;
   if ((filebase = strrchr(file, '/')) == NULL)
      filebase = file;
   else
      filebase++;

   return filebase;
}

bool CPLog::create_globallog(std::string progName, std::string log) 
{
   if (GlobalLog == NULL)
      GlobalLog = new CPLog(progName, log);

   s_global_refcnt++;

   return GlobalLog->logToStdErr();
}

void CPLog::delete_globallog()
{
   if (--s_global_refcnt == 0) 
   {
      delete GlobalLog;
      GlobalLog = NULL;
   }
}

CPLog::CPLog (std::string progName, std::string log) : m_mode (CPLogOutputNone), m_fd (0), m_last_timestamp (0)
{
   time_t clocktime;
   
   m_filename = strdup (log.c_str());

   // m_filename is one of "none", "syslog", "stdout", "stderr", or filename
   if (!strcmp(m_filename, "none")) 
   {
      m_mode = CPLogOutputNone;
   } 
   else if (!strcmp(m_filename, "stdout")) 
   {
      m_fd = stdout;
      m_mode = CPLogOutputStdout;
   } 
   else if (!strcmp(m_filename, "stderr")) 
   {
      m_fd = stderr;
      m_mode = CPLogOutputStderr;
   }
   else if (!strcmp(m_filename, "syslog")) 
   {
      m_mode = CPLogOutputSyslog;
   }
   else 
   {
      m_fd = fopen(m_filename, "w");
      if (m_fd == NULL) 
      {
         m_mode = CPLogOutputNone;
         fprintf(stderr, "Error opening log file %s\n", m_filename);
         fprintf(stderr, "Error %s\n", strerror(errno));
         exit(1);
      }

      m_mode = CPLogOutputFile;
   }

   time(&clocktime);

   if (m_mode == CPLogOutputFile) 
   {
      //assert(m_fd == NULL);
      // Write a log header
      fprintf(m_fd, "Log file initiated at %s", ctime(&clocktime));
      fprintf(m_fd, "Process id: %d\n", getpid());
      fputs("=============================================================\n", m_fd);
      fflush(m_fd);
   }
}

CPLog::~CPLog()
{
   time_t clocktime;

   // Write a log footer
   if (m_mode == CPLogOutputFile) 
   {
      fputs("=============================================================\n", m_fd);
      time(&clocktime);
      fprintf(m_fd, "Log file closed normally at %s", ctime(&clocktime));
      fclose(m_fd);
   }

   free(m_filename);
   free(m_progname);

   if (GlobalLog == this)
      GlobalLog = NULL;
}

void CPLog::__Log (const char *file, int line, const char *fmt, ...)
{
   if (m_mode == CPLogOutputNone) 
      return;

   va_list ap;
   char buffer[1024];

   if (m_mode != CPLogOutputSyslog) 
   {
      char timebuf[1024];
      struct timeval now;
      struct tm time_tm;

      gettimeofday(&now,  NULL);
      time_t nowSec = now.tv_sec;
      localtime_r(&nowSec, &time_tm);
      strftime(timebuf, sizeof(timebuf), "%m/%d %H:%M:%S", &time_tm);

      snprintf(buffer, sizeof(buffer), "%s.%03d %s\n", timebuf, static_cast<int>(now.tv_usec/1000), fmt);
   }
   else
   {
      snprintf(buffer, sizeof(buffer), "%s", fmt);
   }

   va_start (ap, fmt);

   if (m_mode != CPLogOutputSyslog && m_fd > 0)
   {
      vfprintf(m_fd, buffer, ap);
   }
   else
   {
      char sysLogBuffer[2048];
      vsnprintf(sysLogBuffer, sizeof(sysLogBuffer), buffer, ap);
      syslog (LOG_INFO, "%s", sysLogBuffer);
   }

   va_end(ap);

   if (m_mode != CPLogOutputSyslog && m_fd > 0)
   {
      fflush(m_fd);
   }
}

/*
void CPLog::__Fatal (const char *file, int line, const char *pretty_function, const char *fmt, ...)
{
   va_list ap;
   char buffer[1024];
   char errstring[2048];
   time_t clocktime;
   OutputType mode = m_mode;

   m_mode = CPLogOutputNone;

   va_start (ap, fmt);
   snprintf(buffer, sizeof(buffer), "%08x %s:%d: %s", (unsigned)pthread_self(), filebase(file), line, fmt);
   vsnprintf(errstring, sizeof(errstring), buffer, ap);
   va_end(ap);

   // Log to m_fd if that makes sense
   if (mode == CPLogOutputStdout || mode == CPLogOutputFile) 
   {
      fprintf(m_fd, "==================== FATAL ERROR ====================\n");
      fprintf(m_fd, errstring);
      fprintf(m_fd, "in function %s\n", pretty_function);
      time(&clocktime);
      fprintf(m_fd, "Log file closed abnormally at %s", ctime(&clocktime));
      fflush(m_fd);

      if (mode == CPLogOutputFile) 
      {
         fclose(m_fd);
         m_fd = NULL;
      }
   }

   // Log to stderr if that makes sense
   if (mode == CPLogOutputNone || mode == CPLogOutputFile) 
   {
      fprintf(stderr, "%s: fatal error: %s\n", m_progname, errstring);
   }

   abort();
}
*/

/*
void CPLog::__Assert (bool expr, const char *file, int line, const char *pretty_function, const char *assertion, const char *fmt, ...)
{
   va_list ap;
   char buffer[1024];
   char errstring[2048];
   time_t clocktime;
   OutputType mode = m_mode;

   if (expr)
      return;

   m_mode = CPLogOutputNone;

   va_start (ap, fmt);
   snprintf(buffer, sizeof(buffer), "%08x %s:%d: %s", (unsigned)pthread_self(), filebase(file), line, fmt);
   vsnprintf(errstring, sizeof(errstring), buffer, ap);
   va_end(ap);

   // Log to m_fd if that makes sense
   if (mode == CPLogOutputStdout || mode == CPLogOutputFile) 
   {
      fprintf(m_fd, "================== FAILED ASSERTION ==================\n");

      fprintf(m_fd, "%08x %s:%d: %s\n", (unsigned)pthread_self(), file, line, assertion);
      fprintf(m_fd, errstring);
      fprintf(m_fd, "in function %s\n", pretty_function);

      __Backtrace(15, file, line);

      time(&clocktime);
      fprintf(m_fd, "Log file closed abnormally at %s", ctime(&clocktime));

      fflush(m_fd);

      if (mode == CPLogOutputFile) 
      {
         fclose(m_fd);
         m_fd = NULL;
      }
   }

   // Log to stderr if that makes sense
   if (mode == CPLogOutputNone || mode == CPLogOutputFile) 
   {
      fprintf(stderr, "%s: failed assertion: %s\n", m_progname, assertion);
      fprintf(stderr, "%s\n", errstring);
   }

   abort();
}
*/

/*
void CPLog::__Backtrace(int depth, const char * file, int line)
{
#ifdef HAVE_BACKTRACE

#define BACKTRACE_DEPTH 20
   if (m_mode == CPLogOutputNone) 
      return;

   depth++;  // We remove ourselves from the backtrace output

   void *trace[BACKTRACE_DEPTH];
   char **messages = (char **) NULL;
   int i, trace_size = 0;

   if (depth > BACKTRACE_DEPTH) 
      depth = BACKTRACE_DEPTH;

   trace_size = backtrace(trace, depth);
   messages = backtrace_symbols(trace, trace_size);

   fprintf(m_fd, "%s:%d: ===== backtrace begin =====\n", filebase(file), line);

   // start at 1: remove this fn from the output
   for (i=1; i<trace_size; i++) 
      fprintf(m_fd, " [bt] %s\n", messages[i]);

   free(messages);
#else
   fprintf(m_fd, "%08x %s:%d: (backtrace not is supported on this platform)\n", (unsigned)pthread_self(), filebase(file), line);
#endif  // HAVE_BACKTRACE
}
*/

/*
void CPLog::__Timestamp  (const char *file, int line, const char *pretty_function)
{
   struct timeval tv;
   int64_t this_timestamp;

   if (m_mode == CPLogOutputNone) 
      return;

   gettimeofday(&tv, NULL);

   this_timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000;

   fprintf(m_fd, "%08x %s:%d: in %s: Timestamp %lld (delta %lld)\n", (unsigned)pthread_self(), filebase(file), line, pretty_function, this_timestamp, this_timestamp - m_last_timestamp);

   m_last_timestamp = this_timestamp;
}
*/

/*
std::string CPLog::__ErrMsg (const char *file, int line, const char *fmt, ...)
{
   va_list ap;
   char buffer[4098];

   va_start (ap, fmt);
   vsnprintf(buffer, sizeof(buffer), fmt, ap);

   __Log(file, line, "ErrMsg: %s\n", buffer);

   return buffer;
}
*/
