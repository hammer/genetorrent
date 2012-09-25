'''
Template data for manifest.xml file.

Usage::

    import cgdata.manifest

    bam_info = {
        'alias':        'Description of test data file',
        'title':        'Title description of bam information',
        'bam_md5sum':   '3fd013ebe4b4d80602b520b41ea75f3e',     # md5 checksum of bam file.
        'bam_filename': 'foo-bar.bam',                          # name of bam file.
        'server':       'https://cgserver-foo.annailabs.com',   # URI of upload server
    }

    with open('manifest.xml', 'wb') as fp:
        fp.write(cgdata.manifest.xml_template % bam_info)
'''

xml_template = '''\
<?xml version="1.0" encoding="utf-8"?>
<SUBMISSION alias="Random test data: 20120827-18:05:14" center_name="CGHUB" created_by="cgsubmit 3.1.0" submission_date="2012-08-27T19:14:01.063872" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="GeneTorrentManifest.xsd">
  <SERVER_INFO server_path="%(uuid)s" submission_uri="%(server)s/cghub/data/analysis/upload/%(uuid)s"/>
  <FILES>
    <FILE checksum="%(bam_md5sum)s" checksum_method="MD5" filename="%(bam_filename)s" filetype="bam"/>
  </FILES>
</SUBMISSION>'''
