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

import unittest
import sys
import os
import time

from utils.gttestcase import GTTestCase
from utils.genetorrent import GeneTorrentInstance

class TestGeneTorrentArguments(GTTestCase):
    """
    Test various features related to GeneTorrent's
    command line argument processing
    """
    create_credential = True

    def test_short_multiple_modes(self):
        """
        Test multiple mode options to Gene Torrent (-d, -s, -u)
        """
        gt = GeneTorrentInstance("-d xxx -s %s -u xxx" % (os.getcwd()))
        (sout, serr) = gt.communicate()
        self.assertTrue("may only specify one of" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("-d xxx -s %s" % (os.getcwd()))
        (sout, serr) = gt.communicate()
        self.assertTrue("may only specify one of" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("-s %s -u xxx" % (os.getcwd()))
        (sout, serr) = gt.communicate()
        self.assertTrue("may only specify one of" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("-d xxx -u xxx")
        (sout, serr) = gt.communicate()
        self.assertTrue("may only specify one of" in serr)
        self.assertEqual(gt.returncode, 9)

    def test_long_multiple_modes(self):
        """
        Test multiple mode options to Gene Torrent (-d, -s, -u)
        """
        gt = GeneTorrentInstance("--download xxx --server %s --upload xxx" % (os.getcwd()))
        (sout, serr) = gt.communicate()
        self.assertTrue("may only specify one of" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("--download xxx --server %s" % (os.getcwd()))
        (sout, serr) = gt.communicate()
        self.assertTrue("may only specify one of" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("--server %s --upload xxx" % (os.getcwd()))
        (sout, serr) = gt.communicate()
        self.assertTrue("may only specify one of" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("--download xxx --upload xxx")
        (sout, serr) = gt.communicate()
        self.assertTrue("may only specify one of" in serr)
        self.assertEqual(gt.returncode, 9)

    def test_short_upload_options(self):
        """
        Test upload mode options to Gene Torrent (-u)
        """
        gt = GeneTorrentInstance("-u")
        (sout, serr) = gt.communicate()
        self.assertTrue("required parameter is missing in 'upload'" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("-u xxx")
        (sout, serr) = gt.communicate()
        self.assertTrue("manifest file not found" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("-u %s" % (self.cred_filename))
        (sout, serr) = gt.communicate()
        self.assertTrue("include a credential file" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("-u %s -c %s" % (self.cred_filename, self.cred_filename))
        (sout, serr) = gt.communicate()
        self.assertTrue("error attempting to process the file" in serr)
        self.assertEqual(gt.returncode, 97)

        gt = GeneTorrentInstance("-u %s -c %s -p /does/not/exit" % (self.cred_filename, self.cred_filename))
        (sout, serr) = gt.communicate()
        self.assertTrue("unable to opening directory" in serr)
        self.assertEqual(gt.returncode, 9)

    def test_long_upload_options(self):
        """
        Test upload mode options to Gene Torrent (-u)
        """
        gt = GeneTorrentInstance("--upload")
        (sout, serr) = gt.communicate()
        self.assertTrue("required parameter is missing in 'upload'" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("--upload xxx")
        (sout, serr) = gt.communicate()
        self.assertTrue("manifest file not found" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("--upload %s" % (self.cred_filename))
        (sout, serr) = gt.communicate()
        self.assertTrue("include a credential file" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("--upload %s --credential-file %s" % (self.cred_filename, self.cred_filename))
        (sout, serr) = gt.communicate()
        self.assertTrue("error attempting to process the file" in serr)
        self.assertEqual(gt.returncode, 97)

        gt = GeneTorrentInstance("--upload %s --credential-file %s --path /does/not/exit" % (self.cred_filename, self.cred_filename))
        (sout, serr) = gt.communicate()
        self.assertTrue("unable to opening directory" in serr)
        self.assertEqual(gt.returncode, 9)

    def test_short_download_options(self):
        """
        Test download mode options to Gene Torrent (-d)
        """
        gt = GeneTorrentInstance("-d")
        (sout, serr) = gt.communicate()
        self.assertTrue("required parameter is missing in 'download'" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("-d xxx")
        (sout, serr) = gt.communicate()
        self.assertTrue("include a credential file" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("-d xxx -c /file/does/not/exist")
        (sout, serr) = gt.communicate()
        self.assertTrue("credentials file not found" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("-d xxx -c %s" % (self.cred_filename))
        (sout, serr) = gt.communicate()
        self.assertTrue("'xxx' is too short" in serr)
        self.assertEqual(gt.returncode, 201)

        gt = GeneTorrentInstance("-c %s -p /path/does/not/exist -d xxx" % (self.cred_filename))
        (sout, serr) = gt.communicate()
        self.assertTrue("unable to opening directory" in serr)
        self.assertEqual(gt.returncode, 9)

    def test_long_download_options(self):
        """
        Test download mode options to Gene Torrent (-d)
        """
        gt = GeneTorrentInstance("--download")
        (sout, serr) = gt.communicate()
        self.assertTrue("required parameter is missing in 'download'" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("--download xxx")
        (sout, serr) = gt.communicate()
        self.assertTrue("include a credential file" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("--download xxx -c /file/does/not/exist")
        (sout, serr) = gt.communicate()
        self.assertTrue("credentials file not found" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("--download xxx --credential-file %s" % (self.cred_filename))
        (sout, serr) = gt.communicate()
        self.assertTrue("'xxx' is too short" in serr)
        self.assertEqual(gt.returncode, 201)

        gt = GeneTorrentInstance("--credential-file %s --path /path/does/not/exist --download xxx" % (self.cred_filename))
        (sout, serr) = gt.communicate()
        self.assertTrue("unable to opening directory" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("--credential-file %s --download xxx --security-api=aaa" % (self.cred_filename))
        (sout, serr) = gt.communicate()
        self.assertTrue("Invalid URI for '--security-api" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("--download xxx --max-children=0")
        (sout, serr) = gt.communicate()
        self.assertTrue("must be greater than 0" in serr)
        self.assertEqual(gt.returncode, 9)

    def test_usage_and_invalid_options(self):
        """
        Test usage and invalid options for GeneTorrent
        """
        gt = GeneTorrentInstance("--help")
        (sout, serr) = gt.communicate()
        self.assertTrue("Usage" in sout and "man GeneTorrent" in sout)
        self.assertEqual(gt.returncode, 0)

        gt = GeneTorrentInstance("--this-option-does-not-exist")
        (sout, serr) = gt.communicate()
        self.assertTrue("unknown option this-option-does-not-exist" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("-z")
        (sout, serr) = gt.communicate()
        self.assertTrue("unknown option -z" in serr)
        self.assertEqual(gt.returncode, 9)

        gt = GeneTorrentInstance("-c foo --credential-file=bar")
        (sout, serr) = gt.communicate()
        self.assertTrue("multiple occurrences" in serr)
        self.assertEqual(gt.returncode, 9)

if __name__ == '__main__':
    unittest.main()

