/* jackmidi.h
 * function prototypes for interface to midi in
 *
 * for Denemo, a gtk+ frontend to GNU Lilypond
 * (c)2008 Jeremiah Benham, Richard Shann
 */
#ifndef JACKMIDI_H
#define JACKMIDI_H
int init_jack(void);
int jackmidi(void);
void  jackstop(void);
void jack_midi_playback_start(void);
void jack_midi_playback_stop(void);
void jack_playpitch(gint key, gint duration);
void jack_output_midi_event(unsigned char *buffer);
int jack_kill_timer(void);
int create_jack_midi_client(void);
int remove_jack_midi_client(void);
int create_jack_midi_port(int client_number);
int remove_jack_midi_port(int client_number);
int rename_jack_midi_port(int client_number, int port_number, char *port_name);
void remove_all_jack_midi_ports(gint client_number); 
void jack_start_restart (void);
int maxnumber_of_clients(void);
int maxnumber_of_ports(void);

#endif //JACKMIDI_H
