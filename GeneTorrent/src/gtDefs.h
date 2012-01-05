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

/*
 * gtDefs.h
 *
 *  Created on: Aug 15, 2011
 *      Author: donavan
 */

#ifndef GTDEFS_H_
#define GTDEFS_H_

#include <config.h>

// some constants
const std::string SHORT_DESCRIPTION = "GeneTorrent: Genomic Data Transfer Tool";
const std::string GTO_FILE_EXTENSION = ".gto";
const std::string CONF_DIR_DEFAULT = "/usr/share/GeneTorrent";
const std::string DH_PARAMS_FILE = "dhparam.pem";
const std::string GT_OPENSSL_CONF = "GeneTorrent.openssl.conf";

const long UNKNOWN_HTTP_HEADER_CODE = 987654321;        // arbitrary number
const int NO_EXIT = 0;
const int ERROR_NO_EXIT = -1;
const int SLEEP_INTERVAL = 4000000;    // in usec

const int64_t DISK_FREE_WARN_LEVEL = 1000 * 1000 * 1000;  // 1 GB, aka 10^9

// move to future config file
const std::string GT_CERT_SIGN_TAIL = "gtsession";
const std::string DEFAULT_CGHUB_HOSTNAME = "cghub.ucsc.edu";
const std::string CGHUB_WSI_BASE_URL = "https://"+ DEFAULT_CGHUB_HOSTNAME + "/cghub/data/analysis/";
const std::string DEFAULT_TRACKER_URL = "https://tracker.example.com/announce";

// Torrent status text for various GeneTorrent operational modes
//
// these are indexed by the torrent_status::state_t enum, found in torrent_handle.hpp

static char const* server_state_str[] = { 
	"checking (q)",								//			queued_for_checking,
	"checking",										//			checking_files,
	"dl metadata",								//			downloading_metadata,
	"GTOerror",										//			downloading,
	"finished",										//			finished,
	"serving", 										//			seeding,
	"allocating",									//			allocating,
	"checking (r)"								//			checking_resume_data
};

static char const* download_state_str[] = {
	"checking (q)",								//			queued_for_checking,
	"checking",										//			checking_files,
	"dl metadata",								//			downloading_metadata,
	"downloading",								//			downloading,
	"finished",										//			finished,
	"cleanup",										//			seeding,
	"allocating",									//			allocating,
	"checking (r)"								//			checking_resume_data
};

static char const* upload_state_str[] = { 
	"checking (q)",								//			queued_for_checking,
	"checking",										//			checking_files,
	"dl metadata",								//			downloading_metadata,
	"GTOerror",										//			downloading,
	"finished",										//			finished,
	"uploading",									//			seeding,
	"allocating",									//			allocating,
	"checking (r)"								//			checking_resume_data
};

#endif /* GTDEFS_H_ */
