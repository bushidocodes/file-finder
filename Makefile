
file-finder: main.c worker.c shell.c dumper.c
	gcc -flto -O3 -pthread -o $@ $^

clean:
	rm file-finder