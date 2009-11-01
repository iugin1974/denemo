#ifndef FLUID_H
#define FLUID_H
int fluidsynth_init();
void fluidsynth_shutdown();
void fluidsynth_start_restart();
gchar * fluidsynth_get_default_audio_driver();
void fluid_playpitch(int key, int duration);
void fluid_output_midi_event(unsigned char *buffer);
void choose_sound_font (GtkWidget * widget, GtkWidget *fluidsynth_soundfont);
void fluid_midi_play(void);
void fluid_midi_stop(void);
#endif // FLUID_H
