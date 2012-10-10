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
from utils.cgdata.datagen import write_zero_data, write_random_data
from utils.config import TestConfig

class TestGeneTorrentLargeDownload(GTTestCase):
    create_mockhub = True
    create_credential = True

    def test_20gb_download_from_uuid(self):
        '''Download a 20GB zero-data file from a GT server.'''
        uuid = self.data_upload_test(1024 * 1024 * 1024 * 20,
            data_generator=write_zero_data,
            server_options='--zero-storage',
            check_sha1=False)
        self.data_download_test_uuid(uuid, client_options='--max-children=4 --zero-storage',
            server_options='--zero-storage',
            check_sha1=False)

if __name__ == '__main__':
    sys.stdout = StreamToLogger(logging.getLogger('stdout'), logging.INFO)
    sys.stderr = StreamToLogger(logging.getLogger('stderr'), logging.WARN)
    suite = unittest.TestLoader().loadTestsFromTestCase(TestGeneTorrentLargeDownload)
    result = unittest.TextTestRunner(stream=sys.stderr, verbosity=2).run(suite)
    if not result.wasSuccessful():
        sys.exit(1)

