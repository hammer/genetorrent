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

''' Unit tests for client curl peer verify behavior. '''

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
from gtoinfo import read_gto, emit_gto

class TestGeneTorrentCurlVerifyCA(GTTestCase):
    create_mockhub = True
    create_credential = True

    def test_upload_download_client_peer_verify(self):
        '''Test upload/download client curl SSL peer verification behavior.'''
        uuid = uuid4()
        self.generate_bam_data(uuid, 1024)

        # no server in this test

        # prepare upload client
        client_manifest = os.path.join(
            'client',
            str(uuid),
            'manifest-generated.xml',
        )
        client = GeneTorrentInstance('-u %s -p client -c %s ' \
            % (client_manifest, self.cred_filename), ssl_no_verify_ca=False,
            instance_type = InstanceType.GT_UPLOAD)

        # wait for upload client to exit
        client_sout, client_serr = client.communicate()

        if TestConfig.VERBOSE:
            print 'client stdout: ' + client_sout
            print 'client stderr: ' + client_serr

        self.assertEqual(client.returncode, 203)
        self.assertTrue('Peer certificate cannot be authenticated' \
            in client_sout)

        # upload client properly exited
        # now, test download client

        client = GeneTorrentInstance( \
            '-d %s/cghub/data/analysis/%s -p client -c %s ' \
            % (TestConfig.HUB_SERVER, str(uuid), self.cred_filename),
            ssl_no_verify_ca=False, instance_type=InstanceType.GT_DOWNLOAD)

        # wait for upload client to exit
        client_sout, client_serr = client.communicate()

        if TestConfig.VERBOSE:
            print 'client stdout: ' + client_sout
            print 'client stderr: ' + client_serr

        self.assertEqual(client.returncode, 203)
        self.assertTrue('Peer certificate cannot be authenticated' in \
            client_sout)

    def test_server_peer_verify(self):
        '''Test server client curl SSL peer verification behavior.'''
        uuid = uuid4()
        self.generate_bam_data(uuid, 1024)
        self.create_gto_only(uuid)

        # now, test server without --ssl-no-verify-ca

        server = GeneTorrentInstance( \
            '-s %s -q %s -c %s --security-api ' \
            '%s ' \
            '-l stdout:full' \
            %  ('server' + os.path.sep + 'root',
                'server' + os.path.sep + 'workdir',
                self.cred_filename,
                TestConfig.SECURITY_API
                ), ssl_no_verify_ca=False, instance_type=InstanceType.GT_SERVER,
                add_defaults=False)

        # copy gto to server
        client_gto = self.client_gto(uuid)
        server_gto = self.server_gto(uuid)

        gtodict = read_gto(client_gto)
        gtodict['gto_download_mode'] = 'true'
        emit_gto(gtodict, server_gto, True)

        time.sleep(10)

        # kill server
        server.kill()

        # check for SSL signing error
        server.stdout_buffer.seek(0)
        server_sout = server.stdout_buffer.read()
        print server_sout
        self.assertTrue('while attempting a CSR signing transaction' in \
            server_sout)

if __name__ == '__main__':
    sys.stdout = StreamToLogger(logging.getLogger('stdout'), logging.INFO)
    sys.stderr = StreamToLogger(logging.getLogger('stderr'), logging.WARN)
    suite = unittest.TestLoader().loadTestsFromTestCase(TestGeneTorrentCurlVerifyCA)
    result = unittest.TextTestRunner(stream=sys.stderr, verbosity=2).run(suite)
    if not result.wasSuccessful():
        sys.exit(1)

