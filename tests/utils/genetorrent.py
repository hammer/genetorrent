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

from subprocess import Popen, PIPE

class InstanceType:
    GT_ALL = 0
    GT_DOWNLOAD = 1
    GT_UPLOAD = 2
    GT_SERVER = 3

# map to binary name indexed by InstanceType
gtBinaries = [
    'GeneTorrent',
    'gtdownload',    # gtdownload
    'gtupload',      # gtupload
    'gtserver',      # gtserver
]

# map to default args indexed by InstanceType
defaultArgs = [
    '',                       # GT_ALL instance is documented to have
                              # NO default arguments
    ' -vv -l stdout:full -C . ',
    ' -vv -l stdout:full -C . ',
    ' -l server%sgtserver.log:full -C . ' % (os.path.sep),
]

class GeneTorrentInstance:
    '''
    This class spawns and runs a GeneTorrent instance,
    using supplied arguments.  The default instance type, GT_ALL,
    uses the all-in-one binary and supplies NO default arguments.
    '''
    running = False
    returncode = None

    def __init__(self, arguments, instance_type=InstanceType.GT_ALL,
        ssl_no_verify_ca=True):
        self.args = arguments
        self.instance_type = instance_type

        self.args += defaultArgs[instance_type]

        # GT_ALL has NO default arguments
        if ssl_no_verify_ca and self.instance_type != InstanceType.GT_ALL:
            self.args += ' --ssl-no-verify-ca '

        self.start()

    def start(self):
        gt_bin = os.path.join(
            os.getcwd(),
            os.path.pardir,
            'src',
            gtBinaries[self.instance_type],
        )

        command = [gt_bin]
        command.extend(self.args.split(' '))

        print 'GeneTorrentInstance starting: ' + ' '.join(command)
        self.process = Popen(command,
            stderr=PIPE, stdout=PIPE)

    def communicate(self, input=None):
        ret = self.process.communicate(input)
        self.returncode = self.process.returncode
        return ret

    def running(self):
        if self.returncode:
            return False
        return True

    def kill(self):
        self.process.kill()

