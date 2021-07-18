# WeMo Smart Plug Control - PHP

## Description

Daemon for controlling [WEMO](https://www.wemo.com/products) Mini Smart Plugs. The daemon will turn different plugs on/off on a user specified schedule that is configured via an `ini`-file.

## Usage

The daemon is invoked via

```shell
./wemod.php &
```

Below is a `wemo.ini` example configuration file.

```INI
# network interface to use
interface=eth1

[Lamp]
daily=true
ontimes=7:00,12:10
offtimes=8:30,22:00

[Christmas Lights]
daily=true
ontimes=7:00,12:10
offtimes=8:30,22:00
```

Each plug has its own section that is identified by its name, where the `daily` key is either set to true or false to indicate a daily schedule, and the `ontimes` and `offtimes` keys are comma-separated lists of times, expressed using 24-hour notation, to turn a given plug on and off, respectively. The `interface` key that has no section controls the network interface to be used.

The daemon responds to the `SIGUSR1` and `SIGUSR2` signal, where the former forces a re-scan and the latter writes a summary of the daemon's state and the registered timers to `wemo.log`.

## Notes

1. Enabling certain functionality via `php.ini` might be required or could simply not be available on a particular system. For example, the `pcntl_*` family of functions are not available on systems that do not implement the underlying C function, e.g., MacOS and FreeBSD.

## BSD-3 License

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
