

all: schedule

schedule: schedule.c
	gcc -o schedule schedule.c -lm -lpthread

clean:
	rm -fr schedule *dSYM
