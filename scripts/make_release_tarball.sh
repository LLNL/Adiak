#!/bin/bash
# Copyright 2020 Lawrence Livermore National Security, LLC
# See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: MIT
##############################################################################

TAR_CMD=gtar
VERSION=0.2.2

origdir=`pwd`
rootdir=`git rev-parse --show-toplevel`

cd $rootdir

git archive --prefix=adiak-${VERSION}/ -o adiak-${VERSION}.tar HEAD 2> /dev/null

echo "Running git archive submodules..."

p=`pwd` && (echo .; git submodule foreach) | while read entering path; do
    temp="${path%\'}";
    temp="${temp#\'}";
    path=$temp;
    [ "$path" = "" ] && continue;
    (cd $path && git archive --prefix=adiak-${VERSION}/$path/ HEAD > $p/tmp.tar && ${TAR_CMD} --concatenate --file=$p/adiak-${VERSION}.tar $p/tmp.tar && rm $p/tmp.tar);
done

gzip adiak-${VERSION}.tar

cd $origdir
