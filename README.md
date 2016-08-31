# touch-paint

touch-paint is a multi-touch based paint tool for Linux

## Dependencies

- libXi
- cmake

You could get libXi by yum command on CentOS/Fedora/REHL:

    # yum install libXi-devel cmake

For apt-get on Debian/Ubuntu:

    # apt-get install libXi-dev cmake

## Builds

    $ cd src
    $ cmake .
    $ make
