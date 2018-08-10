TermySequence
=============

This is the TermySequence source distribution.

Product home page: https://termysequence.io

Copyright &copy; 2018 TermySequence LLC. Refer to [COPYING.txt](COPYING.txt) for license terms.

## Platforms

  * Linux x64

## Dependencies
### Common

  * cmake 3.9.0+ (build only)
  * libuuid (usually part of the base system)

### Server

  * systemd 233+ (optional)
  * libgit2 0.26.0+ (optional, dynamically loaded at runtime)

### Client

  * qt5 5.9.2+ (core, gui, and svg)
  * sqlite 3.8.0+
  * zlib 1.2.5+
  * libfuse 3.1+ or 2.9.7+ (optional)

### Tests and Utilities

  * cmocka 1.1.1+
  * libxml2

## Build and Install Instructions

See [Building from Source](https://termysequence.io/doc/build.html) in termy-doc.

## Repository Layout

  * __mux__: server source tree
  * __mux/i18n__: server translation files
  * __mux/scripts__: programs that ship with server (imgcat, etc)
  * __mon__: monitor source tree
  * __src__: client source tree
  * __src/dist__: client plugins, images, and other content
  * __src/i18n__: client translation files
  * __lib__: common protocol library
  * __os__: common os library
  * __doc__: man pages
  * __test__: tests
  * __util__: maintainer tools
  * __vendor__: third party code and content
