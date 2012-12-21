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

#ifndef GT_DOWNLOAD_OPTS_H
#define GT_DOWNLOAD_OPTS_H

#include "gtBaseOpts.h"

class gtDownloadOpts: public gtBaseOpts
{
public:
    gtDownloadOpts ();
    gtDownloadOpts (std::string progName, std::string usage_hdr,
                    std::string ver_msg, std::string mode);
    ~gtDownloadOpts () {}

    boost::program_options::options_description m_dl_desc;

    // Storage for data extracted from config/cli.
    int m_maxChildren;
    std::string m_downloadSavePath;
    vectOfStr m_cliArgsDownloadList;
    std::string m_downloadModeCsrSigningUrl;
    bool m_gtAgentMode;

protected:
    virtual void add_options ();
    virtual void add_positionals ();
    virtual void processOptions ();

    virtual void processOption_GTAgentMode ();

private:
    void processOption_MaxChildren ();
    void processOption_DownloadList ();
};

#endif  /* GT_DOWNLOAD_OPTS_H */
