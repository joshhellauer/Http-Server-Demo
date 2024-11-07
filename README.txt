/****************************************************
*****************************************************/
What is this repo?
It includes 4 versions of an HTTP server:
server_proc.c: a process-driven concurrent HTTP server. 
server_thread.c: a threaded concurrent HTTP server
server_cached.c: a threaded concurrent HTTP server that uses a 
                 linked-list with reference counting to cache
                 responses
server_cached_naive.c: another threaded, concurrent HTTP server
                       it is "naive" because the mutual exclusion
                       practiced on the cache data structure is sub-optimal.


Compilation instructions:

1. Run the Makefile with `make`

Important:

- I included two versions of a cached HTTP server.

The first, 'server_cached.c' uses a linked list with reference counting 
to store cached responses. The reference counting allows for true 
concurrency as multiple threads can be sending cached data at the same time.

The second version, 'server_cached_naive.c' uses a PriorityQueue to
cache responses. However, this version is 'naive' in that a mutex is
locked before the cache is searched, and if a response if found, the
mutex is not unlocked until the worker thread has finished sending
the cached response. IOW, only one worker thread must finish sending the cached
data before another thread can even search the cache. I would have expected this
to be slower than the 'server_cached.c' program. But experimental results
surprised me. 

 /***************************************************
*****************************************************/
