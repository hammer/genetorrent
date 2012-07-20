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

//============================================================================
// Name        : gtMain.cpp
//============================================================================

#include <config.h>

#include <iostream>
#include <fstream>

#include <curl/curl.h>

#include <boost/program_options.hpp>

#include "accumulator.hpp"
#include "gtDefs.h"
#include "gtUpload.h"
#include "gtServer.h"
#include "gtDownload.h"

int global_verbosity = 0;    // Work around for boost:program_options not supporting -vvvvv type arguments
bool global_gtAgentMode;
std::string global_startup_message = "";

std::string makeOpt (std::string baseName, const char secondName = SPACE)
{
   if (secondName != SPACE)
   {
      return baseName + "," + secondName;
   }
   return baseName + "," +baseName.substr(0,1);
}

void configureConfigFileOptions (boost::program_options::options_description &options)
{
   // The descriptions below are not used in the help message.
   options.add_options() 
      (makeOpt (BIND_IP_CLI_OPT).c_str(), boost::program_options::value< std::string >(), "Bind IP")        // long and short option using first letter of long option
      (makeOpt (CRED_FILE_CLI_OPT).c_str(), boost::program_options::value< std::string >(), "path/file to credentials file")          
      (makeOpt (CONF_DIR_CLI_OPT, CONF_DIR_SHORT_CLI_OPT).c_str(), boost::program_options::value< std::string >(), "full path to SSL configuration files")    // long option with alternate short option
      (makeOpt (ADVERT_IP_CLI_OPT, ADVERT_IP_SHORT_CLI_OPT).c_str(), boost::program_options::value< std::string >(), "IP Address advertised")    
      (makeOpt (ADVERT_PORT_CLI_OPT, ADVERT_PORT_SHORT_CLI_OPT).c_str(), boost::program_options::value< int >(), "TCP Port advertised")      
      (makeOpt (INTERNAL_PORT_CLI_OPT).c_str(), boost::program_options::value< std::string >(), "local IP port to bind on")     
      (makeOpt (LOGGING_CLI_OPT).c_str(), boost::program_options::value< std::string >(), "path/file to log file, follow by the log level")  
      (makeOpt (PATH_CLI_OPT).c_str(), boost::program_options::value< std::string >(), "file system path used for uploads and downloads")
      (makeOpt (RATE_LIMIT_CLI_OPT).c_str(), boost::program_options::value< float >(), "transfer rate limiter in MB/s (megabytes/second)")
      (makeOpt (GTA_CLIENT_CLI_OPT).c_str(), "Operating as a child of GTA, this option is hidden")
      (makeOpt (TIMESTAMP_STD_CLI_OPT).c_str(), "add timestamps to messages logged to the screen")
      (VERBOSITY_CLI_OPT.c_str(), boost::program_options::value< int >(), "on screen verbosity level")

      // Download
      (makeOpt (DOWNLOAD_CLI_OPT).c_str(), boost::program_options::value< std::vector <std::string> >()->composing(), "URI | UUID | .xml | .gto")
      (MAX_CHILDREN_CLI_OPT.c_str(), boost::program_options::value< int >(), "number of download children")

      // Upload
      (makeOpt (UPLOAD_FILE_CLI_OPT).c_str(), boost::program_options::value< std::string >(), "uuid/manifest.xml")
      (UPLOAD_GTO_PATH_CLI_OPT.c_str(), boost::program_options::value< std::string >(), "writable path for .GTO file during creation and transmission")

      // Server Mode
      (makeOpt (SERVER_CLI_OPT).c_str(), boost::program_options::value< std::string >(),"server data path")
      (makeOpt (QUEUE_CLI_OPT).c_str(), boost::program_options::value< std::string >(), "input GTO directory")    
      (SECURITY_API_CLI_OPT.c_str(), boost::program_options::value< std::string >(), "SSL Key Signing URL")    

      // Legacy long option names
      (BIND_IP_CLI_OPT_LEGACY.c_str(), boost::program_options::value< std::string >(), "Bind IP")        
      (CRED_FILE_CLI_OPT_LEGACY.c_str(), boost::program_options::value< std::string >(), "path/file to credentials file")          
      (CONF_DIR_CLI_OPT_LEGACY.c_str(), boost::program_options::value< std::string >(), "full path to SSL configuration files")          
      (ADVERT_IP_CLI_OPT_LEGACY.c_str(), boost::program_options::value< std::string >(), "IP Address advertised")         
      (ADVERT_PORT_CLI_OPT_LEGACY.c_str(), boost::program_options::value< int >(), "TCP Port advertised")          
      (INTERNAL_PORT_CLI_OPT_LEGACY.c_str(), boost::program_options::value< int >(), "local IP port to bind on")          
      (UPLOAD_FILE_CLI_OPT_LEGACY.c_str(), boost::program_options::value< std::string >(), "manifest.xml")
      (MAX_CHILDREN_CLI_OPT_LEGACY.c_str(), boost::program_options::value< int >(), "number of download children")
   ;
}

void commandLineError (std::string errMessage)
{
   if (errMessage.size())
   {
      if (global_gtAgentMode)
      {
         std::cout << "error:  " << errMessage << std::endl;
         std::cout.flush();
      }
      else
      {
         std::cerr << "error:  " << errMessage << std::endl;
      }
   }
   exit (COMMAND_LINE_OR_CONFIG_FILE_ERROR);
}

void configureCommandLineOptions (boost::program_options::options_description &options)
{
   // The descriptions below are not used in the help message.
   options.add_options() 
      (CONFIG_FILE_CLI_OPT.c_str(), boost::program_options::value< std::string >(), "path/file to optional config file")
      (makeOpt (HELP_CLI_OPT).c_str(), "Help Message")          
      (makeOpt (NO_LONG_CLI_OPT, VERBOSITY_SHORT_CLI_OPT).c_str(), accumulator<int>(&global_verbosity), "on screen verbosity level")
      (VERSION_CLI_OPT.c_str(),       "Display Version")       // long option only
   ;
}

void processCommandLine (boost::program_options::variables_map &clOptions, int argc, char **argv)
{
   bool haveVerboseOnCli = false;
   try
   {
      boost::program_options::options_description configFileOpts ("Config and Command Line Options");
      configureConfigFileOptions (configFileOpts);

      boost::program_options::options_description commandLineOpts ("Command Line Only Options");
      configureCommandLineOptions (commandLineOpts);

      boost::program_options::options_description allOpts ("All Options");
      allOpts.add(configFileOpts).add(commandLineOpts);

      boost::program_options::variables_map cli;
      boost::program_options::store (boost::program_options::parse_command_line (argc, argv, allOpts), cli);

      // Check if help was requested
      if (cli.count (HELP_CLI_OPT))
      {
         std::cout << "Usage:" << std::endl;
         std::cout << "   GeneTorrent -u manifest-file -c cred [ -p path ]" << std::endl;
         std::cout << "   GeneTorrent -d < URI | UUID | .xml | .gto > -c cred [ -p path ]" << std::endl;
         std::cout << "   GeneTorrent -s path -q work-queue -c cred --security-api signing-URI" << std::endl;
         std::cout << std::endl;
         std::cout << "Additional options are available.  Type 'man GeneTorrent' for more information." << std::endl;
sleep (20);
         exit (0);
      }

      if (cli.count (VERSION_CLI_OPT))
      {
         std::cout << "GeneTorrent release " << VERSION << std::endl;
         exit (0);
      }

      if (cli.count (VERBOSITY_CLI_OPT))
      {
         haveVerboseOnCli = true;
      }

      // Check if a config file was specified
      if (cli.count (CONFIG_FILE_CLI_OPT) == 1)
      {
         std::string configPathAndFile = cli[CONFIG_FILE_CLI_OPT].as<std::string>();

         if (statFile (configPathAndFile) != 0)
         {
            commandLineError ("unable to open config file '" + configPathAndFile + "'.");
         }
         
         std::ifstream inputFile(configPathAndFile.c_str());

         if (!inputFile)
         {
            commandLineError ("unable to open config file '" + configPathAndFile + "'.");
         }

         boost::program_options::store (boost::program_options::parse_config_file (inputFile, configFileOpts), cli);

         global_startup_message = " --" + CONFIG_FILE_CLI_OPT + "=" + cli[CONFIG_FILE_CLI_OPT].as<std::string>();
      }

      if (cli.count (GTA_CLIENT_CLI_OPT) == 1)
      { 
         global_gtAgentMode = true;
      }

      if (cli.count (SERVER_CLI_OPT) == 0 && cli.count (DOWNLOAD_CLI_OPT) == 0 && cli.count (UPLOAD_FILE_CLI_OPT) == 0 && cli.count (UPLOAD_FILE_CLI_OPT_LEGACY) == 0)
      {
         commandLineError ("Command line or config file must include one of -d (download), -s (server), or -u (upload).");
      }

      if ((cli.count (SERVER_CLI_OPT) > 0 && cli.count (DOWNLOAD_CLI_OPT) > 0 ) || 
          ((cli.count (UPLOAD_FILE_CLI_OPT_LEGACY) > 0 || cli.count (UPLOAD_FILE_CLI_OPT)) > 0 && cli.count (SERVER_CLI_OPT) > 0 ) ||
          (cli.count (DOWNLOAD_CLI_OPT) > 0 && (cli.count (UPLOAD_FILE_CLI_OPT_LEGACY) > 0 || cli.count (UPLOAD_FILE_CLI_OPT) > 0))) 
      { 
         commandLineError ("Command line or config file may only specifiy one of -d (download), -s (server), or -u (upload).");
      } 

      boost::program_options::notify (cli); 

      // Verify and configure global_verbosity level here
      std::ostringstream shortVerboseFlag;
      shortVerboseFlag << VERBOSITY_SHORT_CLI_OPT;
  
      if (global_verbosity > 0 && haveVerboseOnCli)
      {
         commandLineError ("-" + shortVerboseFlag.str() + " and --" + VERBOSITY_CLI_OPT + " are not permitted on the command line at the same time");
      }

      if (global_verbosity > 0)  // -v is present, -v overrides any possible --verbose
      {
         global_startup_message += " -" + shortVerboseFlag.str(); 
         if (global_verbosity >= 2)  // override out of range value from legacy -vvvv or -vvv
         {
            global_startup_message += shortVerboseFlag.str();
            global_verbosity = 2;
         }
      }
      else  // check if --verbosity available and set level if present
      {
         if (cli.count (VERBOSITY_CLI_OPT))
         {
            global_verbosity =cli[VERBOSITY_CLI_OPT].as< int >();

            if (global_verbosity < 1 ||global_verbosity > 2)
            {
               std::ostringstream errorMes;
               errorMes << "--" << VERBOSITY_CLI_OPT << "=" << global_verbosity << " is not valid, try 1 or 2";
               commandLineError (errorMes.str());
            }

            if (global_verbosity == 1)
            {
               global_startup_message += " --" + VERBOSITY_CLI_OPT + "=1";
            }
            else
            {
               global_startup_message += " --" + VERBOSITY_CLI_OPT + "=2";
            }
         }
      }

      if (cli.count (GTA_CLIENT_CLI_OPT) > 0)
      {
         if (global_verbosity > 0)
         {
            commandLineError ("--" + GTA_CLIENT_CLI_OPT + " may not be combined with either -" + shortVerboseFlag.str() + " or --" + VERBOSITY_CLI_OPT + ".");
         }

         if (cli.count (DOWNLOAD_CLI_OPT) == 0)
         {
            commandLineError ("--" + GTA_CLIENT_CLI_OPT + " can only be used with the --" + DOWNLOAD_CLI_OPT + " command line option.");
         }
      }


      clOptions = cli; 
   } 
   catch(std::exception &e) 
   { 
      commandLineError (e.what());
   }
   catch(...) 
   { 
      commandLineError ("unknown problem parsing command line or config file");
   }
}

int main (int argc, char **argv)
{
#ifdef TORRENT_DEBUG                                                                                                                                         
   std::cerr << "********************************************************\n" << 
                "*** D E B U G  B U I L D  O F  G E N E T O R R E N T ***\n" << 
                "********************************************************" << std::endl;
#endif

   curl_global_init(CURL_GLOBAL_ALL);

   boost::program_options::variables_map commandLine;
   processCommandLine (commandLine, argc, argv);

   gtBase *app;

   if (commandLine.count (DOWNLOAD_CLI_OPT))
   {
      app = new gtDownload (commandLine);
   }
   else if (commandLine.count (UPLOAD_FILE_CLI_OPT))
   {
      app = new gtUpload (commandLine);
   }
   else  // default to Server mode as the mode was previously verified
   {
      app = new gtServer (commandLine);
   }

   app->run();

   delete app;

   return 0;
}
