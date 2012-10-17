'''
Template for run.xml file.

Usage::

    import cgdata.run
    from cgdata.datagen import xml_file

    file_entries = [
        {
            'bam_md5sum':   '3fd013ebe4b4d80602b520b41ea75f3e',     # md5 checksum of bam file.
            'bam_filename': 'foo-bar.bam',                          # name of bam file.
        },
    ]
    my_file_entries = [ xml_file % fe for fe in file_entries ]

    bam_info = {
        'file_entries': '\n'.join (my_file_entries),
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
        %(file_entries)s
      </FILES>
    </DATA_BLOCK>
  </RUN>
</RUN_SET>'''
