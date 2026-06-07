# file-finder

A multi-threaded command-line tool that searches a directory tree for files whose names contain one or more substrings, reporting matches interactively as they are found.

## Building and testing

Requires GCC and POSIX pthreads (Linux / macOS).

```
make          # release binary
make test     # run unit tests (Unity)
make debug    # ThreadSanitizer build → file-finder.debug
make install  # copy to $(PREFIX)/bin  (default: /usr/local)
```

## Usage

```
./file-finder <directory> <substring> [<substring2> ...]
```

A single worker thread traverses the directory tree once, checking each filename against all substrings in one pass. Once running, the tool drops into an interactive shell:

| Command | Effect |
|---------|--------|
| `dump`  | Print all matches found so far, then clear the buffer |
| `exit`  | Exit the program |

Results are also flushed automatically every second.

### Example

```
$ ./file-finder /usr/include pthread mutex
>> dump
/usr/include/pthread.h
/usr/include/boost/thread/pthread/pthread_mutex_scoped_lock.hpp
/usr/include/x86_64-linux-gnu/bits/pthreadtypes.h
>> exit
```

## Architecture

```
file-finder/
├── include/
│   ├── con_str_vec.h   thread-safe growable string vector
│   ├── globals.h       shared application state (extern declarations)
│   ├── worker.h
│   ├── dumper.h
│   └── shell.h
├── src/
│   ├── main.c          argument parsing, thread lifecycle
│   ├── worker.c        recursive directory traversal and substring matching
│   ├── dumper.c        periodic background flush of matches to stdout
│   └── shell.c         interactive dump/exit command loop
└── Makefile
```

| Component | Role |
|-----------|------|
| `src/main.c` | Parses args, spawns threads, joins on shell exit |
| `src/worker.c` | Single traversal of the directory tree; checks each filename against all substrings |
| `src/dumper.c` | Background thread that flushes matches to stdout every second |
| `src/shell.c` | Reads `dump` / `exit` commands from stdin |
| `include/con_str_vec.h` | Thread-safe growable string vector shared by all components |
