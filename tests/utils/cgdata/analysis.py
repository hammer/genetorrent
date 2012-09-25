'''
Template for analysis.xml.

Usage::

    import cgdata.analysis

    bam_info = {
        'alias':        'Description of test data file',
        'title':        'Title description of bam information',
        'bam_md5sum':   '3fd013ebe4b4d80602b520b41ea75f3e',     # md5 checksum of bam file.
        'bam_filename': 'foo-bar.bam',                          # name of bam file.
    }

    with open('analysis.xml', 'wb') as fp:
        fp.write(cgdata.analysis.xml_template % bam_info)
'''

xml_template = '''\
<ANALYSIS_SET xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <ANALYSIS alias="%(alias)s" center_name="CGHUB" broker_name="NCBI" analysis_center="CGHUB" analysis_date="2011-12-07T00:00:00">
    <TITLE>"%(title)s"</TITLE>
    <STUDY_REF accession="SRP000677" refcenter="NHGRI" refname="CGTEST"/>
    <DESCRIPTION>CGHUB Mixed sample for TCGA Benchmark 4: 5 HCC1143BL 95 HCC1143 **ONLY FOR USE IN BENCHMARK EXERCISE**</DESCRIPTION>
    <ANALYSIS_TYPE>
      <REFERENCE_ALIGNMENT>
        <ASSEMBLY>
          <STANDARD short_name="GRCh37_BI_Variant" />
        </ASSEMBLY>
        <RUN_LABELS>
          <RUN data_block_name="D0ENMACXX111207" read_group_label="D0ENM.1" refname="D0ENMACXX111207" refcenter="CGHUB" />
          <RUN data_block_name="D0ENMACXX111207" read_group_label="D0ENM.2" refname="D0ENMACXX111207" refcenter="CGHUB" />
          <RUN data_block_name="D0ENMACXX111207" read_group_label="D0ENM.3" refname="D0ENMACXX111207" refcenter="CGHUB" />
          <RUN data_block_name="D0ENMACXX111207" read_group_label="D0ENM.5" refname="D0ENMACXX111207" refcenter="CGHUB" />
          <RUN data_block_name="D0ENMACXX111207" read_group_label="D0ENM.6" refname="D0ENMACXX111207" refcenter="CGHUB" />
          <RUN data_block_name="D0ENMACXX111207" read_group_label="D0ENM.7" refname="D0ENMACXX111207" refcenter="CGHUB" />
          <RUN data_block_name="D0EN0ACXX111207" read_group_label="D0EN0.4" refname="D0ENMACXX111207" refcenter="CGHUB" />
          <RUN data_block_name="D0EN0ACXX111207" read_group_label="D0EN0.7" refname="D0ENMACXX111207" refcenter="CGHUB" />
          <RUN data_block_name="D0EN0ACXX111207" read_group_label="D0EN0.8" refname="D0ENMACXX111207" refcenter="CGHUB" />
        </RUN_LABELS>
        <SEQ_LABELS>
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="1" seq_label="1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="2" seq_label="2" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="3" seq_label="3" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="4" seq_label="4" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="5" seq_label="5" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="6" seq_label="6" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="7" seq_label="7" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="8" seq_label="8" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="9" seq_label="9" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="10" seq_label="10" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="11" seq_label="11" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="12" seq_label="12" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="13" seq_label="13" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="14" seq_label="14" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="15" seq_label="15" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="16" seq_label="16" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="17" seq_label="17" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="18" seq_label="18" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="19" seq_label="19" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="20" seq_label="20" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="21" seq_label="21" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="22" seq_label="22" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="X" seq_label="X" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="Y" seq_label="Y" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="MT" seq_label="MT" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000207.1" seq_label="GL000207.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000226.1" seq_label="GL000226.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000229.1" seq_label="GL000229.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000231.1" seq_label="GL000231.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000210.1" seq_label="GL000210.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000239.1" seq_label="GL000239.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000235.1" seq_label="GL000235.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000201.1" seq_label="GL000201.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000247.1" seq_label="GL000247.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000245.1" seq_label="GL000245.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000197.1" seq_label="GL000197.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000203.1" seq_label="GL000203.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000246.1" seq_label="GL000246.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000249.1" seq_label="GL000249.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000196.1" seq_label="GL000196.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000248.1" seq_label="GL000248.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000244.1" seq_label="GL000244.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000238.1" seq_label="GL000238.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000202.1" seq_label="GL000202.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000234.1" seq_label="GL000234.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000232.1" seq_label="GL000232.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000206.1" seq_label="GL000206.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000240.1" seq_label="GL000240.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000236.1" seq_label="GL000236.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000241.1" seq_label="GL000241.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000243.1" seq_label="GL000243.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000242.1" seq_label="GL000242.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000230.1" seq_label="GL000230.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000237.1" seq_label="GL000237.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000233.1" seq_label="GL000233.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000204.1" seq_label="GL000204.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000198.1" seq_label="GL000198.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000208.1" seq_label="GL000208.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000191.1" seq_label="GL000191.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000227.1" seq_label="GL000227.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000228.1" seq_label="GL000228.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000214.1" seq_label="GL000214.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000221.1" seq_label="GL000221.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000209.1" seq_label="GL000209.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000218.1" seq_label="GL000218.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000220.1" seq_label="GL000220.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000213.1" seq_label="GL000213.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000211.1" seq_label="GL000211.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000199.1" seq_label="GL000199.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000217.1" seq_label="GL000217.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000216.1" seq_label="GL000216.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000215.1" seq_label="GL000215.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000205.1" seq_label="GL000205.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000219.1" seq_label="GL000219.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000224.1" seq_label="GL000224.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000223.1" seq_label="GL000223.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000195.1" seq_label="GL000195.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000212.1" seq_label="GL000212.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000222.1" seq_label="GL000222.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000200.1" seq_label="GL000200.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000193.1" seq_label="GL000193.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000194.1" seq_label="GL000194.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000225.1" seq_label="GL000225.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="GL000192.1" seq_label="GL000192.1" />
          <SEQUENCE data_block_name="HCC1143_2011_12_07" accession="NC_007605" seq_label="NC_007605" />
        </SEQ_LABELS>
        <PROCESSING>
          <PIPELINE>
            <PIPE_SECTION section_name="Mapping">
              <STEP_INDEX>1</STEP_INDEX>
              <PREV_STEP_INDEX>NIL</PREV_STEP_INDEX>
              <PROGRAM>bwa</PROGRAM>
              <VERSION>0.5.9-r16</VERSION>
              <NOTES>bwa aln reference.fasta -q 5 -l 32 -k 2 -t 4 -o 1 -f fastq.sai input.fastq.gz</NOTES>
            </PIPE_SECTION>
            <PIPE_SECTION section_name="Mapping">
              <STEP_INDEX>2</STEP_INDEX>
              <PREV_STEP_INDEX>1</PREV_STEP_INDEX>
              <PROGRAM>bwa</PROGRAM>
              <VERSION>0.5.9-r16</VERSION>
              <NOTES>bwa sampe -P -f output.sam fastq1.sai fastq2.sai input1.fastq.gz input2.fastq.gz</NOTES>
            </PIPE_SECTION>
            <PIPE_SECTION section_name="Quality Score Recalibration">
              <STEP_INDEX>3</STEP_INDEX>
              <PREV_STEP_INDEX>2</PREV_STEP_INDEX>
              <PROGRAM>GATK TableRecalibration</PROGRAM>
              <VERSION>1.3-14-g59da26a</VERSION>
              <NOTES>default_read_group=null default_platform=null force_read_group=null force_platform=null window_size_nqs=5 homopolymer_nback=7 exception_if_no_tile=false solid_recal_mode=SET_Q_ZERO solid_nocall_strategy=THROW_EXCEPTION recal_file=/seq/picard/D0ENMACXX/C1-210_2011-12-07_2011-12-18/2/Sage-75643/D0ENMACXX.2.recal_data.csv preserve_qscores_less_than=5 smoothing=1 max_quality_score=50 doNotWriteOriginalQuals=false no_pg_tag=false fail_with_no_eof_marker=false skipUQUpdate=false Covariates=[ReadGroupCovariate, QualityScoreCovariate, CycleCovariate, DinucCovariate] 
</NOTES>
            </PIPE_SECTION>
            <PIPE_SECTION section_name="Indel Realignment">
              <STEP_INDEX>4</STEP_INDEX>
              <PREV_STEP_INDEX>3</PREV_STEP_INDEX>
              <PROGRAM>GATK IndelRealigner</PROGRAM>
              <VERSION>1.3-14-g59da26a</VERSION>
              <NOTES>knownAlleles=[(RodBinding name=knownAlleles source=/seq/references/Homo_sapiens_assembly19/v1/Homo_sapiens_assembly19.dbsnp.vcf), (RodBinding name=knownAlleles2 source=/seq/references/Homo_sapiens_assembly19/v1/Homo_sapiens_assembly19.1kg_pilot_indels.vcf)] LODThresholdForCleaning=5.0 out=null bam_compression=null disable_bam_indexing=null generate_md5=null simplifyBAM=null consensusDeterminationModel=USE_SW entropyThreshold=0.15 maxReadsInMemory=1000000 maxIsizeForMovement=3000 maxPositionalMoveAllowed=200 maxConsensuses=30 maxReadsForConsensuses=120 maxReadsForRealignment=20000 noOriginalAlignmentTags=false nWayOut=/seq/picard_aggregation/G15511/HCC1143/v1/final_bams.map generate_nWayOut_md5s=true check_early=false noPGTag=false keepPGTags=false indelsFileForDebugging=null statisticsFileForDebugging=/seq/picard_aggregation/G15511/HCC1143/v1/HCC1143.indel.stats SNPsFileForDebugging=null</NOTES>
            </PIPE_SECTION>
          </PIPELINE>
          <DIRECTIVES>
            <alignment_includes_unaligned_reads>true</alignment_includes_unaligned_reads>
            <alignment_marks_duplicate_reads>true</alignment_marks_duplicate_reads>
            <alignment_includes_failed_reads>false</alignment_includes_failed_reads>
          </DIRECTIVES>
        </PROCESSING>
      </REFERENCE_ALIGNMENT>
    </ANALYSIS_TYPE>
    <TARGETS>
      <TARGET sra_object_type="SAMPLE" refcenter="TCGA" refname="885b8e06-182f-437b-9183-b454643a64ec"/>
    </TARGETS>
    <DATA_BLOCK name="HCC1143_2011_12_07">
      <FILES>
        <FILE checksum="%(bam_md5sum)s" filetype="bam" filename="%(bam_filename)s" checksum_method="MD5"/>
      </FILES>
    </DATA_BLOCK>
  </ANALYSIS>
</ANALYSIS_SET>'''
