#!/bin/bash

BINDIR=/home/$USER/.local/bin
LIBDIR=/home/$USER/.local/lib/maptrace
BIN=$BINDIR/maptrace
PIN=$(realpath pin/pin)
MAPTRACE=$(realpath pin/source/tools/maptrace/obj-intel64/maptrace.so)

mkdir -p $BINDIR
mkdir -p $LIBDIR

cp $MAPTRACE $LIBDIR

echo "#!/bin/bash" > $BIN
echo "CMD=\"\$@\"" >> $BIN
echo "$PIN -t $LIBDIR/maptrace.so \$CMD" >> $BIN

cp vis/vis $BINDIR

chmod +x $BIN
