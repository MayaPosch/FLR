# FLR #

**Author:** Maya Posch

**Date:** 2018/01/27

## Overview ##

FLR is a file revision control system, with a strong focus on file integrity. At this point it's still in an early Alpha stage, serving primarily as a demonstration project for the NymphRPC remote procedure call library ([https://github.com/MayaPosch/NymphRPC](https://github.com/MayaPosch/NymphRPC "NymphRPC @ GitHub") ).

## Building ##

In addition to the NymphRPC dependency, FLR requires `liblzma` for LZMA compression.

The following Make targets are available from the top folder:

`make client` - Build just the client.

`make server` - Build just the server.

After building, the binaries can be found in a newly created `bin` folder.

## Running ##

The FLR server looks for a `config.ini` file in the same folder. This INI file has the following structure:

    [NymphRPC]
	timeout = 5000
	port = 4004

The timeout is for client connections. The port is where the server listens on. Currently the server listens on all network interfaces. By default the FLR server listens on port 4004.

The FLR client allows one to specify the host and port to connect to via the following commandline options:

	$ ./flrclient -s <server name/IP> -p <port>

By default the FLR client tries to connect to `localhost`, port 4004.

## Current status ##

Keeping in mind that FLR is still early Alpha-level quality, it offers the following features:

- Central server-based architecture for managing file collections.
- Small CLI-based client for communicating with said server over a binary protocol.
- User authentication.
- SQLite-based backend.
- Checking in of new file data.