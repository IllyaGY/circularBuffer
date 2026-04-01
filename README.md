# Requires ncurses

## Build and run

### Array Version
```bash
gcc -DTYPE=1 -g run.c buffer_api.c buffer_array.c buffer_list.c -o array.out -lncurses
./array.out
```

### Linked List Version
```bash
gcc -DTYPE=0 -g run.c buffer_api.c buffer_array.c buffer_list.c -o list.out -lncurses
./list.out
```

### Test Case Options - works for both implementations
```bash
gcc -DTEST_CASE=0 #Fully filled buffer
gcc -DTEST_CASE=1 #Half-filled up buffer
gcc -DTEST_CASE=2 #Any other number - empty buffer
```


### Size

```bash
gcc -DSIZE=10 #Size
```


### Controls

i - push an element
r - pop an element
d - destroy the buffer
