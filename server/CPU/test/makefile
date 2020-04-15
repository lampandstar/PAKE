CC:=gcc
MAIN:=./test_bn.c
INC:=-I../ -I../avx_bn/ -I../crypto/ -I../srp/ -I../include/
SRC:=../avx_bn/*.c ../crypto/*.c ../srp/*.c $(MAIN)
ARCH:=-march=native -mavx
FLAGS:=-fPIC -shared
TARGET:= ./test
all:
	$(CC) $(SRC) $(INC) -o $(TARGET) $(ARCH) -O3