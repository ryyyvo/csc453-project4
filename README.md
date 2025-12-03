Ishwarya Namburu, Ryan Vo

Our TinyFS seems to be working well. We chose to do a linked list implementation for our free blocks. We also decided to design our inode block to look like: byte 0-3 as required, bytes4-11 file name, and byte 12-13 as file size. Initially we were using functions such as fopen() which gave us some difficulties, but after switching to functions like open() we found that the disk emulator worked better.
We were unable to add additional functionality.
