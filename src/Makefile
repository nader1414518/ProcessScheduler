build:
	gcc test_generator.c -o test_generator.out
	gcc process_generator.c -o process_generator.out
	gcc clk.c -o clk.out
	gcc scheduler.c -o scheduler.out
	gcc rr_scheduler.c -o rr_scheduler.out -lrt
	gcc process.c -o process.out
	gcc processN.c -o processN.out
clean:
	rm -f *.out  processes.txt

all: clean build

run:
	./process_generator.out
