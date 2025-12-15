 /**************************/
 /* Alfred Roos 2025-12-07 */
 /* Mandelbrot set				 */
 /* Provided as is				 */
 /**************************/

#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#define THREAD_COUNT 100

int kbhit(void);
unsigned int termWidth();
unsigned int termHeight();
struct winsize win;


double* xcoords;
double* ycoords;

double xmin = -2.0;
double xmax = 1.0;
double ymin = -1.5;
double ymax = 1.5;

int width = 900;
int height = 400;

struct pixel{
	int r;
	int g;
	int b;
};

void setCursorPosition(int x, int y) { 
	printf("\033[%d;%dH", y+1, x+1);
}

int iterations_for_zoom(double initial_width, double current_width)
{
    double zoom = initial_width / current_width;
    if (zoom < 1.0) zoom = 1.0;

    double level = log10(zoom);
    if (level < 0.0) level = 0.0;

    double base  = 150.0;
    double scale = 80.0;

    double it = base + scale * powf(level, 1.5);

    if (it > 2000000.0) it = 2000000.0;

    return (int)it;
}

struct pixel render_pixel(int px, int py)
{

	double x0 = xcoords[px] * (xmax - xmin) + xmin;
	double y0 = ycoords[py] * (ymax - ymin) + ymin;

	double x = 0;
	double y = 0;
				
	int iteration = 0;
	int max_iteration = iterations_for_zoom(3,xmax-xmin);
	while(x*x + y*y < 4 && iteration < max_iteration){
		double xtemp = x*x - y*y + x0;
		y = 2*x*y + y0;
		x = xtemp;
		iteration++;
	}


	double t = (double)iteration / (double)max_iteration;
	double it = 1.0 - t;
	double it2 = it*it;
	double it3 = it2*it;
	double t2 = t*t;
	// double t3 = t2*t; // if needed

	double r = 255.0* (1.0 - it3);
	double g = 255.0 * t2;
	double b = 40.0  * t;

	struct pixel pixel;
	pixel.r = r;
	pixel.g = g;
	pixel.b = b;
	return pixel;
}

struct arg{
	int start_y;
	int end_y;
	struct pixel* data;
};

void* work(void* _arg) {
	struct arg* arg = (struct arg*) _arg;
	
	for(int y = arg->start_y; y < arg->end_y; y ++){
		for(int x = 0; x < width; x ++){
			arg->data[y*width+x] = render_pixel(x,y);
  	}

	}
	return NULL;
}



int render(){

	
	pthread_t threads[THREAD_COUNT];
	struct arg args[THREAD_COUNT];

	struct pixel* data = malloc(sizeof(struct pixel)*width*height);
	
	int rows_per_thread = height / THREAD_COUNT;

	
	for(int i = 0; i < THREAD_COUNT; i ++){
		args[i].start_y = i*rows_per_thread;
		args[i].end_y = (i == THREAD_COUNT - 1) ? height : (i + 1) * rows_per_thread;
		args[i].data = data;
		pthread_create(&threads[i], NULL, work, &args[i]);
	}
	
	for(int i = 0; i < THREAD_COUNT; i ++){
		pthread_join(threads[i],NULL);
	}

	for(int y = 0; y < height; y ++){


		char rowbuf[width * 30];  // enough for large width
		char* p = rowbuf;

		for (int x = 0; x < width; x++) {
			struct pixel px = data[y * width + x];
			p += sprintf(p, "\033[48;2;%d;%d;%dm ", px.r, px.g, px.b);
		}

		// Add newline
		*p++ = '\n';

		
		fwrite(rowbuf, 1, p - rowbuf, stdout);

	}
	
	return 0;
}

int main() {

	width = termWidth()-2;
	height = termHeight()-2;
	
	xcoords = malloc(width * sizeof(double));
	ycoords = malloc(height * sizeof(double));

	for (int px = 0; px < width; px++)
    xcoords[px] = (px / (double)width);

	for (int py = 0; py < height; py++)
    ycoords[py] = (py / (double)height);

	
	while(1){
		render();
		setCursorPosition(0,0);
		//		usleep(10000);
		if(kbhit() == 1){
			char key = getchar();
			double xRange = xmax - xmin;
			double yRange = ymax - ymin;

			// Zoom factor (percentage of the range)
			double zoomFactor = 0.1;  // 10% zoom in/out

			if(key == '+' || key == '='){
				xmin += xRange * zoomFactor;
				xmax -= xRange * zoomFactor;
				ymin += yRange * zoomFactor;
				ymax -= yRange * zoomFactor;
			}
			if(key == '-'){
				xmin -= xRange * zoomFactor;
				xmax += xRange * zoomFactor;
				ymin -= yRange * zoomFactor;
				ymax += yRange * zoomFactor;
			}

			// Pan factor (percentage of the range)
			double panFactor = 0.1;  // 10% of current range

			if(key == 'a'){ // left
				xmin -= xRange * panFactor;
				xmax -= xRange * panFactor;
			}
			if(key == 'd'){ // right
				xmin += xRange * panFactor;
				xmax += xRange * panFactor;
			}
			if(key == 'w'){ // up
				ymin -= yRange * panFactor;
				ymax -= yRange * panFactor;
			}
			if(key == 's'){ // down
				ymin += yRange * panFactor;
				ymax += yRange * panFactor;
			}	 		
		}
	}

	return 0;
}


unsigned int termWidth(){
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);
    return (win.ws_col);
}
unsigned int termHeight(){
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &win);
    return (win.ws_row);
}

int kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF)
  {
      ungetc(ch, stdin);
      return 1;
  }

  return 0;
}
