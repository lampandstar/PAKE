CC:=gcc
INC:=-I./ -I./avx_bn/ -I./crypto/ -I./srp/ -I./include/
SRC:=avx_bn/*.c crypto/*.c srp/*.c
ARCH:=-march=native -mavx
FLAGS:=-fPIC -shared
#TARGET:= ./test
TARGET:=srp.so
all:
	$(CC) $(FLAGS) $(SRC) $(INC) -o $(TARGET) $(ARCH) -O3
	#$(CC) -fPIC -shared $(SRC) $(INC) -o $(TARGET) $(ARCH) -O3