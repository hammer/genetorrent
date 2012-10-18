#!/usr/bin/env python
#
# Copyright (c) 2012, Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the University of California nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL REGENTS OF THE UNIVERSITY OF CALIFORNIA BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

'''Script to automate the generation of cghub data.

Data should be suitable for uploading using cgsubmit.

Optional upload_server_uri argument to create_data will cause
cg-data-gen to create a dummy manifest file, as if the
file had been submitted to a cghub instance.

See: https://cghub.ucsc.edu/submission_quickstart.html
'''

import os
import sys
import time
import hashlib
import logging
import random
from uuid import uuid4

import analysis
import experiment
import run
import manifest

log = logging.getLogger (__name__)

xml_file = ('<FILE checksum="%(bam_md5sum)s" checksum_method="MD5" '
            'filetype="bam" filename="%(bam_filename)s"/>')

class DataGenBase (object):
    '''Class for data file generation.
    '''

    def __init__ (self, uuid, data_size, upload_server_uri='', file_list=None, path=''):
        self.uuid = str(uuid)
        self.data_size = data_size
        self.upload_server_uri = upload_server_uri
        self.path = path
        if file_list is None:
            self.file_list = [ ('%s.bam' % (uuid), data_size) ]
        else:
            total_size = 0
            self.file_list = list(file_list)
            for f,sz in self.file_list:
                total_size += sz

            if total_size != data_size:
                raise Exception ('accumulated data size does not match requested size',
                                 'accum=%d' % total_size, 'req=%d' % data_size)

        log.info ('Creating data dir: %s' % uuid)

        self.timestamp = time.strftime('%Y%m%d-%H:%M:%S', time.gmtime())

        self.create_data ()

    def create_data (self):
        file_entries = []
        offset = 0
        for bam_filename, sz in self.file_list:
            log.debug ('  FILE: %-32s: %d bytes at %d' % (bam_filename, sz, offset))
            path = os.path.join(self.path, self.uuid)

            bam_path = os.path.join(path, bam_filename)

            d_path = os.path.dirname (bam_path)
            if not os.path.isdir(d_path):
                log.debug ('    mkdir %s' % d_path)
                os.makedirs (d_path, 0755)

            # Create the bam file.
            md5sum = hashlib.md5 ()
            with open (bam_path, 'wb') as fp:
                self.write_data(fp, md5sum, sz, offset)

            fentry = {
                'bam_md5sum': md5sum.hexdigest (),
                'bam_filename': bam_filename,
                }

            file_entries.append (xml_file % fentry)
            offset += sz

        bam_info = {
            'alias': 'Generated test data: %s' % (self.timestamp),
            'title': 'Generated test data (%s) %s' % (self.uuid, self.timestamp),
            'file_entries': '\n        '.join(file_entries),
            'uuid': self.uuid,
            'server': self.upload_server_uri,
            }

        with open(os.path.join(self.path, self.uuid, 'analysis.xml'), 'wb') as fp:
            fp.write(analysis.xml_template % bam_info)

        with open(os.path.join(self.path, self.uuid, 'experiment.xml'), 'wb') as fp:
            fp.write(experiment.xml_template % bam_info)

        with open(os.path.join(self.path, self.uuid, 'run.xml'), 'wb') as fp:
            fp.write(run.xml_template % bam_info)

        if bam_info['server']:
            with open(os.path.join(self.path, self.uuid, 'manifest-generated.xml'),
                      'wb') as fp:
                fp.write(manifest.xml_template % bam_info)


class DataGenRandom (DataGenBase):
    '''Class for generating files with random data.
    '''

    def write_data (self, fp, md5sum, data_size, offset):
        seed = '%s-%d' % (self.timestamp, offset)
        fp.write('# Generated BAM from Random Data: (%s) with seed = %s\n'
                 % (self.uuid, seed))
        random.seed(seed)
        blk_size = 4096
        while data_size > 0:
            if data_size < blk_size:
                blk_size = data_size

            blk = []
            for i in range (blk_size):
                d = random.randrange (0, 256)
                blk.append (chr (d))

            d_blk = ''.join (blk)
            md5sum.update (d_blk)
            fp.write (d_blk)
            data_size -= blk_size


class DataGenZero (DataGenBase):
    '''Class for generating files with zero data.
    '''

    def write_data (self, fp, md5sum, data_size, offset):
        blk_size = 4096
        while data_size > 0:
            if data_size < blk_size:
                blk_size = data_size

            d_blk = ''.join([chr(0)] * blk_size)
            md5sum.update (d_blk)
            fp.write (d_blk)
            data_size -= blk_size


class DataGenSequential (DataGenBase):
    '''Class for generating files with sequential data.
    '''
    PIECE_SZ = 4 * 1024 * 1024
    BLK_SZ = 16 * 1024
    DATUM_SZ = 16

    BLK_RANGE = PIECE_SZ / BLK_SZ
    DATUM_RANGE = BLK_SZ / DATUM_SZ

    def write_data (self, fp, md5sum, data_size, offset):
        if (offset % self.DATUM_SZ) != 0:
            raise Exception ("Offset is not a multiple of %d" % self.DATUM_SZ,
                             offset)

        piece = offset / self.PIECE_SZ
        blk_start = (offset % self.PIECE_SZ) / self.BLK_SZ
        datum_start = (offset % self.PIECE_SZ) % (self.BLK_SZ) / self.DATUM_SZ

        log.debug ('    P=%d, B=%d, D=%s' % (piece, blk_start, datum_start))

        while data_size > 0:
            for chunk in range (blk_start, self.BLK_RANGE):
                for datum in range (datum_start, self.DATUM_RANGE):
                    data_size -= 16
                    if data_size < 0:
                        return

                    d_blk = '%08x:%02x:%03x\n' % (piece, chunk, datum)
                    md5sum.update (d_blk)
                    fp.write (d_blk)
                datum_start = 0
            blk_start = 0
            piece += 1

if __name__ == '__main__':
    D_SZ    = 6 * 4096 * 1024
    BASE_SZ = 4096 * 1024 + 128

    D_FILES = [
        ('test-1.bam',           BASE_SZ),
        ('test-2.bam',           BASE_SZ),
        ('dir1/test-3.bam',      BASE_SZ),
        ('dir1/dir2/test-4.bam', BASE_SZ),
        ('dir3/test-5.bam',      D_SZ - 4 * BASE_SZ),
    ]

    DataGenSequential ('uuid-1', D_SZ)
    DataGenSequential ('uuid-2', D_SZ, file_list=[ ('test-1.bam', D_SZ ) ])
    DataGenSequential ('uuid-3', D_SZ, 'dummy-server', file_list=D_FILES,
                       path='tmp-1')

    DataGenRandom ('uuid-4', D_SZ, path='tmp-1')
    DataGenRandom ('uuid-5', D_SZ, file_list=[ ('test-1.bam', D_SZ ) ])
    DataGenRandom ('uuid-6', D_SZ, file_list=D_FILES)

    DataGenZero ('uuid-7', D_SZ)
    DataGenZero ('uuid-8', D_SZ, file_list=[ ('test-1.bam', D_SZ ) ])
    DataGenZero ('uuid-9', D_SZ, file_list=D_FILES)
