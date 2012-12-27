#!/usr/bin/env python
import os
import sys
import time
import ctypes
import urllib
import urlparse
import SimpleHTTPServer
import BaseHTTPServer
import ConfigParser
import httplib
from   lxml import etree

MY_NAME = os.path.abspath(sys.argv[0])
MY_DIR  = os.path.dirname(MY_NAME)

if os.path.basename(MY_DIR) == "test":
    # We're running in the source tree
    SRC_DIR = os.path.dirname(MY_DIR)
else:
    # We're running in a runtime tree
    SRC_DIR = MY_DIR

sys.path.insert(0, SRC_DIR)

sys.path.append("../../eraimporter/test")

from testsvr import *

class TestWSIHTTPRequestHandler (TestHTTPRequestHandler):

    def test_Error(self, msg):
        """ ------------------------------------------------------------------
            Report an internal (unexpected) error in the test server
            ------------------------------------------------------------------
        """

        self.log_message("Internal test server error: %s" %(msg))

        self.send_response(444)
        self.send_header('Content-type', 'text/xml')
        self.end_headers();
        self.wfile.write("%s"%(msg))

    def svr_Success(self, msg, status):
        """ ------------------------------------------------------------------
            Report successful completion of a request, sending the 
            passed status string as the body
            ------------------------------------------------------------------
        """

        self.log_message("Success: %s" %(msg))

        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers();
        self.wfile.write(status)

    def svr_Error(self, msg, uri, qsdict):
        """ ------------------------------------------------------------------
            Report a simulated (expected) server error using the standard
            CGHUB_error format
            ------------------------------------------------------------------
        """

        self.log_message("Simulated error: %s" %(msg))

        self.send_response(400)
        self.send_header('Content-type', 'text/xml')
        self.end_headers();

        input=""
        for k, vl in qsdict.items():
            for v in vl:
                input += "%s=%s "%(k, v)

        errstr  = "<CGHUB_error>"
        errstr += "<subsystem>WebServices 3.0 Test</subsystem>"
        errstr += "<input>%s</input>"%(input.strip())
        errstr += "<errnum>99-999</errnum>"
        errstr += "<timestamp>2012-12-06 20:13:14</timestamp>"
        errstr += "<usermsg>%s</usermsg>"%(msg)
        errstr += "<effect>Bad things will happen</effect>"
        errstr += "<remediation>Fix it</remediation>"
        errstr += "</CGHUB_error>"

        self.wfile.write(errstr)

    def filter_result(self, rawxml, uuid):
        """ ------------------------------------------------------------------
            Given a complete result set, filter out the Result element
            for a single uuid, and return a new ResultSet, containing 
            just that element.
            ------------------------------------------------------------------
        """

        root = etree.fromstring(rawxml)
        nodelist = root.xpath("//ResultSet/Result")
    
        #
        # Loop over each Result element, deleting those that
        # are not the UUID we're looking for
        #
        for node in nodelist:
            matches = node.xpath("./analysis_id[text()='%s']"%(uuid))

            if len(matches) == 0:
                root.remove(node)
            else:
                #
                # Ok, we've found the matching result.  Update the id
                #
                node.attrib['id']='1'

        #
        # Send back whatever we're left with
        #
        nodelist = root.xpath("//ResultSet/Result")
        hitlist = root.xpath("//ResultSet/Hits")
        hitlist[0].text = "%s"%(len(nodelist))
        return etree.tostring(root)

    def handle_metadata_request(self, reqparts, uri, qsdict):
        """ ------------------------------------------------------------------
            Handle a /cghub/metadata/<querytype> request
            ------------------------------------------------------------------
        """
        
        # 
        # Minimally, we need the query type
        #
        if len(reqparts) < 3:
            raise ValueError("Invalid metadata request.  Insufficient URI '%s'"%("/".join(reqparts)))

        #
        # Do we see our special marker in the querystring that indicates
        # that we should return an error?
        #
        if qsdict.has_key('TEST_ERROR'):
            self.svr_Error("Simulated error occurred", uri, qsdict)
            return

        #
        # Get the query type, as well as the qualifier, if any
        #
        qtype = reqparts[2]
        if len(reqparts) > 3:
            qtype += "-" + reqparts[3]

        infile="testdata/%s.xml"%(qtype)

        #
        # We only recognize "analysis_id=XXX" within the query.  All 
        # other query parameters are ignored.
        #
        uuid=None
        if qsdict.has_key('analysis_id'):
            uuid = qsdict['analysis_id'][0]
            print "uuid = %s"%(uuid)

        #
        # Read the input XML from a datafile named after the query type
        #
        f = open(infile, 'r')
        result = f.read()
        f.close()

        #
        # Filter out a specific uuid, if requested
        #
        if uuid != None:
            result = self.filter_result(result, uuid)

        #
        # Return success along with the XML response
        #
        self.svr_Success(uri, result)

    def handle_request(self, uri, qsdict):
        """ ------------------------------------------------------------------
            Handle a single request
            ------------------------------------------------------------------
        """

        #
        # URI starts with /, so drop the first item which
        # will be ''.   
        #
        reqparts = uri.split('/')[1:]
        if reqparts[0] != 'cghub' or len(reqparts) < 3:
            raise ValueError("Invalid request '%s'"%(uri))
        request=reqparts[1]

        if request == "interface" and reqparts[2] == "version":
            self.svr_Success(uri, "<GNOS_result><WebServices><version>3.0</version></WebServices></GNOS_result>")
        elif request == "metadata":
            self.handle_metadata_request(reqparts, uri, qsdict)
        else:
            raise ValueError("Unknown request '%s'"%(request))

    def do_POST(self):
        """ ------------------------------------------------------------------
            We don't handle any POST requests currently.
            ------------------------------------------------------------------
        """
        self.test_Error("POST is not yet supported")

    def do_GET(self):
        """ ------------------------------------------------------------------
            Handle a GET request
            ------------------------------------------------------------------
        """
        uri = None
        qsdict = None
        try:
            #
            # Parse the query string
            #
            if self.path.find('?') < 0:
                uri = self.path
            else:
                uri_parts = self.path.split('?',1)
                uri = uri_parts[0]
                qsdict = urlparse.parse_qs(uri_parts[1])

        except Exception, err:
            self.test_Error("Failed to parse request: %s"%(err))
            return

        #
        # Now, execute the request
        #
        try:
            self.handle_request(uri, qsdict)
        except Exception, err:
            self.test_Error("Failed to handle request: %s"%(err))
            return

if __name__ == '__main__':

    HandlerClass = TestWSIHTTPRequestHandler
    ServerClass  = TestHTTPServer

    test_httpd_main(ServerClass, HandlerClass)

