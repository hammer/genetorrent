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

''' Unit tests for upload client '''

import unittest
import time
import os

from uuid import uuid4
from shutil import copy2
try:
    from hashlib import sha1 as hash_sha1
except ImportError:
    from sha import new as hash_sha1

from utils.gttestcase import GTTestCase
from utils.genetorrent  import GeneTorrentInstance
from utils.cgdata.datagen import create_data, write_random_data
from utils.config import TestConfig

class TestGeneTorrentUpload(GTTestCase):
    create_mockhub = True
    create_credential = True

    def test_1mb_upload_from_analysis_file(self):
        '''Upload a randomly-generated 1MB file to a GT server.'''
        self.data_upload_test(1024 * 1024 * 1)

    def test_1mb_upload_from_analysis_file_nossl(self):
        '''Upload a randomly-generated 1MB file to a GT server
           without SSL'''

        # Only mockhub has no-ssl capabilities
        if not TestConfig.MOCKHUB:
            return

        self.data_upload_test(1024 * 1024 * 1, ssl=False)

if __name__ == '__main__':
    unittest.main()

