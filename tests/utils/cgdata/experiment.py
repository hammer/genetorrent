'''
Template data for experiment.xml file.

Just write this out to a file, no substitutions are done.

Usage::

    import cgdata.analysis

    with open('experiment.xml', 'wb') as fp:
        fp.write(cgdata.experiment.xml_template)
'''

xml_template = '''\
<EXPERIMENT_SET xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <EXPERIMENT alias="HCC1143.mix1.n5t95" center_name="CGHUB">
    <TITLE>UCSC ARTIFICIAL MIXED SAMPLE: 5 HCC1143BL 95 HCC1143</TITLE>
    <STUDY_REF refcenter="NHGRI" refname="CGTEST"/>
    <DESIGN>
      <DESIGN_DESCRIPTION>Mixed sample for TCGA Benchmark 4: 5 HCC1143BL 95 HCC1143 **ONLY FOR USE IN BENCHMARK EXERCISE**</DESIGN_DESCRIPTION>
      <SAMPLE_DESCRIPTOR refcenter="TCGA" refname="2e66eb54-239b-470d-b1b0-c5ba07fb1902"/>
      <LIBRARY_DESCRIPTOR>
        <LIBRARY_NAME>Sage-75643</LIBRARY_NAME>
        <LIBRARY_STRATEGY>WGS</LIBRARY_STRATEGY>
        <LIBRARY_SOURCE>GENOMIC</LIBRARY_SOURCE>
        <LIBRARY_SELECTION>RANDOM</LIBRARY_SELECTION>
        <LIBRARY_LAYOUT>
          <PAIRED NOMINAL_LENGTH="200" NOMINAL_SDEV="20.0" />
        </LIBRARY_LAYOUT>
      </LIBRARY_DESCRIPTOR>
      <SPOT_DESCRIPTOR>
        <SPOT_DECODE_SPEC>
          <NUMBER_OF_READS_PER_SPOT>2</NUMBER_OF_READS_PER_SPOT>
          <READ_SPEC>
            <READ_INDEX>0</READ_INDEX>
            <READ_CLASS>Application Read</READ_CLASS>
            <READ_TYPE>Forward</READ_TYPE>
            <BASE_COORD>1</BASE_COORD>
          </READ_SPEC>
          <READ_SPEC>
            <READ_INDEX>1</READ_INDEX>
            <READ_CLASS>Application Read</READ_CLASS>
            <READ_TYPE>Reverse</READ_TYPE>
            <BASE_COORD>101</BASE_COORD>
          </READ_SPEC>
        </SPOT_DECODE_SPEC>
      </SPOT_DESCRIPTOR>
    </DESIGN>
    <PLATFORM>
      <ILLUMINA>
        <INSTRUMENT_MODEL>Illumina HiSeq 2000</INSTRUMENT_MODEL>
        <SEQUENCE_LENGTH>200</SEQUENCE_LENGTH>
      </ILLUMINA>
    </PLATFORM>
    <PROCESSING>
      <PIPELINE>
        <PIPE_SECTION section_name="Base Caller">
          <STEP_INDEX>1</STEP_INDEX>
          <PREV_STEP_INDEX>NIL</PREV_STEP_INDEX>
          <PROGRAM>Casava</PROGRAM>
          <VERSION>V1.7</VERSION>
          <NOTES />
        </PIPE_SECTION>
        <PIPE_SECTION section_name="Quality Scores">
          <STEP_INDEX>2</STEP_INDEX>
          <PREV_STEP_INDEX>1</PREV_STEP_INDEX>
          <PROGRAM>Casava</PROGRAM>
          <VERSION>V1.7</VERSION>
          <NOTES />
        </PIPE_SECTION>
      </PIPELINE>
    </PROCESSING>
  </EXPERIMENT>
</EXPERIMENT_SET>'''
