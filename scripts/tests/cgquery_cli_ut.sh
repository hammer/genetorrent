#!/bin/sh
#set -x

# ----------------------------------------------------------------------
# Script for command-line based cgquery unit testing
# Note that these test cases are 
# ----------------------------------------------------------------------

if [ $# -ge 2 ]
then
    echo "Usage: $0 [cgquery-bin]"
    exit 1
fi

MYNAME=`readlink -f $0`
MYDIR=`dirname $MYNAME`
SRCDIR=`dirname $MYDIR`

if [ $# -ge 1 ] 
then
    CGQUERY="$1"
else
    CGQUERY="$SRCDIR/cgquery"
fi

if [ ! -f $CGQUERY ] 
then
    echo "$CGQUERY does not exist or is not a regular file"
    exit 1
else
    echo "Performing tests using $CGQUERY"
fi

#
# Is python installed?
#
python --version > /dev/null 2>&1
if [ $? -ne 0 ]
then
    echo "Python does not appear to be installed"
    exit 1
fi

#
# Is cgquery executable (it may not be in the svn tree)
#
if [ ! -x $CGQUERY ]
then
    CGQUERY="python ${CGQUERY}"
fi

#
# Do we have the coverage tool available?
#
COV_ERASE=""
COV_REPORT=""
coverage help > /dev/null 2>&1
if [ $? -eq 0 ]
then
    COV_ERASE="coverage erase"
    CGQUERY="coverage run -a ${CGQUERY}"
    COV_REPORT="coverage report -m"
fi

RTDIR="/tmp/_rt$$"
CREDFILE="$RTDIR/rtCred"
OUTFILE="$RTDIR/rtOut"
INFILE=${MYDIR}/testdata/cgquery_in.xml
CONFDIR="$RTDIR/rtConf"
GTBIN=${MYDIR}/gtstub
TESTSVR=
VALIDUUID=
SVRPID=

trap onexit 0 1 2 3 15

if [ -d $RTDIR ]
then
    rm -rf $RTDIR
fi
mkdir -p $RTDIR
touch $CREDFILE
mkdir $CONFDIR
chmod 755 $GTBIN

port=$RANDOM
while [ $port -le 1024 ]
do
    port=$RANDOM
done

$MYDIR/testwsisvr.py $port > testsvr.log 2>&1 &
if [ $? -ne 0 ]
then
    echo "Failed to start WSI stub server on port $port"
    exit 1
fi
SVRPID=$!
TESTSVR=http://127.0.0.1:${port}

verstr=`$CGQUERY --version | awk '{print $3}'`
major=`echo $verstr | awk -F '.' '{print $1}'`
minor=`echo $verstr | awk -F '.' '{print $2}'`


#----------------------------------------------------------------------
# Cleanup to do on exit
#----------------------------------------------------------------------
onexit() {
    if [ ! -z "$SVRPID" ]
    then
        kill $SVRPID
    fi

    echo $RTDIR | grep "^/tmp" > /dev/null
    if [ $? -eq 0 ]
    then
        rm -rf $RTDIR 2> /dev/null
    fi
}

#----------------------------------------------------------------------
# Spit out a test case heading
#----------------------------------------------------------------------
test() {
    #
    # Do any pre-test cleanup
    #
    rm -f $OUTFILE

    echo ""
    echo "TEST: $1"
}

#----------------------------------------------------------------------
# Spit out a note during a test
#----------------------------------------------------------------------
note() {
    echo "$1"
}

#----------------------------------------------------------------------
# Spit out successful completion of a test case
#----------------------------------------------------------------------
pass() {
    echo "Test Passed: $1"
}

#----------------------------------------------------------------------
# Spit out failure message and exit.  We exit on a failed test
# case so that it gets noticed.
#----------------------------------------------------------------------
fail() {
    echo "## TEST FAILED: $1 ##"
    exit 1
}

#----------------------------------------------------------------------
# Run a single command line option test case
#   $1 = expected exit code
#   $2-n = cgquery command line options
#----------------------------------------------------------------------
testopt() {
    expexit=$1
    shift
    cmdline=$*

    test "Command line '$CGQUERY $cmdline'"

    $CGQUERY $cmdline > /dev/null 2>&1
    actexit=$?

    if [ $actexit -ne $expexit ]
    then
        fail "Expected exit $expexit, got $actexit."
    else
        pass "exit status $actexit."
    fi
}

#----------------------------------------------------------------------
# Run a bunch of command line syntax tests
#----------------------------------------------------------------------
TestCmdLineSyntax() {
    testopt 1 
    testopt 0 --version 
    
    testopt 0 -h 
    testopt 0 -help 
    
    testopt 2 -s 
    testopt 1 -s http://www.google.com
    testopt 2 --server 
    testopt 1 --server=http://www.google.com
    
    testopt 1 -a 
    testopt 1 --attributes 
    testopt 2 --attributes=1 
    testopt 1 --attributes --ident
    
    testopt 2 -s 
    testopt 2 --submission 
    testopt 1 --submission=all
    testopt 1 --submission=all --attributes

    testopt 1 -i 
    testopt 1 --ident 
    testopt 2 --ident=1 
    testopt 1 --ident --submission=all

    testopt 1 -A 
    testopt 1 --all-states 
    testopt 2 --all-states=X
    
    testopt 2 -s 
    testopt 2 --submission 
    testopt 1 --submission=all
    testopt 1 --submission=all --attributes
    
    testopt 1 -v 
    testopt 1 --verbose 
    testopt 2 --verbose=1 
    
    testopt 1 -i 
    testopt 1 --interactive 
    testopt 2 --interactive=1 
    testopt 2 --interactive -c 
    testopt 2 --interactive --credential
    testopt 1 --interactive -c $CREDFILE 
    testopt 1 --interactive --credential=$CREDFILE
}

#----------------------------------------------------------------------
# Custom validation that saves a uuid from the query results
#----------------------------------------------------------------------
saveuuid() {
    expcnt=$1
    outfile=$2

    VALIDUUID=`grep "analysis_id.*:.*[0-9a-f-]*" $outfile | head -1 | awk -F ":" '{print $2}' | sed 's/[ 	]*//g'`

    if [ -z "$VALIDUUID" ]
    then
        fail "failed to extract a valid uuid from query results"
    fi
}

randomuuid() {
    expcnt=$1
    outfile=$2

    numuuids=`grep -c "analysis_id.*:.*[0-9a-f-]*" $outfile` 
    idx=`expr $RANDOM % $numuuids + 1`

    VALIDUUID=`grep "analysis_id.*:.*[0-9a-f-]*" $outfile | head -${idx} | tail -1 | awk -F ":" '{print $2}' | sed 's/[ 	]*//g'`

    if [ -z "$VALIDUUID" ]
    then
        fail "failed to extract a valid random uuid from query results"
    fi
}

checkversion() {
    expcnt=$1
    outfile=$2

    VERSION=`grep "Interface Version.*:*" $outfile  | awk -F ":" '{print $2}' | sed 's/[ 	]*//g'`

    echo "VERSION=$VERSION"

    if [ -z "$VERSION" ] || [ "$VERSION" == "Unknown" ]
    then
        fail "test server must implement /version resource"
    fi
}


#----------------------------------------------------------------------
# Custom validation that saves the actual count
#----------------------------------------------------------------------
dispcount() {
    expcnt=$1
    outfile=$2

    DISPCOUNT=`grep -c "^[ ]*Analysis [0-9][0-9]*" $outfile`

    if [ -z "$DISPCOUNT" ]
    then
        fail "failed to extract display count from query results"
    fi
}

#----------------------------------------------------------------------
# Custom validation that saves the number of matching objects
#----------------------------------------------------------------------
hitcount() {
    expcnt=$1
    outfile=$2

    HITCOUNT=`grep "Matching Objects.*: [0-9]*" $outfile  | awk -F ":" '{print $2}' | sed 's/[ 	]*//g'`

    if [ -z "$HITCOUNT" ]
    then
        fail "failed to extract hit count from query results"
    fi
}

#----------------------------------------------------------------------
# Custom validation to check for the correct REST resource
#----------------------------------------------------------------------
checkrest() {

    resname="$1"
    outfile=$2

    #
    # We already checked that we're using a new server
    # so check for the new resource names
    #
    grep -q "REST Resource.*${resname}$" $outfile
    if [ $? -ne 0 ] 
    then
        fail "${resname} REST resource found in results"
    fi
    note "${resname}  REST resource found in results"
}

#----------------------------------------------------------------------
# Custom validation for -I queries.  Make sure we see the
# correct REST resource and that the output contains a subset
# of the fields
#----------------------------------------------------------------------
checkident() {

    expcnt=$1
    outfile=$2

    checkrest "analysisId" $outfile

    grep -q "study.*:" $outfile
    if [ $? -eq 0 ] 
    then
        fail "Found unexpected field in formatted output"
    fi
}

#----------------------------------------------------------------------
# Custom validation for -a queries.  Make sure we see the
# correct REST resource
#----------------------------------------------------------------------
checkattrs() {

    expcnt=$1
    outfile=$2

    checkrest "analysisFull" $outfile
}

#----------------------------------------------------------------------
# Custom validation for '-s all' queries.  Make sure we see the
# correct REST resource
#----------------------------------------------------------------------
checksub() {

    expcnt=$1
    outfile=$2

    checkrest "analysisSubmission" $outfile
}

#----------------------------------------------------------------------
# Custom validation for '-s run_xml' queries.  Make sure we see the
# correct REST resource
#----------------------------------------------------------------------
checksubrun() {

    expcnt=$1
    outfile=$2

    checkrest "analysisSubmission/run_xml" $outfile

}

#----------------------------------------------------------------------
# Custom validation to check sorting
#----------------------------------------------------------------------
checksortuuid() {

    sortopt="$1"
    outfile=$2

    outdir=`dirname $outfile`

    grep "analysis_id [ ]*: [ ]*[0-9a-f-]*$" $outfile > $outdir/uuid_nosort

    grep "analysis_id [ ]*: [ ]*[0-9a-f-]*$" $outfile | sort $sortopt > $outdir/uuid_sort

    diff $outdir/uuid_nosort $outdir/uuid_sort > /dev/null

    if [ $? -ne 0 ] 
    then
        fail "Results not sorted"
    fi
    note "Results properly sorted"

}

#----------------------------------------------------------------------
# Custom validation to check sorting in ascending order
#----------------------------------------------------------------------
checksortascuuid() {

    expcnt=$1
    outfile=$2

    checksortuuid "" $outfile
}

#----------------------------------------------------------------------
# Custom validation to check sorting in descending order
#----------------------------------------------------------------------
checksortdescuuid() {

    expcnt=$1
    outfile=$2

    checksortuuid "-r" $outfile
}

#----------------------------------------------------------------------
# Custom validation to make sure we actually got a CGHUB_error
#----------------------------------------------------------------------
checkcghuberr() {

    expcnt=$1
    outfile=$2

    if [ ! -f $OUTFILE ]
    then
        fail "No output file '$OUTFILE' generated"
    fi

    grep -q CGHUB_error $OUTFILE
    if [ $? -ne 0 ]
    then
        fail "CGHUB_error not found in '$OUTFILE'"
    fi
    note "CGHUB_error found in output file"
}
#----------------------------------------------------------------------
# Custom validation to make sure we actually got an output file
#----------------------------------------------------------------------
checkoutfile() {

    expcnt=$1
    outfile=$2

    if [ ! -f $OUTFILE ]
    then
        fail "No output file '$OUTFILE' generated"
    fi

    xmllint $OUTFILE > /dev/null 2>&1
    if [ $? -ne 0 ]
    then
        fail "Invalid XML data found in '$OUTFILE'"
    fi
    note "XML found in output file"
}

#----------------------------------------------------------------------
# Custom validation to make sure we dit NOT get an output file
#----------------------------------------------------------------------
checknooutfile() {

    expcnt=$1
    outfile=$2

    if [ -e $OUTFILE ]
    then
        fail "Unexpected output file '$OUTFILE' generated"
    fi

}

#----------------------------------------------------------------------
# Custom validation to make sure we got a result in the output file
#----------------------------------------------------------------------
checkoutresult() {

    expcnt=$1
    outfile=$2

    # First, make sure we got a valid output file
    checkoutfile $expcnt $outfile

    # Then, make sure we got a result
    grep -q "<analysis_id[^>]*>" $OUTFILE
    if [ $? -ne 0 ]
    then
        fail "Object result not found in '$OUTFILE'"
    fi
}

#----------------------------------------------------------------------
# Run a single query test case
#   $1 = expected result count if >= 0, or -exit code if less than 0
#   $2 = function to call for any extra checks, "" for none
#   $3-n = cgquery command line options
#----------------------------------------------------------------------
testquery() {

    cmp="-eq"
    expexit=0
    expstr=$1

    echo "$expstr" | grep "\+$" > /dev/null 2>&1
    if [ $? -eq 0 ]
    then
        # Open ended count
        cmp="-gt"
        expcnt=`echo $expstr | sed 's/+.*$//'`
    elif [ $expstr -lt 0 ]
    then
        expexit=`expr $expstr \* -1`
        expcnt=0
    else
        expcnt=$1
    fi

    shift
    checkfn=$1

    shift

    outfile=$RTDIR/rtQuery.out

    cmdline="$@"

    test "Query '$CGQUERY $cmdline'"

    $CGQUERY "$@" > $outfile 
    actexit=$?
    actcnt=`grep -c "^[ ]*Analysis [0-9][0-9]*" $outfile`

    # If there's an extra validation function, call it
    if [ "$checkfn" != "" ]
    then
        $checkfn $expcnt $outfile
    fi

    rm -f $outfile 2> /dev/null

    if [ $actexit -ne $expexit ]
    then
        fail "Expected exit $expexit, got $actexit."
    fi

    if [ ! $actcnt $cmp $expcnt ]
    then
        fail "Expected $expstr results, got $actcnt."
    else
        if [ $actexit -eq 0 ]
        then
            pass "got $actcnt results."
        else
            pass "query returned an error."
        fi
    fi
}

#----------------------------------------------------------------------
# Run a bunch of standard query tests
#----------------------------------------------------------------------
TestQueryResults() {

    #
    # Make sure we're using a recent version of the server
    #
    testquery    1+ checkversion -s $TESTSVR "state=*"

    #
    # For any kind of meaningful testing, we need at least a few objects
    #
    testquery    5+ saveuuid -s $TESTSVR "state=*"

    #
    # Make sure a basic query gets through (this is the only query
    # format supported by our stub server
    #
    testquery    1  "" -s $TESTSVR analysis_id=$VALIDUUID

    #
    # Test for error handling
    #
    testquery   -1  "" -s https://nohub.ucsc.edu state=live
    testquery   -1  "" -s http://www.google.com state=live

    testquery    2+ randomuuid -s $TESTSVR 'center_name=CGHUB&study=CGTEST'
    testquery    1 checkattrs -a -s $TESTSVR "center_name=CGHUB&analysis_id=$VALIDUUID"
    testquery    1 checkattrs --attributes -s $TESTSVR "center_name=CGHUB&analysis_id=$VALIDUUID"
    testquery    1 checksub --submission=all -s $TESTSVR "center_name=CGHUB&analysis_id=$VALIDUUID"
    testquery    1 checksubrun --submission=run_xml -s $TESTSVR "center_name=CGHUB&analysis_id=$VALIDUUID"
    testquery    1 checkident -I -s $TESTSVR "center_name=CGHUB&analysis_id=$VALIDUUID"
    testquery    1 checkoutfile -v -o $OUTFILE -s $TESTSVR "center_name=CGHUB&analysis_id=$VALIDUUID"

    #
    # Added for 2.1 
    # url encoding of spaces
    #
    testquery    2+ saveuuid -s $TESTSVR "title=*Test\ *from*"
    testquery    1  "" -s $TESTSVR "analysis_id=$VALIDUUID&title=*Test\ *from*& &&&"
    testquery    1  "" -s $TESTSVR "analysis_id=$VALIDUUID&title=*Test\ *from*&state=live"
    testquery    1  "" -s $TESTSVR "analysis_id=$VALIDUUID&title=*Test\ *from*&state=liv*"
    testquery    0  "" -s $TESTSVR "analysis_id=XXX$VALIDUUID&title=*Test\ *from*&state=live"

    #
    # XML errors
    #
    testquery -1 checkcghuberr -o $OUTFILE -s $TESTSVR "TEST_ERROR=1"
    testquery -1 checknooutfile -o $OUTFILE -s http://www.google.com "title="

    #
    # Downloadable object filtering
    #
    testquery 5+ dispcount -s $TESTSVR 'center_name=*'
    testquery $DISPCOUNT "" -s $TESTSVR 'study=*'
    testquery 5+ hitcount -s $TESTSVR 'center_name=*'
    testquery $HITCOUNT "" -A -s $TESTSVR 'study=*'

    #
    # Test verbosity level
    #
    testquery 0 checkoutresult -s $TESTSVR -o $OUTFILE "analysis_id=$VALIDUUID"
    testquery 1 checkoutresult -s $TESTSVR -o $OUTFILE -v "analysis_id=$VALIDUUID"
}

#----------------------------------------------------------------------
# Perform a single interactive test case
#   $1 = expected number of files to "download" (fake)
#   $2 = user input file (commands)
#   $3-n = cgquery command line options
#----------------------------------------------------------------------
testint() {

    expexit=0
    expdown=$1

    shift
    infile=$1

    shift
    cmdline=$*

    outfile=$RTDIR/rtQuery.out

    test "Query '$CGQUERY $cmdline'"

    $CGQUERY $cmdline < $infile > $outfile
    actexit=$? 

    if [ $actexit -ne $expexit ]
    then
        fail "Expected exit $expexit, got $actexit."
    fi

    downinfo=`grep "downloads completed successfully" $outfile`
    if [ $? -ne 0 ]
    then
        downok=0
    else
        downok=`echo $downinfo | awk '{print $1}'`
    fi

    if [ $expdown -ne $downok ]
    then
        fail "Some downloads failed.  Expected $expdown, got $downok."
    fi

    pass "$expdown files downloaded"
}

#----------------------------------------------------------------------
# Run a bunch of interactive tests
#----------------------------------------------------------------------
TestInteractive() {

    if [ $major -eq 1 ] && [ $minor -lt 7 ]
    then
        # --gt-binary not supported before 1.7, so we can't automate
        # the interactive tests.  Rats.   We could modify the cgquery
        # script on the fly, but that seems extreme.
        return
    fi
    
    infile=$RTDIR/rtQuery.in
    gtbin=$RTDIR/rtGTBin

    cp $GTBIN $gtbin
    cat >> $gtbin << _EOF_
    exit 0
_EOF_

    # Get a count of the live records
    testquery    2+ dispcount --input-xml=$INFILE center_name=CGHUB

    # Just quit at the prompt
    echo "q" > $infile
    testint 0 $infile -g $gtbin -c $CREDFILE -i --input-xml=$INFILE center_name=CGHUB

    # Select index 3
    echo -e "3\nq" > $infile
    testint 1 $infile -g $gtbin -c $CREDFILE -i --input-xml=$INFILE center_name=CGHUB

    # Select index 0 (all)
    echo -e "0\nq" > $infile
    testint $DISPCOUNT $infile -g $gtbin -c $CREDFILE -i --input-xml=$INFILE center_name=CGHUB

    if [ $major -gt 1 ] || [ $minor -ge 9 ]
    then
        # Try some range tests (first idx greater than second)
        echo -e "5-3\nq" > $infile
        testint 0 $infile -g $gtbin -c $CREDFILE -i --input-xml=$INFILE center_name=CGHUB

        # Try some range tests (non numeric)
        echo -e "a-b\nq" > $infile
        testint 0 $infile -g $gtbin -c $CREDFILE -i --input-xml=$INFILE center_name=CGHUB

        # Try some range tests (valid range)
        echo -e "3-5\nq" > $infile
        testint 3 $infile -g $gtbin -c $CREDFILE -i --input-xml=$INFILE center_name=CGHUB

        # Try some range tests (invalid range followed by valid range)
        echo -e "3 - 5\n3-6\nq" > $infile
        testint 4 $infile -g $gtbin -c $CREDFILE -i --input-xml=$INFILE center_name=CGHUB
    fi

    # Let's make GTBin fail
    cp $GTBIN $gtbin
    cat >> $gtbin << _EOF_
    exit 1
_EOF_

    # Select index 3
    echo -e "3\nq" > $infile
    testint 0 $infile -g $gtbin -c $CREDFILE -i --input-xml=$INFILE center_name=CGHUB
}

#----------------------------------------------------------------------
# Run a bunch of tests with the XML taken from an input file
# instead of the actual server
#----------------------------------------------------------------------
TestFromInputFile() {
    if [ $major -eq 1 ] && [ $minor -lt 9 ]
    then
        # --input-xml not supported before 1.9
        return
    fi

    infile=$RTDIR/rtQuery.in
    
    # Non-existent input file
    rm -f $infile 2> /dev/null
    testquery  -1 "" --input-xml $infile state=live

    # Valid file
    cp $INFILE $infile
    testquery 5+ hitcount --input-xml $infile "state=*"

    # Corrupt the ResultSet token
    sed 's/<ResultSet/<XXXResultSetXXX/' $INFILE > $infile
    testquery -1 "" --input-xml $infile state=live

    # Don't terminate the ResultSet.  Should be okay.
    grep -v "</ResultSet" $INFILE > $infile
    testquery $DISPCOUNT "" --input-xml $infile "state=*"

    # Corrupt one of the Result objects, expect it to be skipped
    # Is that correct?
    sed 's/<Result id="4"/<XXXResult id="4"/' $INFILE > $infile
    expcount=`expr $DISPCOUNT - 1`
    testquery $expcount "" --input-xml $infile state=live

    # Corrupt one of the objects inside the Results.  Expect a parse error.
    sed '1,/<files>/s/<files>/<XXXfilesXXX/' $INFILE > $infile
    testquery -1 "" --input-xml $infile state=live

}

#----------------------------------------------------------------------
# Main
#----------------------------------------------------------------------


$COV_ERASE
TestCmdLineSyntax
TestQueryResults
TestInteractive
TestFromInputFile
echo -e "\n@@ All tests passed.\n"
$COV_REPORT

