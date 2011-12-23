

function build_libtorrent
{
   pwd
   ls -l
cd /tmp
   pwd
}

function build_GeneTorrent
{
   pwd
   ls -l
}

function build_scripts
{
   pwd
   ls -l
}

for dir in libtorrent GeneTorrent scripts
do
   saveDir=${PWD}
   cd ${dir}
   build_${dir}
   cd ${saveDir}
done
