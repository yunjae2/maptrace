# maptrace

A toolbox for analysis of memory access patterns in ROIs (functions) of an application.
This tool is based on Intel PIN.

## Installation
```
$ git clone https://github.com/yunjae2/maptrace
$ cd maptrace/
$ ./install.sh
```

## Usage

__1. Extracting the memory access trace__
```
$ maptrace [-f <roi (regex)>] [-o <output file>] [-l <function log>] [-s <sampling rate>] -- <command>
```
- Example
```
$ maptrace -f “^at::native::_convolution$” -o Conv2d.trace -- python3 test.py
```
In this example, the trace data is saved as a file with name `Conv2d.trace` in current directory.


__2. Visualizing the trace data__
```
$ vis [-r <rconfig>] <trace file>
```
- Example
```
$ vis -r ranges.yaml Conv2d.trace
```
`vis` generates visualized trace as figures in the directory named after the trace file name. In the example above, the figures are saved under `Conv2d/`. Since memory accesses are not dense in the address space (usually, most of the address space is not allocated), `vis` allows users to specify memory address ranges to limit the scope of address space. This is done by specifying `-r` option with `rconfig` file, which contains address range information. In addition, the rconfig file should be written in the YAML format:

- `rconfig` file format
```
<range 1>:
    addr: <start address of the range>
    size: <size of the range (in bytes)>
<range 2>:
…
```
- Example `rconfig`
```
Conv2d0input:
    addr: 140066565758976
    size: 12288
Conv2d0weight:
    addr: 94894153251328
    size: 1800
Conv2d0bias:
    addr: 94894120171072
    size: 24
Conv2d1input:
    addr: 94894153060096
    size: 4704
Conv2d1weight:
    addr: 94894153253888
    size: 9600
Conv2d1bias:
    addr: 94894120149312
    size: 64
```
