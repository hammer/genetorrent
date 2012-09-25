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
import random
from uuid import uuid4

import analysis
import experiment
import run
import manifest

def write_random_data(fp, uuid, seed, data_size):
    fp.write('# Generated BAM from Random Data: (%s) with seed = %s\n' % (uuid, seed))
    random.seed(seed)
    blk_size = 4096
    while data_size > 0:
        if data_size < blk_size:
            blk_size = data_size

        blk = []
        for i in range(blk_size):
            d = random.randrange(0, 256)
            blk.append(chr(d))

        fp.write(''.join(blk))
        data_size -= blk_size

def write_zero_data(fp, uuid, seed, data_size):
    blk_size = 4096
    while data_size > 0:
        if data_size < blk_size:
            blk_size = data_size

        fp.write(''.join([chr(0)] * blk_size))
        data_size -= blk_size

def get_md5(path):
    md5sum = hashlib.md5()
    with open(path, 'rb') as fp:
        while True:
            d = fp.read(4096)
            if not d:
                break
            md5sum.update(d)
    return md5sum.hexdigest()

def create_data(uuid, data_size, data_writer, upload_server_uri=""):
    path = '%s' % (uuid)
    print 'Creating: %s' % path

    os.mkdir(path)

    bam_filename = '%s.bam' % (uuid)
    bam_path = '%s/%s' % (path, bam_filename)

    timestamp = time.strftime('%Y%m%d-%H:%M:%S', time.gmtime())

    # Create the bam file.
    with open(bam_path, 'wb') as fp:
        data_writer(fp, uuid, timestamp, data_size)

    bam_info = {
        'alias': 'Random test data: %s' % (timestamp),
        'title': 'Random test data (%s) %s' % (uuid, timestamp),
        'bam_md5sum': get_md5(bam_path),
        'bam_filename': bam_filename,
        'bam_path': bam_path,
        'uuid': uuid,
        'server': upload_server_uri,
        }

    with open('%s/analysis.xml' % (uuid), 'wb') as fp:
        fp.write(analysis.xml_template % bam_info)

    with open('%s/experiment.xml' % (uuid), 'wb') as fp:
        fp.write(experiment.xml_template % bam_info)

    with open('%s/run.xml' % (uuid), 'wb') as fp:
        fp.write(run.xml_template % bam_info)

    if bam_info['server']:
        with open('%s/manifest-generated.xml' % (uuid), 'wb') as fp:
            fp.write(manifest.xml_template % bam_info)

