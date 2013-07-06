#ifndef __TH12_H__
#define __TH12_H__

/* system command mode */
/* two modes of commands */
/* Initialization is a command sequence that initializes the power board */
/* Run is the normal command sequence that issues speed commands and monitors the power board */

/* TODO need a MODE_STOP and MODE_STARTUP */
/* depends on monitoring actual speed though */
enum {
	MODE_INITIALIZATION,
	MODE_RUN,
};

struct th12_state {
	uint8_t red; 
	uint8_t green;
	uint8_t blue;
};
typedef struct th12_state th12_t;

extern th12_t sys;

#endif /* __SHELL_COMPRESSOR_H__ */
