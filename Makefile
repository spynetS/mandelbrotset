

mandelbrot_set: ./mandelbrot_set.c
	gcc -O3 mandelbrot_set.c -o mandelbrot_set -lm

clean:
	rm -rf ./mandelbrot_set
