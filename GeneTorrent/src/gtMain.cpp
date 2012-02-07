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

#include "gtDefs.h"
// #include "geneTorrent.h"

#include "gtUpload.h"
#include "gtServer.h"
#include "gtDownload.h"

std::string makeOpt (std::string baseName, char secondName = SPACE)
{
   if (secondName != SPACE)
   {
      return baseName + "," + secondName;
   }
   return baseName + "," +baseName.substr(0,1);
}

void configureCommandLineOptions (boost::program_options::options_description &options)
{
   // The descriptions below are not used in the help message.
   options.add_options() 
         (makeOpt (HELP_CLI_OPT).c_str(), "Help Message")          
         (VERSION_CLI_OPT.c_str(),       "Display Version")       // long option only
         (makeOpt (BIND_IP_CLI_OPT).c_str(), boost::program_options::value< std::string >(), "Bind IP")        // long and short option using first letter of long option
         (CONFIG_FILE_CLI_OPT.c_str(), boost::program_options::value< std::string >(), "path/file to optional config file")
         (makeOpt (CRED_FILE_CLI_OPT).c_str(), boost::program_options::value< std::string >(), "path/file to credentials file")          
         (makeOpt (CONF_DIR_CLI_OPT, 'C').c_str(), boost::program_options::value< std::string >(), "full path to SSL configuration files")        // long option with alternate short option
         (makeOpt (ADVERT_IP_CLI_OPT, 'e').c_str(), boost::program_options::value< std::string >(), "IP Address advertised")    
         (makeOpt (ADVERT_PORT_CLI_OPT, 'f').c_str(), boost::program_options::value< uint32_t >(), "TCP Port advertised")      
         (makeOpt (INTERNAL_PORT_CLI_OPT).c_str(), boost::program_options::value< uint32_t >(), "local IP port to bind on")     
         (makeOpt (LOGGING_CLI_OPT).c_str(), boost::program_options::value< std::string >(), "path/file to log file, follow by the log level")  
         (makeOpt (PATH_CLI_OPT).c_str(), boost::program_options::value< std::string >(), "file system path used for uploads and downloads")

         // Download
         (makeOpt (DOWNLOAD_CLI_OPT).c_str(), boost::program_options::value< std::vector <std::string> >()->composing(), "URI | UUID | .xml | .gto")
         (MAX_CHILDREN_CLI_OPT.c_str(), boost::program_options::value< uint32_t >(), "number of download children")

         // Upload
         (makeOpt (UPLOAD_FILE_CLI_OPT).c_str(), boost::program_options::value< std::string >(), "uuid/manifest.xml")

         // Server Mode
         (makeOpt (SERVER_CLI_OPT).c_str(), boost::program_options::value< std::string >(),"server data path")
         (makeOpt (QUEUE_CLI_OPT).c_str(), boost::program_options::value< uint32_t >(), "input GTO directory")    
         (SECURITY_API_CLI_OPT.c_str(), boost::program_options::value< uint32_t >(), "SSL Key Signing URL")    

         // Legacy long option names
         (BIND_IP_CLI_OPT_LEGACY.c_str(), boost::program_options::value< std::string >(), "Bind IP")        
         (CRED_FILE_CLI_OPT_LEGACY.c_str(), boost::program_options::value< std::string >(), "path/file to credentials file")          
         (CONF_DIR_CLI_OPT_LEGACY.c_str(), boost::program_options::value< std::string >(), "full path to SSL configuration files")          
         (ADVERT_IP_CLI_OPT_LEGACY.c_str(), boost::program_options::value< std::string >(), "IP Address advertised")         
         (ADVERT_PORT_CLI_OPT_LEGACY.c_str(), boost::program_options::value< uint32_t >(), "TCP Port advertised")          
         (INTERNAL_PORT_CLI_OPT_LEGACY.c_str(), boost::program_options::value< uint32_t >(), "local IP port to bind on")          
         (UPLOAD_FILE_CLI_OPT_LEGACY.c_str(), boost::program_options::value< std::string >(), "manifest.xml")
         (MAX_CHILDREN_CLI_OPT_LEGACY.c_str(), boost::program_options::value< uint32_t >(), "number of download children")
      ;
}

void processCommandLine (boost::program_options::variables_map &clOptions, int argc, char **argv)
{
   try
   {
      boost::program_options::options_description commandLineOpts("Command Line Options");
      configureCommandLineOptions (commandLineOpts);

      boost::program_options::variables_map cli;
      boost::program_options::store (boost::program_options::parse_command_line (argc, argv, commandLineOpts), cli);

      // Check if a config file was specified
      if (cli.count (CONFIG_FILE_CLI_OPT) == 1)
      {
         std::ifstream inputFile(cli[CONFIG_FILE_CLI_OPT].as<std::string>().c_str());
         if (!inputFile)
         {
            std::cerr << "error: unable to open config file '" << cli[CONFIG_FILE_CLI_OPT].as<std::string>() << "'." << std::endl; 
            exit(10); 
         }
         boost::program_options::store (boost::program_options::parse_config_file (inputFile, commandLineOpts), cli);
      }

      // Check if a help was requested
      if (cli.count (HELP_CLI_OPT))
      {
// DJN remove these
std::cout << commandLineOpts << std::endl;
std::cout << "\n\n\n";
         std::cout << "Usage:" << std::endl;
         std::cout << "   GeneTorrent -u manifest-file -c credentials [ -p path ] [--config-file path/file]" << std::endl;
         std::cout << "   GeneTorrent -d [ URI | UUID | .xml | .gto ] -c credentials [ -p path ] [--config-file path/file]" << std::endl;
         std::cout << "   GeneTorrent -s path -q work-queue -c credentials --security-api signing-URI [--config-file path/file]" << std::endl;
         std::cout << std::endl;
         std::cout << "Type 'man GeneTorrent' for more information." << std::endl;
         exit (11);
      }

      if (cli.count (SERVER_CLI_OPT) == 0 && cli.count (DOWNLOAD_CLI_OPT) == 0 && cli.count (UPLOAD_FILE_CLI_OPT) == 0 && cli.count (UPLOAD_FILE_CLI_OPT_LEGACY) == 0)
      {
         std::cerr << "Command line or config file must include one of -d (download), -s (server), or -u (upload)." << std::endl;
         exit(12);
      }

      if ((cli.count (SERVER_CLI_OPT) > 0 && cli.count (DOWNLOAD_CLI_OPT) > 0 ) || 
          ((cli.count (UPLOAD_FILE_CLI_OPT_LEGACY) > 0 || cli.count (UPLOAD_FILE_CLI_OPT)) > 0 && cli.count (SERVER_CLI_OPT) > 0 ) ||
          (cli.count (DOWNLOAD_CLI_OPT) > 0 && (cli.count (UPLOAD_FILE_CLI_OPT_LEGACY) > 0 || cli.count (UPLOAD_FILE_CLI_OPT) > 0))) 
      { 
         std::cerr << "Command line or config file may only specifiy one of -d (download), -s (server), or -u (upload)." << std::endl; 
      } 

      boost::program_options::notify (cli); 
      clOptions = cli; 
   } 
   catch(std::exception &e) 
   { 
      std::cerr << "error: " << e.what() << std::endl; 
      exit(13); 
   }
   catch(...) 
   { 
      std::cerr << "error: unable to parse command line" << std::endl; 
      exit(14); 
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

// std::cerr << "download = ";

//    const std::vector <std::string> &s = commandLine[DOWNLOAD_CLI_OPT].as< std::vector < std::string> >();
// copy(s.begin(), s.end(), std::ostream_iterator<std::string>(std::cerr, " ")); 
// std::cerr << std::endl;

//<< "Include paths are: " << vm          ["include-path"]  .as< vector<string> >() << "\n";
//.as< std::vector <std::string> >() << std::endl;
// << cli[BIND_IP_CLI_OPT].as<std::string>()

//std::cerr << "inside value = " << cli[BIND_IP_CLI_OPT].as<std::string>() << std::endl;
//std::cerr << "inside 2nd value = " << clOptions[BIND_IP_CLI_OPT].as<std::string>() << std::endl;
//std::cerr << "value = " << commandLine[BIND_IP_CLI_OPT].as<std::string>() << std::endl;

std::cerr << "forced exit" << std::endl;
exit(1);
   
//   geneTorrent app (argc, argv);

   app->run();
   return 0;
}
