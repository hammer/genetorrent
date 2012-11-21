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

import os
import subprocess
import threading
import time
import tempfile

from logging import getLogger

SERVER_STOP_FILE = '/tmp/GeneTorrent.stop'

class InstanceType:
    GT_ALL = 0       # no longer provided
    GT_DOWNLOAD = 1
    GT_UPLOAD = 2
    GT_SERVER = 3

# map to binary name indexed by InstanceType
gtBinaries = [
    '',              # GeneTorrent all-in-one binary is obsolete
    'gtdownload',    # gtdownload
    'gtupload',      # gtupload
    'gtserver',      # gtserver
]

# map to default args indexed by InstanceType
defaultArgs = [
    '',                       # GT_ALL
    ' -vv -l stdout:full -C . -k 3 -i 30101',
    ' -vv -l stdout:full -C . -k 3 -i 30201',
    ' -l stdout:full -C . --peer-timeout=5 -i 30301',
]

class GeneTorrentInstance(subprocess.Popen):
    '''
    This class spawns and runs a GeneTorrent instance,
    using supplied arguments.  The default instance type is
    GT_DOWNLOAD
    '''

    def log_thread(self, pipe, logger, buffer):
        def log_output(out, logger, buffer):
            for line in iter(out.readline, b''):
                buffer.write(line)
                if not 'block_finished' in line and not 'block_download' in line:
                    logger(line.rstrip('\n'))

        t = threading.Thread(target=log_output, args=(pipe, logger, buffer))
        t.daemon = True
        t.start()
        return t

    def __init__(self, arguments, instance_type=InstanceType.GT_DOWNLOAD,
        ssl_no_verify_ca=True, add_defaults=True):
        self.args = arguments
        self.instance_type = instance_type
        self.stdout_buffer = tempfile.TemporaryFile()
        self.stderr_buffer = tempfile.TemporaryFile()

        if add_defaults:
            self.args += defaultArgs[instance_type]
        self.LOG = getLogger(gtBinaries[self.instance_type])

        if ssl_no_verify_ca and add_defaults:
            self.args += ' --ssl-no-verify-ca '

        gt_bin = os.path.join(
            os.getcwd(),
            os.path.pardir,
            'src',
            gtBinaries[self.instance_type],
        )

        if os.name == 'nt':
            gt_bin = os.path.join(
                os.getcwd(),
                os.path.pardir,
                gtBinaries[self.instance_type],
            )

        command = [gt_bin]
        command.extend(self.args.split())

        if instance_type == InstanceType.GT_SERVER:
            try:
                os.unlink(SERVER_STOP_FILE)
            except OSError:  # file does not exist
                pass

        super(GeneTorrentInstance, self).__init__(command,
            stderr=subprocess.PIPE, stdout=subprocess.PIPE,
            bufsize=1)

        self.LOG.debug('Started GeneTorrent instance, pid %s' % self.pid)
        self.LOG.debug('Command: %s', command)

        self.stdout_thread = self.log_thread(self.stdout, self.LOG.info,
            self.stdout_buffer)
        self.stderr_thread = self.log_thread(self.stderr, self.LOG.warn,
            self.stderr_buffer)

    def running(self):
        if self.returncode:
            return False
        return True

    def communicate(self, input=None):
        # sets return code
        self.wait()

        self.stdout_thread.join()
        self.stderr_thread.join()

        self.stdout_buffer.seek(0)
        self.stderr_buffer.seek(0)

        return (self.stdout_buffer.read(), self.stderr_buffer.read())

