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


float* xcoords;
float* ycoords;

float xmin = -2.0;
float xmax = 1.0;
float ymin = -1.5;
float ymax = 1.5;

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

int iterations_for_zoom(float initial_width, float current_width)
{
    float zoom = initial_width / current_width;
    if (zoom < 1.0f) zoom = 1.0f;

    float level = log10f(zoom)*2;
    if (level < 0.0f) level = 0.0f;

    float base  = 50.0f;
    float scale = 80.0f;

    float it = base + scale * powf(level, 1.5f);

    if (it > 2000000.0f) it = 2000000.0f;

    return (int)it;
}

struct pixel render_pixel(int px, int py)
{

	float x0 = xcoords[px] * (xmax - xmin) + xmin;
	float y0 = ycoords[py] * (ymax - ymin) + ymin;

	float x = 0;
	float y = 0;
				
	int iteration = 0;
	int max_iteration = iterations_for_zoom(3,xmax-xmin);
	while(x*x + y*y < 4 && iteration < max_iteration){
		float xtemp = x*x - y*y + x0;
		y = 2*x*y + y0;
		x = xtemp;
		iteration++;
	}


	float t = (float)iteration / (float)max_iteration;
	float it = 1.0f - t;
	float it2 = it*it;
	float it3 = it2*it;
	float t2 = t*t;
	// float t3 = t2*t; // if needed

	float r = 255.0f * (1.0f - it3);
	float g = 255.0f * t2;
	float b = 40.0f  * t;

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
	
	xcoords = malloc(width * sizeof(float));
	ycoords = malloc(height * sizeof(float));

	for (int px = 0; px < width; px++)
    xcoords[px] = (px / (float)width);

	for (int py = 0; py < height; py++)
    ycoords[py] = (py / (float)height);

	
	while(1){
		render();
		setCursorPosition(0,0);
		//		usleep(10000);
		if(kbhit() == 1){
			char key = getchar();
			float xRange = xmax - xmin;
			float yRange = ymax - ymin;

			// Zoom factor (percentage of the range)
			float zoomFactor = 0.1;  // 10% zoom in/out

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
			float panFactor = 0.1;  // 10% of current range

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
