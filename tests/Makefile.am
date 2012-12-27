test_scripts = tests-default/gt_argument_tests.py       \
               tests-default/gt_download_tests.py       \
               tests-default/gt_inactivity_tests.py     \
               tests-default/gt_curl_ssl_verify_tests.py\
               tests-default/gt_script_tests.py         \
               tests-default/gt_cred_as_uri_tests.py

extra_test_scripts = tests-extra/gt_20gb_download_test.py   \
                     tests-extra/gt_upload_tests.py         \
                     tests-extra/gt_download_extra_tests.py

TESTS = run_tests.py
TESTS_ENVIRONMENT = PYTHONPATH="$(top_builddir)/tests:$(top_builddir)/scripts" PATH="$(PATH):$(PWD)"

EXTRA_DIST = $(test_scripts)              \
             README                       \
             mockhub.py                   \
             openssl-ca.cnf               \
             web.py-0.37.tar.gz           \
             utils/__init__.py            \
             utils/genetorrent.py         \
             utils/gttestcase.py          \
             utils/mockhubcontrol.py      \
             utils/cgdata/__init__.py     \
             utils/cgdata/analysis.py     \
             utils/cgdata/experiment.py   \
             utils/cgdata/datagen.py      \
             utils/cgdata/manifest.py     \
             utils/cgdata/run.py          \
             utils/config.py              \
             unittest2/case.py            \
             unittest2/collector.py       \
             unittest2/compatibility.py   \
             unittest2/__init__.py        \
             unittest2/loader.py          \
             unittest2/__main__.py        \
             unittest2/main.py            \
             unittest2/result.py          \
             unittest2/runner.py          \
             unittest2/signals.py         \
             unittest2/suite.py           \
             unittest2/util.py            \
             cgsubmit                     \
             run_tests.py                 \
             tests-default/__init__.py    \
             tests-extra/__init__.py      \
             $(test_scripts)              \
             $(extra_test_scripts)

certs:
	$(MKDIR_P) certs/
	@echo "Generating SSL CA certificate..."
	openssl req -new -x509 -days 3650 -extensions v3_ca \
		-nodes -config $(srcdir)/openssl-ca.cnf \
		-keyout certs/cakey.pem -out certs/cacert.pem

web:
	@echo "Unpacking web.py..."
	tar xzf $(srcdir)/web.py-0.37.tar.gz
	$(LN_S) web.py-0.37/web web

gtoinfo:
	$(LN_S) $(srcdir)/../scripts/gtoinfo.py.in gtoinfo

dhparam.pem:
	$(LN_S) $(srcdir)/../src/dhparam.pem dhparam.pem

cgsubmit:
	$(LN_S) $(srcdir)/../scripts/cgsubmit cgsubmit

.PHONY: check-setup
check-setup: web certs gtoinfo dhparam.pem cgsubmit

check-am: check-setup

CLEANFILES = dhparam.pem gtoinfo web cgsubmit

DISTCLEANFILES = gttest.log

clean-local:
	rm -rf web.py-*/
	rm -rf certs/

