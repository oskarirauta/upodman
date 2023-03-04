# upodman

upodman extends ubus with bindings to podman
and allows controlling and statistics over pods
and containers through ubus.

upodman is written in C++ except for parts which
have been externed from C.

### Use cases
This software can be used as a "remote controller" to
podman. Podman already comes with quite large REST API
but for my purposes I needed to have some changes in
resulting structure and in the way that things are done.
Also, some of actions should be executed in background
as there always is not enough time until receiving
process times out, so that's what this program mostly
does.

One potential use case would be container management
in LuCi. LuCi already has a quite comprehensive add-on
for docker which I ported to support podman (all but
load and ram stats that caused some unexpected difficulties)
for most parts and added POD support. Still, I think
that full container management is best done in a terminal
with provided tools and I loved how it was done with
Cockpit, so I started to cook something of my own and
from that idea, upodman was born.

### Is upodman a replacement for REST API over socket?
Yes and no, it can be used instead of socket, but upodman
does not have even nearly all features available through
socket. And it most certainly will never have, API is
changing nearly on every release update and it would
be painful to update it all and software would be
constantly outdated to expose full API; Also it is
unnecessary, for full features, use REST API through
socket or command-line tool. upodman is meant to provide
basic statistics and basic commands that can be used
to provide a nice simple podman commander where administrator
makes more significant changes to system with feature-full
tools and uses upodman to monitor and do very simple
management for containers.

### Features
 - info: podman info
 - networks: listing
 - pods: start, stop, restart
 - containers: start, stop, restart
 - running: see if container is running
 - pod & container list with statistics
 - logs

### Calls
 - list
 - networks
 - exec { .group = pod/container, .name/.id = pod/container identification, .action = stop,start,restart }
 - running { .name/.id = container identification }
 - list
 - logs { .name/.id = container identification }

### Status
work-in-progress

### Requirements/compatibility
 - openwrt
 - openwrt variants with ubus (untested)

provided Makefile is for native build purposes.

### Work in progress
 - extend capabilities of software
 - expose more features over ubus (logs for example)
 - luci

### Anything else?
Podman code can be used for other purposes as well.
It is broken down to sub-directory and comes with example.

upodman was previously included in my other very similar
project systembus but I decided to break it down into it's own.

### Building
```make```

author: Oskari Rauta
license: MIT
