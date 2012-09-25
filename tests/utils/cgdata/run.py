'''
Template for run.xml file.

Usage::

    import cgdata.run

    bam_info = {
        'bam_md5sum': '3fd013ebe4b4d80602b520b41ea75f3e',
        'bam_filename': '40-byte-bam.bam',
    }

    with open('run.xml', 'wb') as fp:
        fp.write(cgdata.run.xml_template % bam_info)

'''

xml_template = '''\
<RUN_SET xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <RUN alias="D0ENMACXX111207" center_name="CGHUB">
    <EXPERIMENT_REF refname="HCC1143.mix1.n5t95" refcenter="CGHUB" />
    <DATA_BLOCK name="D0ENMACXX111207">
      <FILES>
        <FILE checksum="%(bam_md5sum)s" checksum_method="MD5" filetype="bam" filename="%(bam_filename)s"/>
      </FILES>
    </DATA_BLOCK>
  </RUN>
</RUN_SET>'''
