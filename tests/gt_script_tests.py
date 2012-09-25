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

''' Unit tests for gtoinfo and gtocheck. '''

import unittest
import time
import os

from uuid import uuid4
from shutil import copy2
from subprocess import Popen, PIPE

from utils.gttestcase import GTTestCase
from utils.genetorrent  import GeneTorrentInstance

class TestGtoScripts(GTTestCase):
    create_mockhub = True
    create_credential = True

    gtoinfo_bin = os.path.join(
        str(os.getenv('srcdir')),
        os.path.pardir,
        'scripts',
        'gtoinfo.py',
    )

    gtocheck_bin = os.path.join(
        os.path.pardir,
        'scripts',
        'gtocheck',
    )

    def run_gtoinfo(self, args='', returncode=0):
        arg_list = ['python', self.gtoinfo_bin]

        if args:
            arg_list.extend(args.split(' '))

        self.process = Popen(arg_list,
            stdout=PIPE)

        stdout, stderr = self.process.communicate()

        if self.process.returncode != returncode:
            raise Exception('gtoinfo failed unexpectedly')

        return stdout

    def run_gtocheck(self, args='', returncode=0):
        arg_list = ['python', self.gtocheck_bin]

        if args:
            arg_list.extend(args.split(' '))

        self.process = Popen(arg_list,
            stdout=PIPE)

        stdout, stderr = self.process.communicate()

        if self.process.returncode != returncode:
            raise Exception('gtocheck failed unexpectedly')

        return stdout

    def test_usage_gtoinfo(self):
        stdout = self.run_gtoinfo('', returncode=1)
        self.assertTrue('usage:' in str.lower(stdout))
        self.assertTrue('manipulate a gto file' in str.lower(stdout))

        stdout = self.run_gtoinfo('--help')
        self.assertTrue('usage:' in str.lower(stdout))
        self.assertTrue('manipulate a gto file' in str.lower(stdout))

    def test_usage_gtocheck(self):
        stdout = self.run_gtocheck('', returncode=1)
        self.assertTrue('usage:' in str.lower(stdout))
        self.assertTrue('data files match a gto file' in str.lower(stdout))

        stdout = self.run_gtocheck('--help')
        self.assertTrue('usage:' in str.lower(stdout))
        self.assertTrue('data files match a gto file' in str.lower(stdout))

    def test_gto_info(self):
        uuid = uuid4()

        self.generate_bam_data(uuid, 1024 * 1024 * 1)

        self.create_gto_only(uuid)

        client_gto = self.client_gto(uuid)

        # test behavior with no argument and -d argument
        # should be the same
        single_gto_arg_output = self.run_gtoinfo(client_gto)

        decode_arg_output = self.run_gtoinfo('-d ' + client_gto)

        self.assertEqual(single_gto_arg_output, decode_arg_output,
            'gtoinfo file.gto output does not match gtoinfo -d file.gto output')
        self.assertTrue('announce' in decode_arg_output)
        self.assertTrue('created by' in decode_arg_output)
        self.assertTrue('creation date' in decode_arg_output)
        self.assertTrue('info' in decode_arg_output)
        self.assertTrue('files' in decode_arg_output)
        self.assertTrue('length' in decode_arg_output)
        self.assertTrue('path' in decode_arg_output)
        self.assertTrue('name' in decode_arg_output)
        self.assertTrue('piece length' in decode_arg_output)
        self.assertTrue('pieces' in decode_arg_output)
        self.assertTrue('ssl-cert' in decode_arg_output)
        self.assertTrue(str(uuid) in decode_arg_output)
        self.assertTrue('-----BEGIN CERTIFICATE-----' in decode_arg_output)
        self.assertTrue('-----END CERTIFICATE-----' in decode_arg_output)

        # test changing announce URL

        stdout = self.run_gtoinfo('-a NEW_ANNOUNCE_URL ' + client_gto)
        stdout = self.run_gtoinfo(client_gto)
        self.assertTrue('NEW_ANNOUNCE_URL' in stdout)

        # test --setkey, --getkey
        stdout = self.run_gtoinfo('--set-key=announce NEWER_ANNOUNCE_URL ' + client_gto)
        stdout = self.run_gtoinfo('--get-key=announce ' + client_gto)
        self.assertTrue('NEWER_ANNOUNCE_URL' in stdout)
        stdout = self.run_gtoinfo('-g announce ' + client_gto)
        self.assertTrue('NEWER_ANNOUNCE_URL' in stdout)

    def test_gto_check(self):
        uuid = uuid4()

        self.generate_bam_data(uuid, 1024 * 1024 * 18)

        self.create_gto_only(uuid)

        client_gto = self.client_gto(uuid)

        # test gtocheck
        stdout = self.run_gtocheck('-d client ' + client_gto)
        self.assertTrue(stdout == '')

        # corrupt the BAM file
        client_bam = client_gto.replace('.gto','.bam')
        copy2(client_bam, client_bam + '.backup')

        bam_file = open(client_bam, 'wb')
        try:
            bam_file.write('garbage')
        finally:
            bam_file.close()

        stdout = self.run_gtocheck('-d client ' + client_gto, returncode=1)
        self.assertTrue('expected hash' in str.lower(stdout))
        self.assertTrue('is corrupt' in str.lower(stdout))

        # restore the bam
        copy2(client_bam + '.backup', client_bam)

        os.remove(client_bam + '.backup')

if __name__ == '__main__':
        unittest.main()

