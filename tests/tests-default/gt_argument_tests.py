#!/usr/bin/env python2.7
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

    resourcedir = '-R . '

    def test_short_multiple_modes(self):
        """
        Test multiple mode options to Gene Torrent (-d, -s, -u)
        """
        gt = GeneTorrentInstance(self.resourcedir + "-d xxx -s %s -u xxx" % (os.getcwd()),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()

        self.assertTrue("unknown option" in serr or "unrecognised option" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "-d xxx -s %s" % (os.getcwd()),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("unknown option" in serr or "unrecognised option" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "-s '%s' -u xxx" % (os.getcwd()),
            instance_type=InstanceType.GT_SERVER, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("unknown option" in serr or "unrecognised option" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "-d xxx -u xxx",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("unknown option" in serr or "unrecognised option" in serr)
        self.assertEqual(gt.returncode, 9)

    def test_long_multiple_modes(self):
        """
        Test multiple mode options to Gene Torrent (-d, -s, -u)
        """
        gt = GeneTorrentInstance(self.resourcedir + "--download xxx --server %s --upload xxx -c %s" % (os.getcwd(), self.cred_filename),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("unknown option" in serr or "unrecognised option" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "--download xxx --server %s -c %s" % (os.getcwd(), self.cred_filename),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("too short", serr)
        self.assertEqual(gt.returncode, 201)

        gt = GeneTorrentInstance(self.resourcedir + "--server %s -q %s --download xxx -c %s" % (os.getcwd(), os.getcwd(), self.cred_filename),
            instance_type=InstanceType.GT_SERVER, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("unknown option" in serr or "unrecognised option" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "--download xxx --upload xxx",
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertTrue("unknown option" in serr or "unrecognised option" in serr)
        self.assertEqual(gt.returncode, 9)

    def test_short_upload_options(self):
        """
        Test upload mode options to Gene Torrent (-u)
        """
        gt = GeneTorrentInstance(self.resourcedir + "-u",
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("required", serr)
        self.assertIn("missing", serr)
        self.assertIn("upload", serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "-u xxx",
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("manifest file not found", serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "-u %s" % (self.cred_filename),
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("include a credential file", serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "-u %s -c %s" % (self.cred_filename, self.cred_filename),
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("error attempting to process the file", serr)
        self.assertEqual(gt.returncode, 97)

        gt = GeneTorrentInstance(self.resourcedir + "-u %s -c %s -p /does/not/exit" % (self.cred_filename, self.cred_filename),
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("unable to opening directory", serr)
        self.assertEqual(gt.returncode, 9)

    def test_long_upload_options(self):
        """
        Test upload mode options to Gene Torrent (-u)
        """
        gt = GeneTorrentInstance(self.resourcedir + "--upload",
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("required", serr)
        self.assertIn("missing", serr)
        self.assertIn("upload", serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "--upload xxx",
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("manifest file not found", serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "--upload %s" % (self.cred_filename),
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("include a credential file", serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "--upload %s --credential-file %s" % (self.cred_filename, self.cred_filename),
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("error attempting to process the file", serr)
        self.assertEqual(gt.returncode, 97)

        gt = GeneTorrentInstance(self.resourcedir + "--upload %s --credential-file %s --path /does/not/exit" % (self.cred_filename, self.cred_filename),
            instance_type=InstanceType.GT_UPLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("unable to opening directory", serr)
        self.assertEqual(gt.returncode, 9)

    def test_short_download_options(self):
        """
        Test download mode options to Gene Torrent (-d)
        """
        gt = GeneTorrentInstance(self.resourcedir + "-d",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("required", serr)
        self.assertIn("missing", serr)
        self.assertIn("download", serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "-d xxx",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("include a credential file", serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "-d xxx -c /file/does/not/exist",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("credentials file not found", serr)
        self.assertEqual(gt.returncode, 55)

        gt = GeneTorrentInstance(self.resourcedir + "-d xxx -c %s" % (self.cred_filename),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("'xxx' is too short", serr)
        self.assertEqual(gt.returncode, 201)

        gt = GeneTorrentInstance(self.resourcedir + "-c %s -p /path/does/not/exist -d xxx" % (self.cred_filename),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("unable to opening directory", serr)
        self.assertEqual(gt.returncode, 9)

    def test_long_download_options(self):
        """
        Test download mode options to Gene Torrent (-d)
        """
        gt = GeneTorrentInstance(self.resourcedir + "--download",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("required", serr)
        self.assertIn("missing", serr)
        self.assertIn("download", serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "--download xxx",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("include a credential file", serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "--download xxx -c /file/does/not/exist",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("credentials file not found", serr)
        self.assertEqual(gt.returncode, 55)

        gt = GeneTorrentInstance(self.resourcedir + "--download xxx --credential-file %s" % (self.cred_filename),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("'xxx' is too short", serr)
        self.assertEqual(gt.returncode, 201)

        gt = GeneTorrentInstance(self.resourcedir + "--credential-file %s --path /path/does/not/exist --download xxx" % (self.cred_filename),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("unable to opening directory", serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "--credential-file %s --download xxx --security-api=aaa" % (self.cred_filename),
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("Invalid URI for '--security-api", serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "--download xxx --max-children=0",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("must be greater than 0", serr)
        self.assertEqual(gt.returncode, 9)

    def test_usage_and_invalid_options(self):
        """
        Test usage and invalid options for GeneTorrent
        """
        gt = GeneTorrentInstance(self.resourcedir + "--help",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("Usage", sout)
        self.assertEqual(gt.returncode, 0)

        gt = GeneTorrentInstance(self.resourcedir + "--this-option-does-not-exist",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("this-option-does-not-exist", serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "-z",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        self.assertIn("option", serr)
        self.assertIn("-z", serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance(self.resourcedir + "-c foo --credential-file=bar",
            instance_type=InstanceType.GT_DOWNLOAD, add_defaults=False)
        (sout, serr) = gt.communicate()
        result = "multiple occurrences" in serr or "specified more than once" \
           in serr
        self.assertTrue(result)
        self.assertEqual(gt.returncode, 9)

if __name__ == '__main__':
    sys.stdout = StreamToLogger(logging.getLogger('stdout'), logging.INFO)
    sys.stderr = StreamToLogger(logging.getLogger('stderr'), logging.WARN)
    suite = unittest.TestLoader().loadTestsFromTestCase(TestGeneTorrentArguments)
    result = unittest.TextTestRunner(stream=sys.stderr, verbosity=2).run(suite)
    if not result.wasSuccessful():
        sys.exit(1)

