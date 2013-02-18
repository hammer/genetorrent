/* -*- mode: C++; c-basic-offset: 4; tab-width: 4; -*-
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

#include "gt_config.h"

#include "gtDownloadOpts.h"
#include "gtOptStrings.h"
#include "gt_scm_rev.h"

static const char usage_msg_hdr[] =
    "Usage:\n"
    "   gtdownload [OPTIONS] -c <cred> <URI|UUID|.xml|.gto>\n"
    "\n"
    "For more detailed information on gtdownload, see the manual pages.\n"
#if __CYGWIN__
    "The manual pages can be found as text files in the install directory.\n"
#else /* __CYGWIN__ */
    "Type 'man gtdownload' to view the manual page.\n"
#endif /* __CYGWIN__ */
    "\n"
    "Options"
    ;

static const char version_msg[] =
    "GeneTorrent gtdownload release " VERSION " (SCM REV: " GT_SCM_REV_STR ")"
    ;

gtDownloadOpts::gtDownloadOpts ():
    gtBaseOpts ("gtdownload", usage_msg_hdr, version_msg, "DOWNLOAD"),
    m_dl_desc ("Download Options"),
    m_maxChildren (8),
    m_downloadSavePath (""),
    m_cliArgsDownloadList (),
    m_downloadModeCsrSigningUrl (),
    m_gtAgentMode (false)
{
}

// Pass through constructor for use by derived classes.
gtDownloadOpts::gtDownloadOpts (std::string progName, std::string usage_hdr,
                                std::string ver_msg, std::string mode):
    gtBaseOpts (progName, usage_hdr, ver_msg, mode),
    m_dl_desc ("Download Options"),
    m_maxChildren (8),
    m_downloadSavePath (""),
    m_cliArgsDownloadList (),
    m_downloadModeCsrSigningUrl (),
    m_gtAgentMode (false)
{
}

void
gtDownloadOpts::add_options ()
{
    // Download options
    m_dl_desc.add_options ()
        (OPT_DOWNLOAD            ",d", opt_vect_str()->composing(),
                                                     "<URI|UUID|.xml|.gto>")
        (OPT_MAX_CHILDREN,             opt_int(),    "number of download children")
        ;
    add_desc (m_dl_desc);

    boost::program_options::options_description gta_desc;
    gta_desc.add_options ()
        (OPT_GTA_MODE,          "Operating as a child of GTA, this option is hidden")
        ;
    add_desc (gta_desc, NOT_VISIBLE, CLI_ONLY);

    add_options_hidden ('D');

    gtBaseOpts::add_options ();
}

void
gtDownloadOpts::add_positionals ()
{
    m_pos.add (OPT_DOWNLOAD, -1);
}

void
gtDownloadOpts::processOptions ()
{
    gtBaseOpts::processOptions ();

    if (m_vm.count (OPT_DOWNLOAD) == 0)
    {
        commandLineError ("Download command line or config file must "
                          "include a content-specifier argument.");
    }

    processOption_MaxChildren ();
    processOption_DownloadList ();
    processOption_SecurityAPI ();
    processOption_InactiveTimeout ();
    processOption_RateLimit();

    m_downloadSavePath = processOption_Path ();
}

void
gtDownloadOpts::processOption_MaxChildren ()
{
    if (m_vm.count (OPT_MAX_CHILDREN) == 1)
    {
        m_maxChildren = m_vm[OPT_MAX_CHILDREN].as< int >();
    }

    if (m_maxChildren < 1)
    {
        commandLineError ("Value for '--" OPT_MAX_CHILDREN
                          "' must be greater than 0");
    }
}

void
gtDownloadOpts::processOption_DownloadList ()
{
    if (m_vm.count (OPT_DOWNLOAD))
        m_cliArgsDownloadList = m_vm[OPT_DOWNLOAD].as<vectOfStr>();

    vectOfStr::iterator vectIter = m_cliArgsDownloadList.begin ();

    bool needCreds = false;

    // Check the list of -d arguments to see if any require a credential file
    while ((vectIter != m_cliArgsDownloadList.end ()) && (needCreds == false))
    {
        std::string inspect = *vectIter;

        if (inspect.size () > 4) // Check for GTO file
        {
            if (inspect.substr (inspect.size () - 4) == GTO_FILE_EXTENSION)
            {
                vectIter++;
                continue;
            }
        }

        needCreds = true;
        vectIter++;
    }

    if (needCreds)
    {
        checkCredentials ();
    }
}

/**
 * Should only be called by gtBaseOpts::processOption_Verbosity().
 *
 * This overrides the empty default in gtBaseOpts.
 */
void
gtDownloadOpts::processOption_GTAgentMode ()
{
    if (m_vm.count (OPT_GTA_MODE))
    {
        global_gtAgentMode = true;

        if (global_verbosity > 0)
        {
            commandLineError ("The '--gta' option may not be combined with either -v"
                              " or '--verbose' options.");
        }
    }
}
