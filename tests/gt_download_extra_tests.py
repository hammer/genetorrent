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

''' Unit tests for download client '''

import unittest
import time
import os
import logging
import sys

from uuid import uuid4
from shutil import copy2
from xml.etree.cElementTree import Element, ElementTree, SubElement
from tempfile import NamedTemporaryFile

from utils.gttestcase import GTTestCase, StreamToLogger
from utils.cgdata.datagen import DataGenZero
from utils.config import TestConfig

class TestGeneTorrentDownload(GTTestCase):
    create_mockhub = True
    create_credential = True

    def test_32mb_download_from_uuid_nossl(self):
        '''Download a randomly-generated 32MB file from a GT server
           without SSL'''
        if not TestConfig.MOCKHUB:
            return

        uuid = self.data_upload_test(1024 * 1024 * 32, ssl=False)

        self.data_download_test_uuid(uuid)

    def test_32mb_download_from_gto_nossl(self):
        '''Download a randomly-generated 32MB file from a GT server
           without SSL'''
        if not TestConfig.MOCKHUB:
            return

        uuid = self.data_upload_test(1024 * 1024 * 32, ssl=False)

        self.data_download_test_gto(uuid)

    def test_32mb_download_from_uuid_null_storage(self):
        '''Download a randomly-generated 1MB file from a GT server
           with null-storage option.'''
        uuid = self.data_upload_test(1024 * 1024 * 32)

        self.data_download_test_uuid(uuid,
            client_options='--null-storage', check_sha1=False)

        # client BAM should not exist
        client_bam = os.path.join(
            'client',
            str(uuid),
            str(uuid) + '.bam',
        )

        self.assertTrue(not os.path.isfile(client_bam))

    def test_32mb_download_from_uuid_zero_storage(self):
        '''Download a randomly-generated 32MB file from a GT server
           with zero-storage option.'''

        # if this test was large and fast enough, it would be an
        # expected fail scenario
        # if libtorrent's disk cache doesn't keep up with the data
        # transfer, piece hash checks will begin to fail
        uuid = self.data_upload_test(1024 * 1024 * 32)

        self.data_download_test_uuid(uuid,
            client_options='--zero-storage', check_sha1=False)

        # client BAM should not exist
        client_bam = os.path.join(
            'client',
            str(uuid),
            str(uuid) + '.bam',
        )

        self.assertTrue(not os.path.isfile(client_bam))

if __name__ == '__main__':
    sys.stdout = StreamToLogger(logging.getLogger('stdout'), logging.INFO)
    sys.stderr = StreamToLogger(logging.getLogger('stderr'), logging.WARN)
    suite = unittest.TestLoader().loadTestsFromTestCase(TestGeneTorrentDownload)
    result = unittest.TextTestRunner(stream=sys.stderr, verbosity=2).run(suite)
    if not result.wasSuccessful():
        sys.exit(1)

