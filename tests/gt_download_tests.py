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

class TestGeneTorrentDownload(GTTestCase):
    create_mockhub = True
    create_credential = True

    def test_100mb_download_from_uuid(self):
        '''Download a 100MB zero-data file from a GT server.'''
        uuid = self.data_upload_test(1024 * 1024 * 100,
            data_generator=write_zero_data)
        self.data_download_test_uuid(uuid, client_options='--max-children=4')

    def test_1mb_download_from_uuid(self):
        '''Download a randomly-generated 1MB file from a GT server.'''
        uuid = self.data_upload_test(1024 * 1024 * 1)

        self.data_download_test_uuid(uuid)

    def test_1mb_download_from_gto(self):
        '''Download a randomly-generated 1MB file from a GT server.'''
        # GTO downloads must be manually added to server workqueue
        # therefore, this test will not work remotely
        if not TestConfig.MOCKHUB:
            return
        uuid = self.data_upload_test(1024 * 1024 * 1)

        self.data_download_test_gto(uuid)

    def test_1mb_download_from_uuid_nossl(self):
        '''Download a randomly-generated 1MB file from a GT server
           without SSL'''
        if not TestConfig.MOCKHUB:
            return

        uuid = self.data_upload_test(1024 * 1024 * 1, ssl=False)

        self.data_download_test_uuid(uuid)

    def test_1mb_download_from_gto_nossl(self):
        '''Download a randomly-generated 1MB file from a GT server
           without SSL'''
        if not TestConfig.MOCKHUB:
            return

        uuid = self.data_upload_test(1024 * 1024 * 1, ssl=False)

        self.data_download_test_gto(uuid)

    def test_3x1mb_download_from_xml(self):
        '''Download three randomly-generated 1MB files from a GT server
           via an XML manifest'''
        uuid1 = self.data_upload_test(1024 * 1024 * 1)
        uuid2 = self.data_upload_test(1024 * 1024 * 1)
        uuid3 = self.data_upload_test(1024 * 1024 * 1)

        uuids = [uuid1, uuid2, uuid3]

        # build a XML result set
        result_set = Element('ResultSet')
        result_1 = SubElement(result_set, 'Result')
        analysis_data_uri_1 = SubElement(result_1, 'analysis_data_uri')
        analysis_data_uri_1.text = '%s/cghub/data/analysis/download/' \
            % TestConfig.HUB_SERVER + str(uuid1)
        result_2 = SubElement(result_set, 'Result')
        analysis_data_uri_2 = SubElement(result_2, 'analysis_data_uri')
        analysis_data_uri_2.text = '%s/cghub/data/analysis/download/' \
            % TestConfig.HUB_SERVER + str(uuid2)
        result_3 = SubElement(result_set, 'Result')
        analysis_data_uri_3 = SubElement(result_3, 'analysis_data_uri')
        analysis_data_uri_3.text = '%s/cghub/data/analysis/download/' \
            % TestConfig.HUB_SERVER + str(uuid3)

        doc = ElementTree(result_set)

        f = NamedTemporaryFile(delete=False, suffix='.xml')
        doc.write(f)
        f.close()

        self.data_download_test_xml(f.name, uuids)

        os.remove(f.name)

    def test_1mb_download_from_uuid_null_storage(self):
        '''Download a randomly-generated 1MB file from a GT server
           with null-storage option.'''
        uuid = self.data_upload_test(1024 * 1024 * 1)

        self.data_download_test_uuid(uuid,
            client_options='--null-storage', check_sha1=False)

        # client BAM should not exist
        client_bam = os.path.join(
            'client',
            str(uuid),
            str(uuid) + '.bam',
        )

        self.assertTrue(not os.path.isfile(client_bam))

    def test_1mb_download_from_uuid_zero_storage(self):
        '''Download a randomly-generated 1MB file from a GT server
           with zero-storage option.'''

        # if this test was large and fast enough, it would be an
        # expected fail scenario
        # if libtorrent's disk cache doesn't keep up with the data
        # transfer, piece hash checks will begin to fail
        uuid = self.data_upload_test(1024 * 1024 * 1)

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
    unittest.TextTestRunner(stream=sys.stderr, verbosity=2).run(suite)

