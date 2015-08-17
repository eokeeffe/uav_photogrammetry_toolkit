
Automatic Color Enhancement (ACE) and its Fast Implementation
Pascal Getreuer, getreuer@gmail.com
Version 20121029 (October 29, 2012)

  *** Please cite IPOL article "Automatic Color Enhancement (ACE) and ***
  *** its Fast Implementation" if you publish results obtained with   ***
  *** this software.                                                  ***


== Overview ==

This C source code accompanies with Image Processing On Line (IPOL) article
"Automatic Color Enhancement (ACE) and its Fast Implementation" by Pascal 
Getreuer.  The article and online demo can be found at

    http://www.ipol.im

Future software releases and updates will be posted at 

    http://dev.ipol.im/~getreuer/code/


== License (BSD) ==

File avs.jpg was created by NASA (http://dragon.larc.nasa.gov/retinex/).

All other files are distributed according to the simplified BSD license.  You 
should have received a copy of this license along this program.  If not, see 
<http://www.opensource.org/licenses/bsd-license.html>.


== Program Usage ==

This source code includes two command line programs: ace and histeq

    * ace: runs ACE color enhancement

    * histeq: performs uniform histogram equalization


--- ace ---

Usage: ace [options] input output

where "input" and "output" are BMP files (JPEG, PNG, or TIFF files can also 
be used if the program is compiled with libjpeg, libpng, and/or libtiff).  

Options:
  -a <number>  alpha, stronger implies stronger enhancement
  -w <omega>   omega, spatial weighting function, choices are
               1/r      default ACE, omega(x,y) = 1/sqrt(x^2+y^2)
               1        constant, omega(x,y) = 1
               G:#      Gaussian, where # specifies sigma,
                        omega(x,y) = exp(-(x^2+y^2)/(2 sigma^2))
  -m <method>  method to use for fast computation, choices are
               interp:# interpolate s_a(L - I(x)) with # levels
               poly:#   polynomial s_a with degree #

  -q <number>  quality for saving JPEG images (0 to 100)


--- histeq ---

Usage: histeq [options] input output

Options:
   -b <number>     number of histogram bins (default 256)
   -q <number>     quality for saving JPEG images (0 to 100)


Example:

    # Perform ACE automatic color enhancement on avs.jpg with alpha = 8
    ./ace -a 8 -m interp:12 avs.jpg ace.bmp

    # Perform uniform histogram equalization
    ./histeq avs.jpg equalized.bmp

Note: If the programs are compiled without libjpeg support, please convert 
avs.jpg to avs.bmp in Windows Bitmap BMP format and use this as the input.

Each of these programs prints detailed usage information when executed 
without arguments or "--help".


== Compiling ==

Instructions are included below for compiling on Linux sytems with GCC, on
Windows with MinGW+MSYS, and on Windows with MSVC.

Compiling requires the FFTW3 Fourier transform library (http://www.fftw.org/).
For supporting additional image formats, the programs can optionally be
compiled with libjpeg, libpng, and/or libtiff.  Windows BMP images are always
supported.


== Compiling (Linux) ==

To compile this software under Linux, first install the development files for
libfftw, libjpeg, libpng, and libtiff.  On Ubuntu and other Debian-based 
systems, enter the following into a terminal:
    sudo apt-get install build-essential libfftw3-dev libjpeg8-dev libpng-dev libtiff-dev
On Redhat, Fedora, and CentOS, use
    sudo yum install make gcc libfftw-devel libjpeg-turbo-devel libpng-devel libtiff-devel

Then to compile the software, use make with makefile.gcc:

    tar -xf ace_20121029.tar.gz
    cd ace_20121029
    make -f makefile.gcc

This should produce two executables, ace and histeq.

Source documentation can be generated with Doxygen (www.doxygen.org).

    make -f makefile.gcc srcdoc


== Compiling (Windows) ==

The MinGW+MSYS is a convenient toolchain for Linux-like development under 
Windows.  MinGW and MSYS can be obtained from

    http://downloads.sourceforge.net/mingw/

The FFTW3 library is needed to compile the programs.  FFTW3 can be 
obtained from

    http://www.fftw.org/

Instructions for building FFTW3 with MinGW+MSYS can be found at

    http://www.fftw.org/install/windows.html
    http://neuroimaging.scipy.org/doc/manual/html/devel/install/windows_scipy_build.html


--- Building with BMP only ---

The simplest way to build the tvdeconv programs is with support for only BMP
images.  In this case, only the FFTW3 library is required.  Edit makefile.gcc 
and comment the LDLIB lines to disable use of libjpeg, libpng, and libtiff:

    #LDLIBJPEG=-ljpeg
    #LDLIBPNG=-lpng -lz
    #LDLIBTIFF=-ltiff

Then open an MSYS terminal and compile the program with 

    make CC=gcc -f makefile.gcc

This should produce the executables ace and histeq.


--- Building with PNG, JPEG, and/or TIFF support ---

To use the tvdeconv program with PNG, JPEG, and/or TIFF images, the 
following libraries are needed.

    For PNG:    libpng and zlib
    For JPEG:   libjpeg 
    For TIFF:   libtiff

These libraries can be obtained at 
    
    http://www.libpng.org/pub/png/libpng.html
    http://www.zlib.net/
    http://www.ijg.org/
    http://www.remotesensing.org/libtiff/

It is not necessary to include support for all of these libraries, for 
example, you may choose to support only PNG by building zlib and libpng 
and commenting the LDLIBJPEG and LDLIBTIF lines in makefile.gcc.

Instructions for how to build the libraries with MinGW+MSYS are provided at

    http://permalink.gmane.org/gmane.comp.graphics.panotools.devel/103
    http://www.gaia-gis.it/spatialite-2.4.0/mingw_how_to.html

Once the libraries are installed, build the tvdeconv programs with the 
makefile.gcc included in this archive.

    make CC=gcc -f makefile.gcc

This should produce two executables, ace and histeq.


== Compiling (Mac OSX) ==

The following instructions are untested and may require adaptation, but
hopefully they provide something in the right direction.

First, install the XCode developer tools.  One way to do this is from 
the OSX install disc, in which there is a folder of optional installs 
including a package for XCode.

The program requires the free FFTW library to build.  Optionally, it can
also use the libpng, libjpeg, and libtiff libraries to support more image
formats.  These libraries can be obtained on Mac OSX from the Fink project 

    http://www.finkproject.org/

Go to the Download -> Quick Start page for instructions on how to get 
started with Fink.  The Fink Commander program may then be used to download
and install the packages fftw, libpng, libjpeg, and libtiff.  It may be 
necessary to install fftw-dev, libpng-dev, libjpeg-dev, and libtiff-dev as 
well.

Once the libraries are installed, compile the program using the included
makefile "makefile.gcc":

    make -f makefile.gcc

This should produce the executables ace and histeq.


== Acknowledgements ==

This material is based upon work supported by the National Science 
Foundation under Award No. DMS-1004694.  Any opinions, findings, and 
conclusions or recommendations expressed in this material are those of 
the author(s) and do not necessarily reflect the views of the National
Science Foundation.
