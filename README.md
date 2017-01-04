# sysexd

This daemon allows applications to communicate SysEx messages over MIDI using WebSockets.

As such, it opens a WebSockets-enabled HTTP server on a predefined port (e.g. 9002) on the local host, on which the host's MIDI interfaces are exposed.

## Why use a daemon?

Although the [Web MIDI API](https://www.w3.org/TR/webmidi/) is currently under development for browsers, there a number of situations where a daemon may have advantages:

* Broader browser compatibility, e.g. including browsers that do not support the Web MIDI API.
* Lower-level implementation, e.g. to support SysEx communication with devices that do not behave according to MIDI SysEx specifications.
* Isolation, e.g. keeping low-level SysEx communication related code, such as data corruption and message retries, out of the client codebase.

## Examples

An example of use for sysexd can be found at [wer.si](http://wer.si)

