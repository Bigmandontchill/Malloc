Implmentation of malloc using c

Run command: first command:gcc -o m main.c
             
             second command:m 
             
             
Our implementation keeps a linklist. The program traverse the linked list looking for a block with the size requested by the user.  The program split the block into two blocks if the block is larger then the user requested.Program return the block with the requested size to the user. The program uses sbrk to get memory from operating system if the program  can't find a block on the list.
