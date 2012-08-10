===================
Block IO API Design
===================

.. contents::

.. sectnum::

.. footer::
   |cp_logo_sm|

   Page ###Page###

.. raw:: pdf

   PageBreak

.. |cp_logo_sm| image:: CP_logo_rgb_sm.jpg

.. COMMENT: The following replacements allow easy changing of API
   names should they need to change as the design evolves. If the API
   names change in the included header file, will also need to make
   the changes here instead of doing global search and replace.

.. |gtio|          replace:: **GTIO**
.. |gtio_error|    replace:: **GTIO_Error**
.. |gtio_fileinfo| replace:: **GTIO_FileInfo**
.. |gtio_dir|      replace:: **GTIO_Dir**
.. |gtio_dirent|   replace:: **GTIO_Dirent**

.. |gtio_open|     replace:: **gtio_open_\*()**
.. |gtio_open_gto| replace:: **gtio_open_gto()**
.. |gtio_open_uri| replace:: **gtio_open_uri()**
.. |gtio_close|    replace:: **gtio_close()**
.. |gtio_read|     replace:: **gtio_read()**
.. |gtio_pread|    replace:: **gtio_pread()**
.. |gtio_lseek|    replace:: **gtio_lseek()**

.. |gtio_get_file_list| replace:: **gtio_get_file_list()**
.. |gtio_opendir|  replace:: **gtio_opendir()**
.. |gtio_readdir|  replace:: **gtio_readdir()**
.. |gtio_closedir| replace:: **gtio_closedir()**

.. |blk_api|       replace:: **Block IO API**

.. NOTES FROM HOWDY:

   > I'm thinking about passing in some object to store state instead of
   > using 'int fd'.

   I would favor using an opaque pointer, more similar to the FILE*
   returned by fopen.  So it would be a GTHandle \*, returned by gt_open,
   and passed into all the other calls.  Any memory that was allocated
   into this handle should be freed by gt_close.

   > Biggest question in my mind right now is whether to return in quantum
   > of blocks (size spec'd by caller of open) or to use quantum of byte
   > (return a range of bytes like normal posix read() does).

   I'd favor bytes, personally.

   Some other questions:

   - How would the behavior of gt_pread be different than gt_read, under
     the covers? [troth: done]

   - I think we will end up with multiple versions of gt_open, that would
     basically parallel the arguments you can currently pass to
     GeneTorrent -d: [troth: done]

   One version should take a pointer to a GTO file -- perhaps this could
   be a filename, or a URI, or a char * buffer that simply contains the
   GTO file.  (Although you cannot count on a GTO file not containing
   \0's, so if you go this route the caller would also need to pass a
   size argument so that gt_open knows how big the buffer containing the
   GTO is.) [troth: done]

   Another version should take a fully-qualified URI pointing to an
   analysis object at the GeneTorrent Executive. [troth: done]

   Don't know if we want to support the XML file approach -- would be a
   question for Christy.

   - Would it be possible to discuss in this spec the tasks performed
     under the covers for each call?  For instance, where do the calls to
     GT Exec happen?  (I'm assuming they are either a) performed
     synchronously during the gt_open call (and thus if there are any
     errors the error will be returned from the call to gt_open), or b)
     performed asynchronously after the gt_open call (and thus we need a
     method to capture errors that occur asynchronously).

   - Likewise, some discussion of performance and perhaps some pseudo
     code for how we are going to implement each of the calls would be
     worthwhile.


Introduction
------------

The |blk_api| will allow a user to download a subset of the data
available for a given UUID as a range of bytes of data, or even
multiple, overlapping ranges of data, without having to download the
entire data set associated with the UUID.

It will also allow random, remote access to BAM files so an
application can access BAM data for small portions of a genome with
out having to download the entire BAM

Functional API
--------------

The following functions will be exposed to the higher level application:

.. include:: ../GeneTorrent/src/gtBlockIO.h
    :start-after: // DOC: start: Functional API
    :end-before: // DOC: end: Functional API
    :literal:

.. raw:: pdf

   PageBreak

|gtio|
    Opaque pointer to torrent file descriptor information.

    The user of the API will not have access to the internals of this
    object.

|gtio_error|
    Structure to be allocated by the caller. Could either supply
    functions to the library for reporting the error or just be a place
    for the library to write information about the error.

    The fields of this structure are still to be determined.

|gtio_open_gto|
    Opens a connection to the torrent specified with a GTO file. The
    GTO file should be read into memory by the caller and passed in
    the *gto_buf* as well as the size of the GTO buffer via *gto_size*.
    Note that GTO files may contain null (zero) bytes so it can not be
    considered a null terminated string.

    The *cred* argument is the credentials authorization token which
    will allow you to access the data repo. It should be a null
    terminated buffer.

    Returns:

    * On success, returns a pointer to an opaque |gtio| object for the
      interaction.
    * On failure returns **NULL** and error information is placed in
      *err*.

|gtio_open_uri|
    Similar to |gtio_open_gto|, but takes a fully quallified URI
    string as the specification of the torrent.

|gtio_close|
    Closes the connection to the torrent. Must be called when the
    caller is done with the |gtio| object.

    Failure to close a |gtio| object will result in a memory leak.

    Returns:

    * 0 on success.
    * -1 on failure, error information will be placed in *err*.

|gtio_read|
    Reads up to count bytes from the torrent starting from the current
    offset. The offset (stored in the |gtio| object) is then
    incremented by the number of bytes read.

    Returns:

    * Number of bytes read on success.
    * -1 on failure, error information will be placed in *err*.

|gtio_pread|
    Reads up to count bytes from the torrent at the given offset, but
    the file offset in the |gtio| object is not changed (as would
    happen with |gtio_read| or |gtio_lseek|).

    Returns:

    * Number of bytes read on success.
    * -1 on failure, error information will be placed in *err*.

|gtio_lseek|
    Repositions the offset into the torrent such that the next call to
    |gtio_read| will start reading at that offset. The offset stored
    in the |gtio| object is updated.

    Returns:

    * 0 on success.
    * -1 on failure, error information will be placed in *err*.

.. note:: Still debating if we need the following functions. It will
          likely be useful to an application using the |blk_api| to
          get a list of files that are in the torrent. The following
          are really two mutually exclusive API alteratives
          (get_file_list vs. opendir/readdir/closedir).

|gtio_get_file_list|
    Get a list of files in the torrent. For each file, returns an info
    structure with the name of the file, the offset of the file in the
    torrent and the size of the file in bytes.

    Will allocate memory for the file info list which must be freed by
    caller. The returned file info list will be an array of
    |gtio_fileinfo| structures which not be NULL terminated.

    If the function fails in any way, the *\*\fi* argument will be set
    to **NULL** and any memory allocated before the failure will be
    freed before the function returns.

    Returns:

    * The number of files listed on success.
    * -1 on failure, error information will be placed in *err*.

|gtio_opendir|
    Get a "directory" pointer into the torrent.

    Must call |gtio_closedir| on the returned *dirp* when done with
    it.

    Memory will be allocated for *dirp*. User must call
    |gtio_closedir| to free the memory.

    Returns:

    * A pointer to a valid |gtio_dir| on success.
    * **NULL** on failure, error information will be placed in *err*.

|gtio_readdir|
    Get info on the next file in the torrent as denoted by the *dirp*
    argument.

    The caller is responsible for allocated storage for *ent*.

    The *ent* argument will be filled in with the file name, file size
    and offset in the torrent stream.

    Returns:

    * 1 if all files have been accessed.
    * 0 on success.
    * -1 on failure, error information will be placed in *err*.

|gtio_closedir|
    Free up allocated resources for *dirp* argument.

    Returns:

    * 0 on success.
    * -1 on failure, error information will be placed in *err*.

These functions will provide POSIX-like interfaces where possible, but
will not adhere to strict POSIX specifications.

.. raw:: pdf

   PageBreak

Command Line Invocation
-----------------------

The GeneTorrent download client will have access to the |blk_api|
functionality via the following command line options:

**--gtio-file-range <uuid>:<filename>:<start>:<length>**
    Restricts the data downloaded to the given range of the specified
    file within the given UUID torrent.

    The downloaded data will be written to the following relative path::

        <uuid>/<filename>-<start>:<length>

    The option value will consist of the following colon separated fields:

    <uuid>
       The uuid of the analysis object. This is needed to continue to
       allow multiple the user to pass the *-d* option multiple time
       on a single invocation of GeneTorrent.

    <filename>
       The name of file in the torrent.

    <start>
       The offset (in bytes) of the begining of the range in the
       file. This is relative to the first byte of the file.

    <length>
       The number of bytes to be read from the file.

**--gtio-range <uuid>:<start>:<length>**
    Restricts the data downloaded to the given range of the specified
    UUID torrent.

    The downloaded data will be written to the following relative path::

        <uuid>/range-<start>:<length>

    The option value will consist of the following colon separated fields:

    **<uuid>**
       The uuid of the analysis object.

    **<start>**
       The offset (in bytes) of the begining of the range in the
       torrent. This is relative to the first byte of the torrent stream.

    **<length>**
       The number of bytes to be read from the torrent.

Multiple ranges can be specified for a single invocation. Overlapping
ranges are also permissible.

Internal Implementation Details
-------------------------------

Opening a UUID
++++++++++++++

Each UUID has a 1-to-1 relationship with a GTO file. As such, each
|gtio| object will have a 1-to-1 relationship with a torrent object
which is associated with a UUID.

Torrent sessions will be handled internally by the |gtio_open|
functions. When called, an existing session will be used, or one will
be created if needed. A single torrent session will likely be
associated with multiple |gtio| objects.

During the |gtio_open| process, the tracker will be sent a *start*
event. The tracker will also be sent a *stopped* event during the
|gtio_close| process.

Also during the |gtio_open| process, the GT Executive will need to be
contacted in order to get the GTO file processed so the client can
start fetching pieces as a result of future calls to |gtio_read|.

Reading from a Torrent
++++++++++++++++++++++

The |gtio| object will maintain a current offset value. The offset
will be incremented with each call to |gtio_read|. Internally,
|gtio_read| will manage the offset and call |gtio_pread| to perform
the actual read operation.

|gtio_read| psuedocode implementation:

.. include:: ../GeneTorrent/src/gtBlockIO.cpp
    :start-after: // RST-DOC: start: read
    :end-before: // RST-DOC: end: read
    :literal:

|gtio_pread| psuedocode implementation:

.. include:: ../GeneTorrent/src/gtBlockIO.cpp
    :start-after: // RST-DOC: start: pread
    :end-before: // RST-DOC: end: pread
    :literal:

The heavy lifting of getting the pieces needed will be done by
|gtio_pread|. It will need to generate the mask of pieces that it
needs and asynchronously fetch the pieces with libtorrent.

|gtio_lseek| psuedocode implementation:

.. include:: ../GeneTorrent/src/gtBlockIO.cpp
    :start-after: // RST-DOC: start: lseek
    :end-before: // RST-DOC: end: lseek
    :literal:

Writing to a Torrent
++++++++++++++++++++++

The initial implementation of the |blk_api| will be read-only.
Write operations on a torrent will not be allowed.

Caching
+++++++

The underlying implementation will cache recently downloaded pieces to
avoid having to re-download them on subsequent reads.

The cache will be implemented as a map keyed by the sha1 value of the
piece. Each data item in the map will be a (time, piece_data)
pair. The algorithm for accessing the cache follows (psuedo code)::

    uint8_t *check_cache(string &piece_sha1)
    {
        if (piece_sha1 in cache)
	    return cache[piece_sha1].second();
	else
            return NULL;
    }

    void insert_cache(string &sha1, uint8_t *data)
    {
        time t = now();
	cache[sha1] = pair(t, data);

	while (cache.size() > MAX_CACHE_SIZE)
            cache.remove_oldest()
    }

Example of using the cache::

    // Memory allocation/freeing of data still needs addressed.
    uint8_t *get_piece(string &piece_sha1, int piece_index)
    {
        uint8_t *data = check_cache(piece_sha1);
	if (!data)
	{
	    data = download_piece(piece_sha1, piece_index);
	    if (data)
	    {
	        insert_cache(piece_sha1, data);
	    }
	}
	return data;
    }

Since GeneTorrent is typically dealing with 4 MB piece sizes, having a
cache depth of 25 pieces will require 100 MB memory overhead for the
application. To allow the user to tune the cache for their
environment, the depth of the cache will need to be configurable via
either the command line or a config file.

.. note:: Is this caching scheme too simplistic when downloading lots
          of overlapping ranges? At what point does just downloading
          the entire torrent become more practical?

.. raw:: pdf

   PageBreak

Thread Safety
+++++++++++++

It is assumed that most applications using the |blk_api| will be
multi-threaded. As such, care must be taken in the implementation to
ensure the consistency of the |gtio| object which will need to be
shared by multiple threads.

It is likely that applications will want to improve performance by
parallelizing reads. Making multiple calls to |gtio_read| will be
tricky since each read will be competing for the |gtio| object in
order to update the current offset which will make the offset
unreliable.

A better approach would be to have the application precompute the
offsets for each read and perform multiple calls to |gtio_pread|.

Both the |gtio_read| and |gtio_lseek| functions are inherently not
thread safe due to the use of the offset pointer in the |gtio| object.

Another domain where thread safety will be a concern is in FUSE
implementations. A file system can not assume that there is only one
application holding a given file descriptor. Consider the case where
an application opens a file (which is supplied via a FUSE
implementation using the |blk_api|). If the application forks, then
there are now two processes holding onto the same file descriptor.

.. raw:: pdf

   PageBreak

Performance Considerations
--------------------------

There are three main areas to consider regarding performance:

* Open
* Close
* Read

Also of interest is how the proposed range options for GeneTorrent
will work with the |blk_api|.

Performance of Open
+++++++++++++++++++

Calls to |gtio_open| can possibly take many minutes to complete since
they need to perform multiple actions over the network:

* Fetch GTO from GT Executive (if using |gtio_open_uri|).
* Send GTO to WSI for key signing.
* Interact with the tracker.

In the best case, all of these happen quickly and |gtio_open|
completes within a few seconds.

In worst case, the WSI or tracker is heavy loaded and some of the
accesses timeout and are eventually successful on a retry or fail
altogether.

Performance of Close
++++++++++++++++++++

Calls to |gtio_close| need to tear down the torrent and possibly the
torrent session if no other torrent belongs to the session. This also
involves waiting for notifications from the torrent and sending the
stop event to the tracker.

Not sure what the best/worst cases are. There is a 5 second
**sleep()** in the code before shutting down the torrent to work
around some edge case issues.

Performance of Reads
++++++++++++++++++++

Things get ugly regarding calls to |gtio_read|.

There will be issues with serialization of torrent piece fetching if
you don't know everything you are going to fetch up front.

Consider the following example situation:

   Suppose the user wants to read 10 GB of data sequentially from a
   BAM. To avoid having to allocate a 10 GB buffer (which may not be
   possible on their system), they decide to code their application so
   it makes a series of calls to |gtio_read| in a for loop, with each
   read asking for 1 MB. Every fourth read will cause a torrent piece
   to be downloaded (assuming 4 MB piece length) and the application
   will be blocked from doing any processing until the download
   completes. They have effectively killed any potential benefits of
   the bittorrent transfer parallelism by doing this.

How is this going to fit into FUSE and have any kind of decent
performance? The reads could be very random in nature and may kill
caching performance.

Command Line Usage Performance
++++++++++++++++++++++++++++++

When the |blk_api| is used with the range command line options
(`--gtio-file-range` and/or `--gtio-range`) of GeneTorrent, then
GeneTorrent could make some optimizations by precomputing all of the
pieces it will need. It can then perform that reads in such a way to
minimize the time waiting for downloading of pieces to complete. This
is still suboptimal and counter to the way bittorrent works.

Userspace Filesystem
--------------------

.. Do we have a solid use case for what Annai wants to do with the
   |blk_api| using FUSE? Matt and I talked about this and in general
   everything came back to FUSE using reads to force pieces to be
   download sequentially and serially killing any benefit of
   bittorrent. We really need a good use case outlining how they plan
   to use the |blk_api|.

On Linux systems, using the FUSE library, one can create new
filesystems in userspace.  In order to map a torrent into a file
system, the |blk_api| will need to supply functionality needed by
FUSE.

On other operating systems, there are differing implementations of
userspace filesystems.

FUSE Information
++++++++++++++++

General information on FUSE:

* http://fuse.sourceforge.net/

Example application:

* http://fuse.sourceforge.net/helloworld.html

FUSE operations which can be supplied by an application to implement a
filesystem:

* http://fuse.sourceforge.net/doxygen/structfuse__operations.html

Not all operations are of interest to GeneTorrent. Operations that
will likely need support from the |blk_api| are:

* open()
* read()
* flush()
* release()
* getattr()
* readdir()

.. note:: FUSE does not support a *close()* method. See the *flush()*
          and *release()* methods.
