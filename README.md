# Sony PTP Camera Client #

The Sony PTP Camera Client allows connecting to Sony cameras in Remote Control mode (currently tested on the Sony Alpha 6000). The client also includes a CPython module allowing camera control from Python.

## Supported operations ##

* Polling camera parameters
* Setting camera parameters
* Taking pictures
* Transferring pictures (device to host)

## Prerequisits ##

* Git
* gcc
* libpthread
* libusb-1.0 development files
* Python 2.7 development files

## Installation ##

The following steps describe the installation process under Ubuntu 15.10 (Wily).
The manual assumes gcc and libpthread are already installed (as in Ubuntu).

### Install Git ###

    sudo apt-get install git

### Clone the repository ###

Go to a directory of your choice and clone the repository:

    cd ~/projects
    git clone https://bitbucket.org/rftech/ptpclient.git

### Install libusb ###

Make sure to install libusb-1.0, do not confuse it with libusb-0.1:

    sudo apt-get install libusb-1.0-dev

### Install Python ###

    sudo apt-get install python2.7-dev

### Build the project ###

Go to the project's directory and run `make` as usual:

    make

This should create an executable named *ptpclient* and a shared library named *pyptp.so* in the project's directory.
To build the application only, use `make ptpclient`.
To build the python module only, use `make pyptp`.


## Usage ##

Just run `ptpclient`:

    ./ptpclient

The program has no command-line options - it is intended to be used as a part of the AUVSI Airborne Platform, and currently all options can only be changed from the code.

To terminate the program early, use Ctrl+C. If pictures are being transferred, the transfer will continue until the camera's buffer is depleted.

To use the Python module, just use `import pyptp`. See [ptpclient.py](ptpclient.py) for sample code.

## Project general structure ##

File           | Description
-------------- | -------------------------------------------------------------------
*client.c*     | Main source file containing the program entry point.
*ptp.c*        | PTP over USB transport implementation.
*ptp-pima.c*   | PIMA 15740:2000 PTP minimal implementation using PTP/USB transport.
*ptp-sony.c*   | Sony PTP Vendor extensions implementation.
*usb.c*        | libusb-1.0 helper/wrapper implementing async API event loop.
*timer.c*      | Simple timer block for timing various operations.
*pyptp.c*      | Python PTP client wrapper module
*ptpclient.py* | Python module usage sample

## External references ##
* [PIMA 15740:2000](people.ece.cornell.edu/land/courses/ece4760/FinalProjects/f2012/jmv87/site/files/pima15740-2000.pdf)
* [PTP transport over USB specifications](http://www.usb.org/developers/docs/devclass_docs/usb_still_img10.zip)
* [Picture Transfer Protocol](https://en.wikipedia.org/wiki/Picture_Transfer_Protocol)
* [libusb-1.0 API Reference](http://libusb.sourceforge.net/api-1.0/)
* [CPython Extension API Reference](https://docs.python.org/2/extending/index.html)