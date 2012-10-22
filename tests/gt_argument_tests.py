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

''' Unit tests for command line arguments feature '''

import unittest
import sys
import os
import time
import logging
import sys

from utils.gttestcase import GTTestCase, StreamToLogger
from utils.genetorrent import GeneTorrentInstance, InstanceType

class TestGeneTorrentArguments(GTTestCase):
    """
    Test various features related to GeneTorrent's
    command line argument processing
    """
    create_credential = True

    confdir = '-C . '

    def test_short_multiple_modes(self):
        """
        Test multiple mode options to Gene Torrent (-d, -s, -u)
        """
        gt = GeneTorrentInstance(self.confdir + "-d xxx -s %s -u xxx" % (os.getcwd()),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()

        self.assertTrue("may only specify one of" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "-d xxx -s %s" % (os.getcwd()),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("may only specify one of" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "-s %s -u xxx" % (os.getcwd()),
            instance_type=InstanceType.GT_SERVER, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("may only specify one of" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "-d xxx -u xxx",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("may only specify one of" in serr)
        self.assertEqual(gt.returncode, 9)

    def test_long_multiple_modes(self):
        """
        Test multiple mode options to Gene Torrent (-d, -s, -u)
        """
        gt = GeneTorrentInstance(self.confdir + "--download xxx --server %s --upload xxx" % (os.getcwd()),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("may only specify one of" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "--download xxx --server %s" % (os.getcwd()),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("may only specify one of" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "--server %s --upload xxx" % (os.getcwd()),
            instance_type=InstanceType.GT_SERVER, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("may only specify one of" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "--download xxx --upload xxx",
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("may only specify one of" in serr)
        self.assertEqual(gt.returncode, 9)

    def test_short_upload_options(self):
        """
        Test upload mode options to Gene Torrent (-u)
        """
        gt = GeneTorrentInstance(self.confdir + "-u",
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("required parameter is missing in 'upload'" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "-u xxx",
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("manifest file not found" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "-u %s" % (self.cred_filename),
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("include a credential file" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "-u %s -c %s" % (self.cred_filename, self.cred_filename),
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("error attempting to process the file" in serr)
        self.assertEqual(gt.returncode, 97)

        gt = GeneTorrentInstance(self.confdir + "-u %s -c %s -p /does/not/exit" % (self.cred_filename, self.cred_filename),
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("unable to opening directory" in serr)
        self.assertEqual(gt.returncode, 9)

    def test_long_upload_options(self):
        """
        Test upload mode options to Gene Torrent (-u)
        """
        gt = GeneTorrentInstance(self.confdir + "--upload",
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("required parameter is missing in 'upload'" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "--upload xxx",
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("manifest file not found" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "--upload %s" % (self.cred_filename),
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("include a credential file" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "--upload %s --credential-file %s" % (self.cred_filename, self.cred_filename),
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("error attempting to process the file" in serr)
        self.assertEqual(gt.returncode, 97)

        gt = GeneTorrentInstance(self.confdir + "--upload %s --credential-file %s --path /does/not/exit" % (self.cred_filename, self.cred_filename),
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("unable to opening directory" in serr)
        self.assertEqual(gt.returncode, 9)

    def test_short_download_options(self):
        """
        Test download mode options to Gene Torrent (-d)
        """
        gt = GeneTorrentInstance(self.confdir + "-d",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("required parameter is missing in 'download'" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "-d xxx",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("include a credential file" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "-d xxx -c /file/does/not/exist",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("credentials file not found" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "-d xxx -c %s" % (self.cred_filename),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("'xxx' is too short" in serr)
        self.assertEqual(gt.returncode, 201)

        gt = GeneTorrentInstance(self.confdir + "-c %s -p /path/does/not/exist -d xxx" % (self.cred_filename),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("unable to opening directory" in serr)
        self.assertEqual(gt.returncode, 9)

    def test_long_download_options(self):
        """
        Test download mode options to Gene Torrent (-d)
        """
        gt = GeneTorrentInstance(self.confdir + "--download",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("required parameter is missing in 'download'" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "--download xxx",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("include a credential file" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "--download xxx -c /file/does/not/exist",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("credentials file not found" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "--download xxx --credential-file %s" % (self.cred_filename),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("'xxx' is too short" in serr)
        self.assertEqual(gt.returncode, 201)

        gt = GeneTorrentInstance(self.confdir + "--credential-file %s --path /path/does/not/exist --download xxx" % (self.cred_filename),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("unable to opening directory" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "--credential-file %s --download xxx --security-api=aaa" % (self.cred_filename),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("Invalid URI for '--security-api" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "--download xxx --max-children=0",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("must be greater than 0" in serr)
        self.assertEqual(gt.returncode, 9)

    def test_usage_and_invalid_options(self):
        """
        Test usage and invalid options for GeneTorrent
        """
        gt = GeneTorrentInstance(self.confdir + "--help",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("Usage" in sout and "man gtdownload" in sout)
        self.assertEqual(gt.returncode, 0)

        gt = GeneTorrentInstance(self.confdir + "--this-option-does-not-exist",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("unknown option this-option-does-not-exist" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "-z",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("unknown option -z" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.confdir + "-c foo --credential-file=bar",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("multiple occurrences" in serr)
        self.assertEqual(gt.returncode, 9)

if __name__ == '__main__':
    sys.stdout = StreamToLogger(logging.getLogger('stdout'), logging.INFO)
    sys.stderr = StreamToLogger(logging.getLogger('stderr'), logging.WARN)
    suite = unittest.TestLoader().loadTestsFromTestCase(TestGeneTorrentArguments)
    result = unittest.TextTestRunner(stream=sys.stderr, verbosity=2).run(suite)
    if not result.wasSuccessful():
        sys.exit(1)

