function configureRPMBuild
{
   cd rpmbuild/SPECS

   sed -i "s/Version:.*/Version:        ${geneTorrentVer}/" GeneTorrent.spec

   cd - > /dev/null
   cd rpmbuild/SOURCES

   # dummy source hack
   if [[ -e GeneTorrent-${geneTorrentVer} || -e GeneTorrent-${geneTorrentVer}.tgz ]]
   then
      rm -f GeneTorrent-${geneTorrentVer} GeneTorrent-${geneTorrentVer}.tgz
   fi
   cp -ar GeneTorrent-template GeneTorrent-${geneTorrentVer}
   tar czf GeneTorrent-${geneTorrentVer}.tgz GeneTorrent-${geneTorrentVer}

   cd - > /dev/null
}

function buildRPMS
{
   if [[ -e ~/rpmbuild ]]
   then
      rm -rf ~/rpmbuild
   fi

   cp -ar rpmbuild ~/.
   cd ~/rpmbuild/SPECS

   rpmbuild -bb GeneTorrent.spec

   cd - > /dev/null
}

function collectRPMS
{
   rm -rf ~/GeneTorrent-${geneTorrentVer}
   mkdir ~/GeneTorrent-${geneTorrentVer}

   cd ~/rpmbuild/RPMS/x86_64
   cp GeneTorrent-${geneTorrentVer}-1.el6.CP.x86_64.rpm ~/GeneTorrent-${geneTorrentVer}/.

   cd - > /dev/null

   cp ${startDir}/release.notes.txt ~/GeneTorrent-${geneTorrentVer}/.

   cd

   tar czf GeneTorrent-${geneTorrentVer}.tgz GeneTorrent-${geneTorrentVer}

   echo "All done!  GeneTorrent-${geneTorrentVer}.tgz is available in `pwd` for testing and delivery to the client."
}

geneTorrentVer="`grep AC_INIT GeneTorrent/configure.ac|cut -d, -f2|tr -d ')'|awk '{print $1}'`"
startDir="$PWD"

configureRPMBuild
buildRPMS
collectRPMS
