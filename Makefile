
file-finder: main.cpp worker.cpp shell.cpp dumper.cpp
	clang++-14 -std=c++20 -g3 -O0 -o $@ $^

clean:
	rm file-finder