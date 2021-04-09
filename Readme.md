Disk Simulation

The software simulates reading and writing to disk.
The virtual disk is at size 256 (can be changed).
The data of the disk is stored in a file in this folder.
 

functaions (in order to call them insert the function number):
0. exit the program.
1. listALL - return info about all the files in the system and also the data on the disk/
2. format - formating the disk (block size and direct entries).
3. createFile - creating new file (insert name).
4. openFile - open file (insert name).
5. closeFile - close file (insert fd).
6. writeToFile - writing to a file (insert fd and string).
7. readFromFile - read from file (insert fd and numbers of chars to read).
8. delFile - delete file from disk (insert name).



How to compile:
In VSCode ctrl+shift+b.
In linux: g++ main.cpp -o main.

How to run:
In VSCode ctrl+f5.
In linux: open the terminal in the folder and write "./main" and press enter.

Program files:
makefile
DISK_SIM_FILE.txt // disk data file
main.cpp // main
main    // Executable file
