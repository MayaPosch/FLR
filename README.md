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

## Current status ##

Keeping in mind that FLR is still early Alpha-level quality, it offers the following features:

- Central server-based architecture for managing file collections.
- Small CLI-based client for communicating with said server over a binary protocol.
- User authentication.
- SQLite-based backend.
- Checking in of new file data.