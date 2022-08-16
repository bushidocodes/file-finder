
file-finder: main.cpp worker.cpp shell.cpp dumper.cpp
	clang++-15 -std=c++20 -stdlib=libc++ -pthread -o $@ $^

clean:
	rm file-finder