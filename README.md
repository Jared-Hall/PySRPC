# PySRPC
My own variant of SRPC for python. It's a bit faster than a pure python implementation since most of the work is done in C.

## Dependancies
1. Python 3.5+
2. libtool
3. distutil
4. SRPC
5. ADTs

## Installation instructions
1. Clone or download the PySRPC source.
2. Install the C-SRPC source:
   1. Clone the following from https://www.github.com/jsventek/ into your PC:
      1. ADTs
      2. SRPC
   2. In each run the following commands as root:
      1. ``libtoolize``
      2. ``autoreconf -if``
      3. ``./configure``
      4. ``make clean``
      5. ``make``
      6. ``make install``
   3. Add the libraries for linking:
      1. execute the following: ``sudo vim /etc/ld.so.conf.d/usrlocallib.conf``
      2. add the line: /usr/local/lib and save.
      3. execute: ``sudo ldconfig``
3. Install the python c-extension for SRPC: 
   1. cd to PySRPC/src/core
   2. execute: ``python3 setup.py install --user``
Done! PySRPC is now installed on your system.

I included a simple example of an echo client and server running SRPC. Give them a look to see how to use the protocol.
I havn't extended all of the SRPC functionality just yet, only the parts that I was actually using. Can extend more if needed.

Notes:
1. On Raspberry Pi you will need to install both yacc and flex before installing PySRPC
  i. ``sudo apt-get install byacc``
  ii. ``sudo apt-get install flex``

If you have any questions email me (Jared) @: jhall10@uoregon.edu
