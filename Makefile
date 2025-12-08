

mandelbrot_set: ./mandelbrot_set.c
	gcc mandelbrot_set.c -o mandelbrot_set -lm

clean:
	rm -rf ./mandelbrot_set
