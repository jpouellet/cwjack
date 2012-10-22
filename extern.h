#include <stdbool.h>

#define DEFAULT_FREQUENCY 1000.0
#define CLIENT_NAME "Virtual CW Keyer"

extern volatile bool cw_keydown;
extern double cw_freq;

/* jack.c */
int cw_jack_init(void);
int cw_jack_connect(void);
int cw_jack_cleanup(void);

/* key.c */
int key_configure(void);
int key_init(void);
int key_listen(void);
int key_stop(void);
int key_cleanup(void);

/* main.c */
void cw_stop(void);
