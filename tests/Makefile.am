test_scripts = gt_argument_tests.py       \
               gt_download_tests.py       \
               gt_inactivity_tests.py     \
               gt_curl_ssl_verify_tests.py\
               gt_script_tests.py         \
               gt_cred_as_uri_tests.py

extra_test_scripts = gt_20gb_download_test.py   \
                     gt_upload_tests.py         \
                     gt_download_extra_tests.py

TESTS = $(test_scripts)
TESTS_ENVIRONMENT = PYTHONPATH="$(top_srcdir)/scripts:." PATH="$(PATH):$(PWD)"

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
             cgsubmit                     \
             $(extra_test_scripts)

# FIXME: linking with LN_S may not work on Windows

certs:
	mkdir -p certs/
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

all: web certs gtoinfo dhparam.pem

CLEANFILES = dhparam.pem gtoinfo web

clean-local:
	rm -rf web.py-*/
	rm -rf certs/

