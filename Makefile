all: server_proc server_thread server_cached server_cached_naive

flags="-Wall"

server_proc: server_proc.c
	gcc $(flags) -o server_proc server_proc.c -pthread

server_thread: server_thread.c
	gcc $(flags) -o server_thread server_thread.c -pthread

server_cached: server_cached.c
	gcc $(flags) -o server_cached server_cached.c -pthread

server_cached_naive: server_cached_naive.c PriorityQueue.c
	gcc $(flags) -o server_cached_naive server_cached_naive.c PriorityQueue.c -pthread
