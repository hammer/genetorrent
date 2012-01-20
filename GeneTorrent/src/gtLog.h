/*                                           -*- mode: c++; tab-width: 2; -*-
 * $Id: cp_log.h,v 1.10 2007/07/17 21:12:45 howdy Exp $
 *
 * Copyright (c) 2002-2007, Cardinal Peak, LLC.  All rights reserved. 
 * URL: www.cardinalpeak.com
 */

#ifndef CP_LOG_H
#define CP_LOG_H

#ifndef WIN32
#include <inttypes.h>
#endif

#include <string>

#ifndef __STRING
#define __STRING(x) # x
#endif

// Function Log: This function works exactly like printf, except (a)
// the line is prepended with the time, source file name, and source
// file number; and (b) the text goes to wherever the log destination
// is set to
#define Log(s1, ...) GlobalLog->__Log(__FILE__, __LINE__, (s1), ## __VA_ARGS__)

// Function Fatal: Just like Log, except in addition (a) the text is
// printed to stderr, and (b) the program is terminated with a call to
// abort()
// DJNX #define Fatal(s1, ...) __Fatal(__FILE__, __LINE__, __PRETTY_FUNCTION__, (s1), ## __VA_ARGS__)

// Function Assert: Just like the standard system assert call, except
// on failure the failed assertion and the text string is printed to
// stderr and the log file
// DJNX #define Assert(expr, s1, ...) __Assert((expr), __FILE__, __LINE__, __PRETTY_FUNCTION__, __STRING(expr), (s1), ## __VA_ARGS__)

// Places a backtrace of the specified depth into the log file.  Does
// not work on all platforms (but does work on Linux)
// DJNX #define Backtrace(depth) __Backtrace((depth), __FILE__, __LINE__)

// Places a timestamp into the log file, with a delta in milliseconds
// from the last time the Timestamp function was called
// DJNX #define Timestamp() __Timestamp(__FILE__, __LINE__, __PRETTY_FUNCTION__)

// Returns a string containing the result of sprintf'ing all the
// contents together; also logs that string to debug log
// DJNX #define ErrMsg(s1, ...) __ErrMsg(__FILE__, __LINE__, (s1), ## __VA_ARGS__)


//  TODO, This is out of date
// CPLog parses the program's command line as follows:
//
//   By default, no log file whatsoever is used, unless the boolean
//   use_log_file is set to true, in which case the log file is
//   created as ~/logs/(argv[0]).(timestamp).log
// 
//   The user can override this behavior by specifying an argument 
//       --log=(destination) 
//   on the command line.  In which case the following destinations
//   are allowed:
//      none       No log file will be created
//      stdout     Log will be sent to stdout
//      normal     Log will be created in the default location of 
//                 ~/logs/(argv[0]).(timestamp).log
//      filename   Log will be written to the specified file
//

class CPLog 
{
   public:
   
   static bool create_globallog (std::string, std::string, int childID = 0);
   static void delete_globallog();

   void __Log        (const char *file, int line, const char *string, ...);
// DJNX   void __Fatal      (const char *file, int line, const char *pretty_function, const char *string, ...) __attribute__ ((noreturn));
// DJNX   void __Assert     (bool expr, const char *file, int line, const char *pretty_function, const char *assertion, const char *string, ...);
// DJNX   void __Backtrace  (int depth, const char *file, int line);
// DJNX   void __Timestamp (const char *file, int line, const char *pretty_function);

// DJNX   std::string __ErrMsg (const char *file, int line, const char *string, ...);

   const char *log_file_name() { return m_filename; }
   bool logToStdErr() { return m_mode == CPLogOutputStderr; }

   private:
   enum OutputType 
   { 
      CPLogOutputNone,
      CPLogOutputSyslog,
      CPLogOutputStdout,
      CPLogOutputStderr,
      CPLogOutputFile, 
   } ;

   //CdPLog (int argc, char **argv, bool use_log_file);
   CPLog (std::string progName, std::string log, int childID);
   ~CPLog();

   static int s_global_refcnt;

   OutputType m_mode;
   char *m_filename;
   char *m_progname;
   FILE *m_fd;

   int64_t m_last_timestamp;
};

extern CPLog *GlobalLog;

#endif
