Requirements
----

If you want to be an effective developer, you must have the following packages installed:

 * jansson (2.6 or later) - http://www.digip.org/jansson/releases/jansson-2.6.tar.bz2
 * linux (kernel 2.6.25 or later)

Build and install these the usual way (`./configure`, `make`, `make install` for most libraries).

Build Types
----
To set the build type, set the `BUILD_TYPE` variable. For example, if you want to make a release
build of the system, set `BUILD_TYPE` to `release`. The target types are defined in the build/
directory of the source tree.

The default build type is `debug`.

Useful Environmental Notes
----

To run TSL apps as configured by default, you will need huge pages enabled on your
system. To do this, create a pool of system hugepages:

`echo -n 32 > /proc/sys/vm/nr_hugepages`

This will create 32 hugepages that can be used by the system. Production systems will probably
have more hugepages allocated than 32.

Building the System
----

If you have all the dependencies installed (compiler, mostly), you should be good to go. Just
type

`make`

in the top level directory.
