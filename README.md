# sysexd

This daemon allows applications to communicate SysEx messages over MIDI using WebSockets.

As such, it opens a WebSockets-enabled HTTP server on a predefined port (e.g. 9002) on the local host, on which the host's MIDI interfaces are exposed.

## Installation instructions

### Windows

If you don't feel like building sysexd, head over to https://github.com/ijsf/sysexd/releases to download the latest binary and just run it.

### OSX/*nix

For these platforms, it is required to build sysexd. First, make sure you have the following already installed:

* Boost - e.g. `brew install boost` on OSX using Homebrew.

Now generate the makefiles using CMake:

    cmake .

If no errors have occurred, build sysexd:

    make

If the build was successful, you should now be able to run sysexd as follows:

    ./bin/sysexd

## Why use a daemon?

Although the [Web MIDI API](https://www.w3.org/TR/webmidi/) is currently under development for browsers, there a number of situations where a daemon may have advantages:

* Broader browser compatibility, e.g. including browsers that do not support the Web MIDI API.
* Lower-level implementation, e.g. to support SysEx communication with devices that do not behave according to MIDI SysEx specifications.
* Isolation, e.g. keeping low-level SysEx communication related code, such as data corruption and message retries, out of the client codebase.

## Examples

An example of use for sysexd can be found at [wer.si](http://wer.si).

