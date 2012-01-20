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
 * loggingmasks.h
 *
 *  Created on: January 19, 2012
 *      Author: donavan
 */

#ifndef LOGGINGMASKS_H_
#define LOGGINGMASKS_H_

const uint64_t LOG_OUTGOING_CONNECTION        = 0x0000000000000001;
const uint64_t LOG_DISCONNECT                 = 0x0000000000000002;

// const uint64_t VARIABLE                    = 0x0000000000000004;
// const uint64_t VARIABLE                    = 0x0000000000000008;

// const uint64_t VARIABLE                    = 0x0000000000000010;
// const uint64_t VARIABLE                    = 0x0000000000000020;
// const uint64_t VARIABLE                    = 0x0000000000000040;
// const uint64_t VARIABLE                    = 0x0000000000000080;

// const uint64_t VARIABLE                    = 0x0000000000000100;
// const uint64_t VARIABLE                    = 0x0000000000000200;
// const uint64_t VARIABLE                    = 0x0000000000000400;
// const uint64_t VARIABLE                    = 0x0000000000000800;

// const uint64_t VARIABLE                    = 0x0000000000001000;
// const uint64_t VARIABLE                    = 0x0000000000002000;
// const uint64_t VARIABLE                    = 0x0000000000004000;
// const uint64_t VARIABLE                    = 0x0000000000008000;

// const uint64_t VARIABLE                    = 0x0000000000010000;
// const uint64_t VARIABLE                    = 0x0000000000020000;
// const uint64_t VARIABLE                    = 0x0000000000040000;
// const uint64_t VARIABLE                    = 0x0000000000080000;

// const uint64_t VARIABLE                    = 0x0000000000100000;
// const uint64_t VARIABLE                    = 0x0000000000200000;
// const uint64_t VARIABLE                    = 0x0000000000400000;
// const uint64_t VARIABLE                    = 0x0000000000800000;

// const uint64_t VARIABLE                    = 0x0000000001000000;
// const uint64_t VARIABLE                    = 0x0000000002000000;
// const uint64_t VARIABLE                    = 0x0000000004000000;
// const uint64_t VARIABLE                    = 0x0000000008000000;

// const uint64_t VARIABLE                    = 0x0000000010000000;
// const uint64_t VARIABLE                    = 0x0000000020000000;
// const uint64_t VARIABLE                    = 0x0000000040000000;
// const uint64_t VARIABLE                    = 0x0000000080000000;

// const uint64_t VARIABLE                    = 0x0000000100000000;
// const uint64_t VARIABLE                    = 0x0000000200000000;
// const uint64_t VARIABLE                    = 0x0000000400000000;
// const uint64_t VARIABLE                    = 0x0000000800000000;

// const uint64_t VARIABLE                    = 0x0000001000000000;
// const uint64_t VARIABLE                    = 0x0000002000000000;
// const uint64_t VARIABLE                    = 0x0000004000000000;
// const uint64_t VARIABLE                    = 0x0000008000000000;

// const uint64_t VARIABLE                    = 0x0000010000000000;
// const uint64_t VARIABLE                    = 0x0000020000000000;
// const uint64_t VARIABLE                    = 0x0000040000000000;
// const uint64_t VARIABLE                    = 0x0000080000000000;

// const uint64_t VARIABLE                    = 0x0000100000000000;
// const uint64_t VARIABLE                    = 0x0000200000000000;
// const uint64_t VARIABLE                    = 0x0000400000000000;
// const uint64_t VARIABLE                    = 0x0000800000000000;

// const uint64_t VARIABLE                    = 0x0001000000000000;
// const uint64_t VARIABLE                    = 0x0002000000000000;
// const uint64_t VARIABLE                    = 0x0004000000000000;
// const uint64_t VARIABLE                    = 0x0008000000000000;

// const uint64_t VARIABLE                    = 0x0010000000000000;
// const uint64_t VARIABLE                    = 0x0020000000000000;
// const uint64_t VARIABLE                    = 0x0040000000000000;
// const uint64_t VARIABLE                    = 0x0080000000000000;

// const uint64_t VARIABLE                    = 0x0100000000000000;
// const uint64_t VARIABLE                    = 0x0200000000000000;
// const uint64_t VARIABLE                    = 0x0400000000000000;
// const uint64_t VARIABLE                    = 0x0800000000000000;

// const uint64_t VARIABLE                    = 0x1000000000000000;
// const uint64_t VARIABLE                    = 0x2000000000000000;
// const uint64_t VARIABLE                    = 0x4000000000000000;

const uint64_t LOG_UNDEFINED                  = 0x8000000000000000;          // Unknown notification types in all notification handling functions

// These define the 3 logging levels offered via the command line.

const uint64_t LOGMASK_STANDARD = LOG_OUTGOING_CONNECTION | LOG_DISCONNECT;

const uint64_t LOGMASK_VERBOSE = LOGMASK_STANDARD;

const uint64_t LOGMASK_FULL = LOGMASK_VERBOSE | LOG_UNDEFINED;

#endif
