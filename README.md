# file-finder

A multi-threaded command-line tool that searches a directory tree for files whose names contain one or more substrings, reporting matches interactively as they are found.

## Building

Requires GCC and POSIX pthreads (Linux / macOS).

```
make
```

## Usage

```
./file-finder <directory> <substring> [<substring2> ...]
```

Each substring gets its own worker thread searching the tree concurrently. Once running, the tool drops into an interactive shell:

| Command | Effect |
|---------|--------|
| `dump`  | Print all matches found so far, then clear the buffer |
| `exit`  | Exit the program |

Results are also flushed automatically every second.

### Example

```
$ ./file-finder /usr/include pthread mutex
>> dump
pthread.h
pthread_mutex_destroy.3.gz
pthread_mutex_init.3.gz
pthread_mutex_lock.3.gz
>> exit
```

## Architecture

| Component | File | Role |
|-----------|------|------|
| Main      | `main.c` | Parses args, spawns threads, joins on shell exit |
| Worker    | `worker.c` | One thread per substring; recursively walks the directory tree |
| Dumper    | `dumper.c` | Background thread that flushes matches to stdout every second |
| Shell     | `shell.c` | Reads `dump` / `exit` commands from stdin |
| Buffer    | `con_str_vec.h` | Thread-safe growable string vector shared by all components |
