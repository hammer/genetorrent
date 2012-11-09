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

''' Unit tests for client inactivity timeouts. '''

import unittest
import time
import os
import logging
import sys

from uuid import uuid4
from shutil import copy2

from utils.gttestcase import GTTestCase, StreamToLogger
from utils.genetorrent  import GeneTorrentInstance, InstanceType
from utils.config import TestConfig

class TestGeneTorrentInactivityTimeout(GTTestCase):
    create_mockhub = True
    create_credential = True

    def test_upload_download_client_timeout(self):
        '''Test upload/download client inactivity timeout.'''
        uuid = uuid4()
        self.generate_bam_data(uuid, 1024 * 1024 * 1)

        # no server in this test

        start_time = time.clock()

        # prepare upload client
        client_manifest = os.path.join(
            'client',
            str(uuid),
            'manifest-generated.xml',
        )
        client = GeneTorrentInstance('-u %s -p client -c %s ' \
            '-k 1 -vv -l stdout:full -C . --ssl-no-verify-ca' \
            % (client_manifest, self.cred_filename), instance_type=InstanceType.GT_UPLOAD,
            add_defaults=False)

        time.sleep(5)

        # wait for upload client to exit
        client_sout, client_serr = client.communicate()

        if TestConfig.VERBOSE:
            print 'client stdout: ' + client_sout
            print 'client stderr: ' + client_serr


        self.assertEqual(client.returncode, 206)
        self.assertTrue('Inactivity timeout triggered' in client_sout)

        # upload client properly timed out
        # now, test download client

        # delete bam first, else client will exit since it has the file
        client_bam = self.client_bam(uuid)

        os.remove(client_bam)

        client = GeneTorrentInstance( \
            '-d %s/cghub/data/analysis/%s -p client -c %s ' \
            '-k 1 -vv -l stdout:full -C . --ssl-no-verify-ca' \
            % (TestConfig.HUB_SERVER, str(uuid), self.cred_filename),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)

        time.sleep(5)

        # wait for download client to exit
        client_sout, client_serr = client.communicate()

        if TestConfig.VERBOSE:
            print 'client stdout: ' + client_sout
            print 'client stderr: ' + client_serr

        self.assertEqual(client.returncode, 206)
        self.assertTrue('Inactivity timeout triggered' in client_sout)

        end_time = time.clock()

        # that should take a little more than 2 minutes,
        # allowing for two 60-second timeouts
        self.assertTrue(end_time - start_time < 200,
            'Two inactivity timeouts took longer than two minutes')

if __name__ == '__main__':
    sys.stdout = StreamToLogger(logging.getLogger('stdout'), logging.INFO)
    sys.stderr = StreamToLogger(logging.getLogger('stderr'), logging.WARN)
    suite = unittest.TestLoader().loadTestsFromTestCase(TestGeneTorrentInactivityTimeout)
    result = unittest.TextTestRunner(stream=sys.stderr, verbosity=2).run(suite)
    if not result.wasSuccessful():
        sys.exit(1)

