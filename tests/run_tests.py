#!/usr/bin/env python2.7

import sys
import logging
import os

from utils.gttestcase import StreamToLogger

if sys.version_info < (2, 7):
    import unittest2 as unittest
else:
    import unittest

top_srcdir = os.getenv('top_srcdir', '')
test_top = os.path.realpath(os.path.join(top_srcdir, 'tests')) if top_srcdir else None

sys.stdout = StreamToLogger(logging.getLogger('stdout'), logging.INFO)
sys.stderr = StreamToLogger(logging.getLogger('stderr'), logging.WARN)

suite = unittest.TestLoader().discover('tests-default', pattern='*.py',
    top_level_dir=test_top)
result = unittest.TextTestRunner(stream=sys.stderr, verbosity=2).run(suite)

print 'Test run completed: status = %s' % result

if not result.wasSuccessful():
    sys.exit(1)

