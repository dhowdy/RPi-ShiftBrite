#include "shiftbrite.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

// BUFSIZE: How much input is read at once from stdin. This really should be
// set some more sane way, like the size of the display.
#define BUFSIZE 1024
#define min(X, Y)  ((X) < (Y) ? (X) : (Y))

// This structure is passed into the listener to specify how it should run.
typedef struct {
    /* mode: Display mode.
    If STDIN, the listener displays the frames that are given via stdin as
    RGBRGBRGB... scanlines, every 3 characters giving a 24-bit RGB triplet.
    If CYCLE, the listener displays a cycling pattern mostly for testing.
    If SOLID, the listener displays a solid grey image of level
    'constant_value'. */
    enum { STDIN, CYCLE, SOLID, RED, GREEN, YELLOW, BLUE, PURPLE, CYAN } mode;
    /* refresh: Refresh cycle delay, in microseconds.
    This is the time, in microseconds, to delay between refreshing the display
    (i.e. pushing a frame out over SPI). If mode=STDIN, then this delay only
    applies for async mode. */
    int refresh;
    /* async: Asynchronous mode. (Only matters for mode=STDIN)
    If nonzero, then it should refresh asynchronously, i.e. it will delay for
    'refresh' microseconds and then refresh the display, whether a new frame
    is waiting on stdin or not. If this is zero, then the listener refreshes
    the display only when it has read input from stdin. */
    int async;
    /* verbose: Verbosity level, currently, 0, 1, or 2. 
    Still in the process of being refined. */
    int verbose;
    /* constant_value: The constant gray level (0-255) that will be sent to
    the display. This only matters if mode=SOLID. */
    int constant_value;
    int constant_value_red;
    int constant_value_green;
    int constant_value_blue;
    int constant_value_yellow;
    int constant_value_purple;
    int constant_value_cyan;
} listener_options_t;

void print_help(char * name);
int run_display(listener_options_t * opt);

int main(int argc, char *argv[]) {
    int opt;

    listener_options_t options;
    options.async = 0;
    options.verbose = 0;
    options.constant_value = -1;
    options.constant_value_red = -1;
    options.constant_value_green = -1;
    options.constant_value_yellow = -1;
    options.constant_value_blue = -1;
    options.constant_value_purple = -1;
    options.constant_value_cyan = -1;
    options.refresh = 10000;
    options.mode = STDIN;

    while((opt = getopt(argc, argv, "htc:R:G:Y:B:P:C:r:asvV")) != -1) {
        switch(opt) {
        case 'h':
            print_help(argv[0]);
            return 0;
            break;
        case 'a':
            options.async = 1;
            printf("Async mode on\n");
            break;
        case 's':
            options.async = 0;
            printf("Sync mode on\n");
            break;
        case 'v':
            options.verbose = 1;
            printf("Verbose output on\n");
            break;
        case 'V':
            options.verbose = 2;
            printf("Really verbose output on\n");
            break;
        case 't':
            if (options.mode != STDIN) {
                fprintf(stderr, "Error - Options -t and -c are exclusive\n\n");
                print_help(argv[0]);
                return -15;
            }
            options.mode = CYCLE;
            printf("Printing test pattern\n");
            break;
        case 'c':
            if (options.mode != STDIN) {
                fprintf(stderr, "Error - Options -t and -c are exclusive\n\n");
                print_help(argv[0]);
                return -14;
            }
            options.mode = SOLID;
            options.constant_value = atoi(optarg);
            if (options.constant_value < 0 || options.constant_value > 255) {
                fprintf(stderr, "Error - Level must be value from 0 to 255.\n\n");
                return -13;            
            }
            printf("Printing constant value %d\n", options.constant_value);
            break;
        case 'B':
            if (options.mode != STDIN) {
                fprintf(stderr, "Error - Options -t -c -R -G -Y -B -P -C are exclusive\n\n");
                print_help(argv[0]);
                return -55;
            }
            options.mode = BLUE;
            options.constant_value_blue = atoi(optarg);
            if (options.constant_value_blue < 0 || options.constant_value_blue > 255) {
                fprintf(stderr, "Error - Level must be value from 0 to 255.\n\n");
                return -54;
            }
            printf("Printing constant value of blue %d\n", options.constant_value_blue);
            break;
        case 'P':
            if (options.mode != STDIN) {
                fprintf(stderr, "Error - Options -t -c -R -G -Y -B -P -C are exclusive\n\n");
                print_help(argv[0]);
                return -45;
            }
            options.mode = PURPLE;
            options.constant_value_purple = atoi(optarg);
            if (options.constant_value_purple < 0 || options.constant_value_purple > 255) {
                fprintf(stderr, "Error - Level must be value from 0 to 255.\n\n");
                return -44;
            }
            printf("Printing constant value of purple %d\n", options.constant_value_purple);
            break;
        case 'C':
            if (options.mode != STDIN) {
                fprintf(stderr, "Error - Options -t -c -R -G -Y -B -P -C are exclusive\n\n");
                print_help(argv[0]);
                return -35;
            }
            options.mode = CYAN;
            options.constant_value_cyan = atoi(optarg);
            if (options.constant_value_cyan < 0 || options.constant_value_cyan > 255) {
                fprintf(stderr, "Error - Level must be value from 0 to 255.\n\n");
                return -34;
            }
            printf("Printing constant value of cyan %d\n", options.constant_value_cyan);
            break;
	case 'R':
            if (options.mode != STDIN) {
                fprintf(stderr, "Error - Options -t -c -R -G -Y -B -P -C are exclusive\n\n");
                print_help(argv[0]);
                return -25;
            }
            options.mode = RED;
            options.constant_value_red = atoi(optarg);
            if (options.constant_value_red < 0 || options.constant_value_red > 255) {
                fprintf(stderr, "Error - Level must be value from 0 to 255.\n\n");
                return -24;            
            }
            printf("Printing constant value of red %d\n", options.constant_value_red);
            break;	
	case 'G':
            if (options.mode != STDIN) {
                fprintf(stderr, "Error - Options -t -c -R -G -Y -B -P -C are exclusive\n\n");
		print_help(argv[0]);
                return -23;
            }
            options.mode = GREEN;
            options.constant_value_green = atoi(optarg);
            if (options.constant_value_green < 0 || options.constant_value_green > 255) {
                fprintf(stderr, "Error - Level must be value from 0 to 255.\n\n");
                return -21;
            }
            printf("Printing constant value of green %d\n", options.constant_value_green);
            break;
	case 'Y':
            if (options.mode != STDIN) {
                fprintf(stderr, "Error - Options -t -c -R -G -Y -B -P -C are exclusive\n\n");
                print_help(argv[0]);
                return -20;
            }
            options.mode = YELLOW;
            options.constant_value_yellow = atoi(optarg);
            if (options.constant_value_yellow < 0 || options.constant_value_yellow > 255) {
                fprintf(stderr, "Error - Level must be value from 0 to 255.\n\n");
                return -21;
            }
            printf("Printing constant value of yellow %d\n", options.constant_value_yellow);
            break;
        case 'r':
            options.refresh = atoi(optarg);
            if (options.refresh < 0) {
                fprintf(stderr, "Error - Refresh time must be positive value.\n\n");
                return -12;
            }
            break;
        case ':':
            fprintf(stderr, "Error - Option `%c' needs a value\n\n", optopt);
            print_help(argv[0]);
            return -10;
            break;
        case '?':
            fprintf(stderr, "Error - No such option: `%c'\n\n", optopt);
            print_help(argv[0]);
            return -11;
            break;
        }
    }

    return run_display(&options);
}

void print_help(char * name) {
    printf("Usage: %s [-t|-c <val>] [-r <time>] [-v]\n", name);
    printf("  -t: Print a cycling test pattern\n");
    printf("  -c: Print a constant white value, 0-255\n");
    printf("  -v: Verbose startup; echo character at each frame\n");
    printf("  -V: Really verbosely echo for each frame\n");
    printf("  -r: Set the refresh period to 'r' microseconds (default 10000)\n");
    printf("\n");
    printf("  -R: Set to constant color Red\n");
    printf("  -G: Set to constant color Green\n");
    printf("  -B: Set to constant color Blue\n");
    printf("  -P: Set to constant color Purple\n");
    printf("  -Y: Set to constant color Yellow\n");
    printf("  -C: Set to constant color Cyan\n");
    printf("\n");
    printf("The following only matter if listening on stdin (i.e. not -t not -c)\n");
    printf("  -a: Use async mode (push frames whether more are read via stdin or not )\n");
    printf("  -s: Use sync mode (no frames are pushed until read via stdin)\n");
    printf("      Note that sync mode ignore refresh period (it becomes tied to stdin)");
    printf("\n");
    printf("If neither -t nor -c given, then this listens on stdin for frames.\n");
    printf("Note that the delay in -r is only a minimum delay.\n");
}

int run_display(listener_options_t * opt) {
    unsigned char buf[BUFSIZE + 1];

    // 'x' and 'y' are the size of the screen in pixels. We set them later.
    int x = 0;
    int y = 0;
    // 'img' is the image buffer, packed as RGBRGBRGB... across a scanline.
    unsigned char * img = NULL;
    // 'frame' is the current frame number
    unsigned long int frame = 0;

    // Statistics on underrun/overrun
    unsigned int frames_display = 0;
    unsigned int frames_under = 0;
    unsigned int frames_over = 0;

    // 'start' is the time when the display routine began (i.e. frame=0)
    // 'current' us the time for the current frame 
    struct timeval start;
    struct timeval current;

    if (rpi_gpio_init()) {
        fprintf(stderr, "Error - Failed to initialize GPIO!\n");
        return 1;
    }

    img = shiftbrite_get_image(&x, &y);
    if (img == NULL) {
        fprintf(stderr, "Error - Image buffer is NULL!\n");
        return 2;
    }
    if (opt->verbose) {
        printf("Got %dx%d image buffer...\n", x, y);
    }

    if (opt->mode == STDIN && opt->async) {
        // If we need async mode, then set up stdin for non-blocking reads.
        int flags = fcntl(0, F_GETFL);
        fcntl(0, F_SETFL, flags | O_NONBLOCK);
    }
    gettimeofday(&start, NULL);
    // Until killed by the user, repeatedly send an image to the display. 
	while (1)
	{        
        // Fill the image.
        if (opt->mode == CYCLE) {
	    int i;
            for(i = 0; i < x * y; ++i) {
                img[3*i + 0] = (2*frame + i) % 255; // R
                img[3*i + 1] = (frame + 2*i) % 253; // G
                img[3*i + 2] = (3*frame + 3*i) % 254; // B
            
	}
	} else if (opt->mode == SOLID && opt->constant_value >= 0) {
                img[0] = opt->constant_value;
		shiftbrite_refresh();
                return 0;
	} else if (opt->mode == RED && opt->constant_value_red >= 0) {
	    	img[0] = opt->constant_value_red;
	        shiftbrite_refresh();
		return 0;
        } else if (opt->mode == GREEN && opt->constant_value_green >= 0) {
                img[1] = opt->constant_value_green;
		shiftbrite_refresh();
                return 0;
        } else if (opt->mode == BLUE && opt->constant_value_blue >= 0) {
                img[2] = opt->constant_value_blue;
                shiftbrite_refresh();
                return 0;
        } else if (opt->mode == YELLOW && opt->constant_value_yellow >= 0) {
                img[1] = opt->constant_value_yellow;
		img[0] = opt->constant_value_yellow;
		shiftbrite_refresh();
                return 0;
         } else if (opt->mode == PURPLE && opt->constant_value_purple >= 0) {
                img[2] = opt->constant_value_purple;
                img[0] = opt->constant_value_purple;
                shiftbrite_refresh();
                return 0;
	 } else if (opt->mode == CYAN && opt->constant_value_cyan >= 0) {
                img[2] = opt->constant_value_cyan;
                img[1] = opt->constant_value_cyan;
                shiftbrite_refresh();
                return 0;
    
// DMH LOTS of fucking hacking and trial and error but I got it figured out.
	} else {
            int r = read(0, buf, BUFSIZE);
            if (r != EAGAIN && r != EWOULDBLOCK && r != -1) {
                if (opt->verbose >= 2) {
                    printf("Read %d bytes from stdin\n", r);
                }
                // under = discrepancy between how many bytes we want, and
                // how many we read
                int under = x*y*3 - r;
                if (under > 0) {
                    if (opt->verbose >= 2) {
                        printf("Ignoring, need %d more\n", under);
                    }
                    ++frames_under;
                } else {
                    memcpy(img, buf, x*y*3);
                    ++frames_display;
                    if (under < 0) {
                        if (opt->verbose >= 2) {
                            printf("Ignoring %d extra bytes at end\n", -under);
                        }
                        ++frames_over;
                    }
                }
            }
        }

        if (opt->verbose >= 2) {
            printf("Frame %ld\n", frame);
            if (frame > 0 && frame % 1000 == 0) {
                gettimeofday(&current, NULL);
                double rate = (double) frame / difftime(current.tv_sec, start.tv_sec); 
                //printf("dt=%f\n", difftime(current.tv_sec, start.tv_sec));
                //printf("Averaging %f frames/second.\n");
            }
        } else if (frame % 200 == 0 && frame > 0 && opt->verbose && opt->mode == STDIN) {
            printf("Frames refr/recv/short/miss: %u/%u/%u/%u\n", frame, frames_display, frames_under, frames_over);
        }

        //printHexDump(img, x*y*3);

        // Finally, actually push the image out to the ShiftBrite.
        shiftbrite_refresh();
       
        // If we're in sync mode, don't delay. 
        if (opt->mode == STDIN && opt->async) {
            usleep(opt->refresh);
        }
        ++frame;
    
    }
    return 0;
}
