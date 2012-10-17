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

''' Unit tests for credential-as-URI feature '''

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
from utils.config import TestConfig

class TestGeneTorrentCredAsURI(GTTestCase):
    create_mockhub = True
    create_credential = True

    def test_1mb_download_from_uuid_cred_as_uri(self):
        '''Download a 1MB file from a GT server,
           using a http/https credential file.'''
        uuid = self.data_upload_test(1024 * 1024 * 1)

        hub_server = TestConfig.HUB_SERVER

        # credential URI is OK
        print TestConfig.HUB_SERVER
        self.data_download_test_uuid(uuid,
            client_options='-c %s/credential/DUMMY-CREDENTIAL.key'
            % hub_server)

        # 404 error
        self.data_download_test_uuid(uuid,
            client_options='-c %s/does-not-exist.key' % hub_server,
            assert_rc=9, assert_serr='Failed to download authentication token')

        # wrong protocol
        http_hub = TestConfig.HUB_SERVER.replace('https', 'http')
        self.data_download_test_uuid(uuid,
            client_options='-c %s/DUMMY-CREDENTIAL.key' % http_hub,
            assert_rc=9, assert_serr='Failed to download authentication token')

        # wrong protocol
        ftp_hub = TestConfig.HUB_SERVER.replace('https', 'ftp')
        self.data_download_test_uuid(uuid,
            client_options='-c %s/DUMMY-CREDENTIAL.key' % ftp_hub,
            assert_rc=9, assert_serr='Failed to download authentication token')

if __name__ == '__main__':
    sys.stdout = StreamToLogger(logging.getLogger('stdout'), logging.INFO)
    sys.stderr = StreamToLogger(logging.getLogger('stderr'), logging.WARN)
    suite = unittest.TestLoader().loadTestsFromTestCase(TestGeneTorrentCredAsURI)
    result = unittest.TextTestRunner(stream=sys.stderr, verbosity=2).run(suite)
    if not result.wasSuccessful():
        sys.exit(1)

