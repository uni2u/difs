# Installing difs

## Prerequisites

* [ndn-cxx and its dependencies](https://named-data.net/doc/ndn-cxx/current/INSTALL.html)
* sqlite3

## Build

To build in a terminal, change to the directory containing the difs repository.
Then enter:

    ./waf configure
    ./waf
    sudo ./waf install

This builds and installs `ndn-difs` and related tools.

If configured with tests (`./waf configure --with-tests`), the above commands will
also generate unit tests that can be run with `./build/unit-tests`.

## Configuration

The default configuration file path is `/usr/local/etc/ndn/difs.conf`.
Users may copy the [difs.conf.sample](difs.conf.sample) config sample to that path.

## Database

The database path is set in the `storage.path` key of the configuration file.
The default database path is `/var/lib/ndn/difs`.

difs will automatically create a database if one does not exist.

Users should make sure that the `ndn-difs` process has write access to the
database directory.

## Tools

Currently, three tools are included: *ndngetfile*, *ndnputfile*, and *repo-ng-ls*.
Users can find detailed information about these tools on the
[repo-ng wiki](https://redmine.named-data.net/projects/repo-ng/wiki/Tools).
