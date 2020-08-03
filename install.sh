#!/bin/bash

BINDIR=/home/$USER/.local/bin
LIBDIR=/home/$USER/.local/lib/maptrace
BIN=$BINDIR/maptrace
PIN=$(realpath pin/pin)
MAPTRACE=$(realpath pin/source/tools/maptrace/obj-intel64/maptrace.so)
MAPTRACEDIR=$(realpath pin/source/tools/maptrace/)

mkdir -p $BINDIR
mkdir -p $LIBDIR

# 1. Build pin tool
cd $MAPTRACEDIR
make
cd -

# 2. Install pin tool
cp $MAPTRACE $LIBDIR

echo "#!/bin/bash" > $BIN
echo "CMD=\"\$@\"" >> $BIN
echo "$PIN -t $LIBDIR/maptrace.so \$CMD" >> $BIN

chmod +x $BIN

# 3. Install vis tool
cp vis/vis $BINDIR
