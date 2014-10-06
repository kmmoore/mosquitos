#! /bin/sh

PROJECT_BASE=`dirname $0`
cd $PROJECT_BASE

echo "Generating build info..."
/usr/bin/env python build_info.py > src/common/build_info.c

make -C src/kernel/drivers/acpica
make $1