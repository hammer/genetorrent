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

import httplib
import os
import sys
import errno
import socket
import logging

import subprocess
from time import sleep

from utils.config import TestConfig

log = logging.getLogger(__name__)

class MockHub(object):
    ''' A wrapper class for interaction with MockHub instances. '''
    running = False

    def __init__(self, srcdir=None, stream=None, gto_base_path=None):
        if srcdir is None:
            srcdir = os.getenv('srcdir')
        mockhub_bin = os.path.join(srcdir, 'mockhub.py')

        env = dict(os.environ)
        if gto_base_path:
            env['MOCK_GTO_BASE_PATH'] = gto_base_path

        if stream is None:
            stderr = None
        else:
            stderr = subprocess.STDOUT

        self.process = subprocess.Popen(['python', mockhub_bin], stdout=stream,
                                        stderr=stderr, env=env)

        h = httplib.HTTPSConnection(TestConfig.HUB_HOST,
            int(TestConfig.HUB_PORT), timeout=2)

        retries = 5
        while retries > 0:
            sleep(1)

            try:
                h.request('GET', '/control')
                resp = h.getresponse()
                log.debug ("MockHub HTTP response: " + resp.read())
                log.debug ("MockHub HTTP status code: " + str(resp.status))

                if resp.status == httplib.OK:
                    break
            except httplib.CannotSendRequest:
                pass
            except socket.error, e:
                if e.errno != errno.ECONNREFUSED:
                    raise

            retries -= 1
        else:
            raise Exception("Mockhub never came up")

        self.running = True

    def is_running (self):
        return self.process.poll () is None

    def close(self):

        retries = 5
        while retries > 0:
            sleep(1)
            h = httplib.HTTPSConnection(TestConfig.HUB_HOST,
                int(TestConfig.HUB_PORT), timeout=2)
            try:
                h.request('GET', '/control?exit=1')
            except httplib.CannotSendRequest:
                break
            except socket.error, e:
                if e.errno != errno.ECONNREFUSED:
                    raise
                break

            retries -= 1
        else:
            raise Exception("Mockhub never shut down")

        log.debug ("Waiting for MockHub to exit...")
        self.process.wait()
        self.running = False

