#!/bin/sh

make clean
make 
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes ./utests
