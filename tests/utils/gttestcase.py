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
import os
import time
import errno
import logging
import io
import sys

from uuid import uuid4
from shutil import copy2, rmtree
try:
    from hashlib import sha1 as hash_sha1
except ImportError:
    from sha import new as hash_sha1
from tempfile import NamedTemporaryFile
from subprocess import Popen, PIPE

from utils.cgdata.datagen import create_data, write_random_data
from utils.genetorrent import GeneTorrentInstance, InstanceType
from utils.mockhubcontrol import MockHub
from utils.config import TestConfig
from gtoinfo import read_gto, emit_gto, set_key

logger = logging.getLogger()
logger.setLevel(logging.DEBUG)
formatter = logging.Formatter('%(asctime)s - %(name)s - (%(levelname)s) %(message)s',
    '%m/%d/%Y %H:%M:%S')

if TestConfig.VERBOSE:
    sh = logging.StreamHandler()
    sh.setFormatter(formatter)
    logger.addHandler(sh)

fh = logging.FileHandler('gttest.log')
fh.setFormatter(formatter)
logger.addHandler(fh)

class StreamToLogger(object):
    def __init__(self, logger, log_level=logging.INFO):
        self.logger = logger
        self.log_level = log_level

    def flush(self):
        pass

    def write(self, buf):
        for line in buf.rstrip().splitlines():
            self.logger.log(self.log_level, line.rstrip())

class GTTestCase(unittest.TestCase):
    ''' This class is the test harness for GeneTorrent
    unit tests. '''
    create_mockhub = False
    create_credential = False

    def setUp(self):
        logger.info('\n')
        logger.info('=' * 80)
        logger.info('TEST: ' + self._testMethodName)
        if self.shortDescription():
            logger.info(self.shortDescription())
        logger.info('=' * 80 + '\n')

        if self.create_credential and not TestConfig.CREDENTIAL:
            self.make_dummy_credential()
        if TestConfig.CREDENTIAL:
            self.cred_filename = TestConfig.CREDENTIAL
        if self.create_mockhub and TestConfig.MOCKHUB:
            self.mockhub = MockHub()

        try:
            rmtree('client')
            rmtree('server')
        except OSError, e:
            if e.errno != errno.ENOENT:
                raise

        os.makedirs('client')
        os.makedirs('server' + os.path.sep + 'workdir')
        os.makedirs('server' + os.path.sep + 'root')

    def tearDown(self):
        if self.create_credential and not TestConfig.CREDENTIAL:
            os.unlink(self.cred_filename)
        if self.create_mockhub and TestConfig.MOCKHUB:
            self.mockhub.close()
        rmtree('client')
        rmtree('server')

    def client_gto(self, uuid):
        return os.path.join(
            'client',
            str(uuid),
            str(uuid) + '.gto',
        )

    def client_bam(self, uuid):
        return os.path.join(
            'client',
            str(uuid),
            str(uuid) + '.bam',
        )

    def server_gto(self, uuid):
        return os.path.join(
            'server',
            'workdir',
            str(uuid) + '.gto',
        )

    def server_bam(self, uuid):
        return os.path.join(
            'server',
            'root',
            str(uuid),
            str(uuid) + '.bam',
        )

    def make_dummy_credential(self):
        credential = NamedTemporaryFile(delete=False)
        credential.write('DUMMY-CREDENTIAL')
        credential.close()

        self.cred_filename = credential.name

    def compare_hashes(self, file1, file2):
        ''' Compare the SHA1 hashes of two files. Returns True or False. '''
        h1 = hash_sha1()
        h2 = hash_sha1()

        f1 = open(file1, 'r')
        try:
            while True:
                f1_data = f1.read(4096)
                h1.update(f1_data)
                if len(f1_data) < 4096:
                    break
        finally:
            f1.close()

        f2 = open(file2, 'r')
        try:
            while True:
                f2_data = f2.read(4096)
                h2.update(f2_data)
                if len(f2_data) < 4096:
                    break
        finally:
            f2.close()

        return h1.digest() == h2.digest()

    def generate_bam_data(self, uuid, size, data_generator=write_random_data):
        ''' Generate random or zero BAM data using datagen module. '''
        basedir = os.getcwd()
        os.chdir('client')
        create_data(uuid, size, data_generator,
            TestConfig.HUB_SERVER)
        os.chdir(basedir)

    def upload_sleep(self, minutes):
        print 'Waiting %s minutes for remote processing of upload(s)...' % str(minutes)
        while minutes > 0:
            print str(minutes) + \
                (' minutes' if minutes != 1 else ' minute')  + ' remaining'
            time.sleep(60)
            minutes -= 1
        print 'Proceeding...'

    def data_upload_test(self, size, data_generator=write_random_data,
        ssl=True, client_options='', server_options='', check_sha1=True):
        server = None
        uuid = uuid4()

        self.generate_bam_data(uuid, size, data_generator)

        if TestConfig.MOCKHUB:
            # prepare server
            server = GeneTorrentInstance(
                '-s server%sroot -q server%sworkdir -c %s ' \
                '--security-api %s %s' \
                % (
                    os.path.sep,
                    os.path.sep,
                    self.cred_filename,
                    TestConfig.SECURITY_API,
                    server_options
                  ), instance_type=InstanceType.GT_SERVER)
        else:
            # submit via cgsubmit
            cgsubmit_process = Popen(['python', '../cgsubmit', '-s',
                TestConfig.HUB_SERVER, '-c', TestConfig.CREDENTIAL, '-u',
                str(uuid)],
                stdout=PIPE, stderr=PIPE, cwd='client')
            cgout, cgerr = cgsubmit_process.communicate()

            if TestConfig.VERBOSE:
                print 'cgsubmit stdout: ' + cgout
                print 'cgsubmit stderr: ' + cgerr

            self.assertEqual(cgsubmit_process.returncode, 0)

        # prepare upload client
        client_manifest = os.path.join(
            'client',
            str(uuid),
            'manifest-generated.xml',
        )

        # modify manifest for nossl, if necessary
        if not ssl:
            manifest_file = open(client_manifest, 'r+')
            try:
                manifest_text = manifest_file.read()
                manifest_text = manifest_text.replace('/analysis/',
                    '/analysis-nossl/')
                manifest_file.seek(0)
                manifest_file.truncate()
                manifest_file.write(manifest_text)
            finally:
                manifest_file.close()

        client = GeneTorrentInstance('-u %s -p client -c %s %s' \
            % (
                client_manifest,
                self.cred_filename,
                client_options,
            ), instance_type=InstanceType.GT_UPLOAD)

        # add new upload GTO to server work queue
        gto_file = False

        while not gto_file:
            try:
                os.stat(self.client_gto(uuid))
                gto_file = True
            except:
                time.sleep(5)
                pass

        # wait for SSL cert
        time.sleep(20)

        copy2(self.client_gto(uuid), os.path.join(
            'server',
            'workdir',
        ))

        # wait for upload client to exit
        client_sout, client_serr = client.communicate()

        if server:
            server.kill()

        # check upload client return code
        self.assertEqual(client.returncode, 0)

        # check upload client stderr complete message (100.000%)
        self.assertTrue('100.000% complete' in client_serr)

        # check file hashes on both sides of transfer
        if server and check_sha1:
            self.assertTrue(self.compare_hashes(
                self.client_bam(uuid), self.server_bam(uuid)))

        return uuid

    def data_download_test_xml(self, xml_file_name, uuids,
        client_options='', server_options='', check_sha1=True):
        server = None
        # delete the bams so we can download them
        for uuid in uuids:
            if os.path.isfile(self.client_bam(uuid)):
                os.remove(self.client_bam(uuid))

        if TestConfig.MOCKHUB:
            # prepare server
            server = GeneTorrentInstance(
                '-s server%sroot -q server%sworkdir -c %s ' \
                '--security-api %s %s' \
                % (
                    os.path.sep,
                    os.path.sep,
                    self.cred_filename,
                    TestConfig.SECURITY_API,
                    server_options,
                ), instance_type=InstanceType.GT_SERVER)

            # add new download GTO to server work queue
            # add gt_download_mode flag to server gto
            for uuid in uuids:
                gtodict = read_gto(self.client_gto(uuid))
                gtodict['gt_download_mode'] = 'true'
                expiry = time.time() + (1 * 3600 * 24) # expire in 1 day
                set_key(gtodict, 'expires on', int(expiry))
                emit_gto(gtodict, self.server_gto(uuid), True)
        else:
            self.upload_sleep(3)

        # prepare download client
        client = GeneTorrentInstance('-d %s ' \
            '-p client -c %s %s' \
            % (
                xml_file_name,
                self.cred_filename,
                client_options,
            ), instance_type=InstanceType.GT_DOWNLOAD)

        # wait for download client to exit
        client_sout, client_serr = client.communicate()

        if server:
            server.kill()

        # check download client return code
        self.assertEqual(client.returncode, 0)

        self.assertTrue('Downloaded' in client_serr)
        self.assertTrue('state changed to: finished' in client_sout)

        # check file hashes on both sides of transfer
        for uuid in uuids:
            if check_sha1 and TestConfig.MOCKHUB:
                self.assertTrue(self.compare_hashes(
                    self.client_bam(uuid), self.server_bam(uuid)))

        return client.returncode

    def data_download_test_uuid(self, uuid, client_options='', server_options='',
        check_sha1=True, assert_rc=0, assert_serr=''):
        server = None
        # delete the bam so we can download it
        if os.path.isfile(self.client_bam(uuid)):
            os.remove(self.client_bam(uuid))

        if TestConfig.MOCKHUB:
            # prepare server
            server = GeneTorrentInstance(
                '-s server%sroot -q server%sworkdir -c %s ' \
                '--security-api %s %s' \
                % (
                    os.path.sep,
                    os.path.sep,
                    self.cred_filename,
                    TestConfig.SECURITY_API,
                    server_options,
                ), instance_type=InstanceType.GT_SERVER)

            # add new download GTO to server work queue
            # add gt_download_mode flag to server gto
            gtodict = read_gto(self.client_gto(uuid))
            gtodict['gt_download_mode'] = 'true'
            expiry = time.time() + (1 * 3600 * 24) # expire in 1 day
            set_key(gtodict, 'expires on', int(expiry))
            emit_gto(gtodict, self.server_gto(uuid), True)
        else:
            self.upload_sleep(3)

        # prepare download client
        client = GeneTorrentInstance('-d %s/cghub/data/analysis/download/%s ' \
            '-p client %s %s' \
            % (
                TestConfig.HUB_SERVER,
                str(uuid),
                '-c ' + self.cred_filename if not '-c ' in client_options else '',
                client_options,
            ), instance_type=InstanceType.GT_DOWNLOAD)

        # wait for download client to exit
        client_sout, client_serr = client.communicate()

        if server:
            server.kill()

        # check download client return code
        self.assertEqual(client.returncode, assert_rc)

        # caller-passed client_serr contains assert
        if assert_serr:
            self.assertTrue(assert_serr in client_serr)

        # only continue checking if assert_rc == 0
        if assert_rc == 0:
            # check download client
            self.assertTrue('Downloaded' in client_serr)
            self.assertTrue('state changed to: finished' in client_sout)

            # check file hashes on both sides of transfer
            if server and check_sha1:
                self.assertTrue(self.compare_hashes(
                    self.client_bam(uuid), self.server_bam(uuid)))

        return client.returncode

    def data_download_test_gto(self, uuid, client_options='',
        server_options='', check_sha1=True):
        server = None
        # delete the bam so we can download it
        if os.path.isfile(self.client_bam(uuid)):
            os.remove(self.client_bam(uuid))

        if TestConfig.MOCKHUB:
            # prepare server
            server = GeneTorrentInstance(
                '-s server%sroot -q server%sworkdir -c %s ' \
                '--security-api %s %s' \
                % (
                    os.path.sep,
                    os.path.sep,
                    self.cred_filename,
                    TestConfig.SECURITY_API,
                    server_options,
                ), instance_type=InstanceType.GT_SERVER)

            # add new download GTO to server work queue
            # add gt_download_mode flag to server gto
            gtodict = read_gto(self.client_gto(uuid))
            gtodict['gt_download_mode'] = 'true'
            expiry = time.time() + (1 * 3600 * 24) # expire in 1 day
            set_key(gtodict, 'expires on', int(expiry))
            emit_gto(gtodict, self.server_gto(uuid), True)
        else:
            self.upload_sleep(3)

        client = GeneTorrentInstance('-d %s -p client -c %s %s' \
            '--security-api %s' \
            % (
                self.client_gto(uuid),
                self.cred_filename,
                client_options,
                TestConfig.SECURITY_API,
            ), instance_type=InstanceType.GT_DOWNLOAD)

        # wait for download client to exit
        client_sout, client_serr = client.communicate()

        if server:
            server.kill()

        # check download client return code
        self.assertEqual(client.returncode, 0)

        # check download client complete message
        self.assertTrue('Downloaded' in client_serr)
        self.assertTrue('state changed to: finished' in client_sout)

        # check file hashes on both sides of transfer
        if check_sha1 and TestConfig.MOCKHUB:
            self.assertTrue(self.compare_hashes(
                self.client_bam(uuid), self.server_bam(uuid)))

        return client.returncode

    def create_gto_only(self, uuid):
        # prepare upload client
        client_manifest = os.path.join(
            'client',
            str(uuid),
            'manifest-generated.xml',
        )

        client = GeneTorrentInstance('-u %s -p client -c %s ' \
            '--gto-only' \
            % (
                client_manifest,
                self.cred_filename,
            ), instance_type=InstanceType.GT_UPLOAD)

        # wait for upload client to exit
        client_sout, client_serr = client.communicate()

        self.assertEqual(client.returncode, 0)
        self.assertTrue('upload will be skipped' in str.lower(client_sout))

