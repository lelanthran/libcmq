# libcmq
A simple thread-safe message queue for C

## Overview
Implementation of a very simple thread-safe queue. The queue stores pointers
to messages of any type; it does not keep a copies of messages. In short, if
the message was `malloc()`ed before put into the queue, the caller should
`free()` the message when removing it from the queue.

A new queue is created with `cmq_new()`, messages are placed into the queue
using `cmq_nq()`, messages are removed from the queue using `cmq_dq()` and the
queue can be destroyed using `cmq_del()`.

Destroying a queue which has messages will cause the messages in the queue to
be discarded.

## Usage
See the header file for usage of each function. See the `cmq_test.c` file for
an example of how the queue can be used.
```
// Create a queue
cmq_t *my_queue = cmq_new ();
if (!my_queue) {
   // Handle error
}
```
```
// Insert elements into the queue
if (!(cmq_nq (my_queue, &src_data, sizeof src_data))) {
   // Handle error
}
```
```
// Remove elements from the queue; if the queue
// is empty, we want to wait 5s for a message to
// arrive
if (!(cmq_dq (my_queue, &dst_data, sizeof dst_data, 5))) {
   // Handle error
} else {
   // Use dst_data
}
...
```
```
// Finally, delete the queue after we are done with it.
cmq_del (my_queue);
```

## Build
Only tested on Linux and Windows. Will maybe port to BSD and macOS depending on
whether I have the time or not. Note that porting to macOS might be problematic
due to macOS not supporting the optional semaphore functions from POSIX.

After building `debug` or `release` the library files (`.so` on Linux, `.dll`
on Windows) will be in a debug or release directory respectively. An
executable containing all the tests will be in a `bin/` directory within the
debug/release directory.

### Linux
On Linux type `make debug` or `make release` to generate debug or release
builds respectively. To clean, use one of `make clean-debug`, `make
clean-release` or `make clean-all`.

### Windows
On Windows, I build it using git-bash as the shell with mingw64 in the PATH
environment variable. The same make targets are used, but be sure to use
`mingw32-make` and not `make` when in the git-bash shell.

## BUGS
Probably. Threaded code is notoriously difficult to get correct and this code
does not even attempt deadlock detection, nevermind deadlock avoidance.

A good-faith and reasonable test was run to attempt to find deadlocks but,
as OJ's lawyer once said, nothing can be proven.

## Still on the cards
- Maybe remove the length requirement for messages? Callers might use it only
   when storing raw buffers. Mostly callers store struct types in a queue.
- BSD testing, obviously.
- Maybe some code-butchering to make macOS work.
- Queue persistence (write queue out to disk and read queue back in from disk).
- Collection of statistics/counters:
   - How much time was spent waiting on locks
   - How much time was spent waiting for messages
   - The largest the queue ever got
   - The average size of elements over the lifetime of the queue
   - The number of insertions over the lifetime of the queue
