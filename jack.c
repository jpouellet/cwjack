#include <err.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <jack/jack.h>

#include "extern.h"

jack_client_t *client = NULL;
jack_nframes_t sample_rate;
jack_port_t *port;

/*
 * Oscillate at a frequency close to the goal frequency such that its period
 * starts and stops evenly on a discrete frame of the PCM buffer to avoid
 * noise when starting and stoping the oscilator.
 */
double cw_res_freq;

int
process_callback(jack_nframes_t nframes, void *arg)
{
	/*
	 * If we start and stop mid-wave when the key is pressed and
	 * released, we get a bunch of noise because the waveform gets cut
	 * off mid-cycle, so wait until it has oscillated a full period
	 * before stopping to play it.
	 */
	static bool is_playing_wave;
	static long offset;

	jack_default_audio_sample_t *buf;
	jack_nframes_t i;

	buf = jack_port_get_buffer(port, nframes);

	for(i = 0; i < nframes; i++) {
		offset++;
		buf[i] = is_playing_wave ? sin(offset * 2 * cw_res_freq * (double)M_PI / (double)sample_rate) : 0;
		/*
		 * There is still some nose upon start and stop.
		 * I do not know why.
		 */
		if (offset * cw_res_freq / (double)sample_rate >= 1) {
			offset = 0;
			is_playing_wave = cw_keydown;
		}
	}

	return 0;
}

int
change_sample_rate_callback(jack_nframes_t nframes, void *arg)
{
	printf("new sample rate: %u\n", nframes);
	sample_rate = nframes;

	/* recalculate the sample-rate resonance frequency */
	cw_res_freq = (double)sample_rate / round((double)sample_rate / cw_freq);
	printf("using sample rate resonating frequency of %.3f Hz\n", cw_res_freq);

	return 0;
}

void
shutdown_callback(void *arg)
{
	warnx("JACK server shutting down");
	cw_stop();
}

int
cw_jack_init(void)
{
	client = jack_client_open(CLIENT_NAME, JackNullOption, NULL);
	if (client == NULL) {
		warnx("unable to connect to JACK server");
		return -1;
	}

	jack_set_process_callback(client, process_callback, NULL);
	jack_set_sample_rate_callback(client, change_sample_rate_callback, NULL);
	jack_on_shutdown(client, shutdown_callback, NULL);

	sample_rate = jack_get_sample_rate(client);

	port = jack_port_register(client, "output", JACK_DEFAULT_AUDIO_TYPE,
	    JackPortIsOutput, 0);

	if (jack_activate(client)) {
		warnx("unable to activate client");
		return -1;
	}

	return 0;
}

int
cw_jack_connect(void)
{
	int i;
	const char **ports;

	if (client == NULL) {
		warnx("tried to connect to output ports without a JACK connection");
		return -1;
	}

	ports = jack_get_ports(client, NULL, NULL,
	    JackPortIsPhysical | JackPortIsInput);
	if (ports == NULL) {
		warnx("unable to get playback ports");
		return -1;
	}

	for (i = 0; ports[i] != NULL; i++)
		if (jack_connect(client, jack_port_name(port), ports[i])) {
			warnx("unable to connect to JACK output port");
			jack_free(ports);
			return -1;
		}

	jack_free(ports);

	/* connect to fldigi if we find it */
	if (jack_connect(client, jack_port_name(port), "fldigi:in1") == 0)
		printf("connected to fldigi\n");

	return 0;
}

int
cw_jack_cleanup(void)
{
	if (client == NULL) {
		warnx("tried to cleanup the JACK client "
		    "when it was not initialized");
		return -1;
	}

	jack_client_close(client);
	client = NULL;

	return 0;
}
