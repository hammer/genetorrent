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

#include <config.h>

#include "tclapOutput.h"

#include "gtDefs.h"

void tclapOutput::failure(TCLAP::CmdLineInterface& c, TCLAP::ArgException& e)
{
   static_cast<void>(c); // Ignore input, don't warn
   std::string message = e.what();
   if (message.substr(0,4) == " -- ")
   {
      message = message.substr(4);
   }
   displayErrorMessage (message);
   usage(c);
   exit(1);
}

void tclapOutput::displayErrorMessage(std::string details)
{
   std::cerr << "GeneTorrent Encountered an Error:  Unable to process command line arguments\nError:  " << details << std::endl;
   std::cerr << "Cannot continue." << std::endl;
}

void tclapOutput::usage(TCLAP::CmdLineInterface& c)
{
   std::cout << "Usage:" << std::endl;
   std::cout << "   GeneTorrent -u manifest-file -c credentials [ -p path ]" << std::endl;
   std::cout << "   GeneTorrent -d [ URI | UUID | .xml | .gto ] -c credentials [ -p path ]" << std::endl;
   std::cout << "   GeneTorrent -s path -q work-queue -c credentials --security-api signing-URI" << std::endl;
   std::cout << std::endl;
   std::cout << "Type 'man GeneTorrent' for more information." << std::endl;
}

void tclapOutput::version(TCLAP::CmdLineInterface& c)
{
   static_cast<void>(c); // Ignore input, don't warn
   std::cout << SHORT_DESCRIPTION << " version " << VERSION << std::endl;
}
