#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <complex>
#include <cmath>
#include <iomanip>
#include "AudioFile.h"

using namespace std;

struct note 
{
    string name;
    double range_start;
    double range_end;
    double note_freq;
};

struct chord
{
    string name;
    int notes[3];
};

struct note_location
{
    int note_index;
    int octave;

};

void setup(double* input_buffer, double output_buffer[8192*2][2], int buffer_size, double* phases_in, double* phases_out_h2, double* phases_out_h3, int fft_size);

int incrementCircularBuffer(int index, int increment, int buffer_size);

void fft( complex<double>* f, complex<double>* ftilde, int log2N );

void ifft( complex<double>* ftilde, complex<double>* f, int log2N );

double wrap(int phase);

note_location processing(complex<double>* section_fft, complex<double>* section_fft_h2, complex<double>* section_fft_h3, double* last_phases_in, double* last_phases_out_h2, double* last_phases_out_h3, note_location prev_note);

void printArray(double* array, int length);

void printArray(complex<double>* array, int length);

note_location isInChord(double freq, int c);
note_location nextNoteInChord(note_location current, int chord_num);

note range_of_notes[12][9];

chord range_of_chords[24];

int buffer_size = 8192;
int hop_size = 512;
int fft_size = 2048;
int Fs = 44100;
int chord_num=1;
bool start = false;
int beats_per_measure =4;
int beats_per_min = 120;
int samples_per_measure = Fs*60*beats_per_measure/beats_per_min;
//int samples_per_measure = 10;

int num_of_chords=8;
int song_chords[8];
int curr_chord=0;

int main()
{
    AudioFile<double> audioFile;
    AudioFile<double> newaudioFile;
    audioFile.load("twinkle_twinkle.wav");
    int length = audioFile.getNumSamplesPerChannel();

    newaudioFile.setNumChannels(2);
    newaudioFile.setNumSamplesPerChannel(length);


    double audio[length];
    vector< vector<double> > audioOut;
    Fs = audioFile.getSampleRate();
    double input_buffer[buffer_size];
    double output_buffer[buffer_size][2];
    int in_write_index=0;
    int out_read_index=0;
    int out_write_index= 2*hop_size;
    int hop_count =0;
    double ratio = (double)hop_size/(double)fft_size;
    int measure_index = hop_size - fft_size;

    double analysis_magnitudes[fft_size/2 + 1];
    double analysis_frequencies[fft_size/2 + 1];
    double last_phases_in[fft_size];
    double last_phases_out_h2[fft_size];
    double last_phases_out_h3[fft_size];


    song_chords[0] = 0;
    song_chords[1] = 0;
    song_chords[2] = 10;
    song_chords[3] = 0;
    song_chords[4] = 10;
    song_chords[5] = 0;
    song_chords[6] = 14;
    song_chords[7] = 0;

    for(int i=0; i<length;i++)
    {
        audio[i] = audioFile.samples[0][i];
    }

    setup(input_buffer, output_buffer, buffer_size, last_phases_in, last_phases_out_h2, last_phases_out_h3, fft_size);

    note_location prev_note;
    prev_note.note_index=0;
    prev_note.octave=4;
    for(int i=0;i<length;i++) //loop through each sample in wav file
    {
        input_buffer[in_write_index] = audio[i]; //store input into input buffer
        in_write_index = incrementCircularBuffer(in_write_index, 1, buffer_size);
        double outSample = output_buffer[out_read_index][0]/output_buffer[out_read_index][1]; //read sample from output buffer
        output_buffer[out_read_index][0]=0;
        output_buffer[out_read_index][1]=0;
        out_read_index = incrementCircularBuffer(out_read_index, 1, buffer_size);
        
        if(++hop_count >= hop_size) //ready for processing
        {
            hop_count = 0;
            //read section of input buffer and store
            complex<double> section[fft_size];
            int in_read_index = in_write_index-fft_size;
            if(in_read_index < 0)
            {
                in_read_index = buffer_size + in_read_index;
            }
            for(int j=0; j<fft_size; j++)
            {
                section[j] = complex<double>(input_buffer[in_read_index],0);
                in_read_index = incrementCircularBuffer(in_read_index,1, buffer_size);
                
            }
            complex<double> section_fft[fft_size];
            complex<double> section_fft_h2[fft_size];
            complex<double> section_fft_h3[fft_size];
            fft(section, section_fft, (int)log2(fft_size));
            prev_note = processing(section_fft, section_fft_h2,section_fft_h3, last_phases_in, last_phases_out_h2, last_phases_out_h3, prev_note);
            complex<double> new_section[fft_size];
            complex<double> new_section_h2[fft_size];
            complex<double> new_section_h3[fft_size];
            ifft(section_fft, new_section, (int)log2(fft_size));
            ifft(section_fft_h2, new_section_h2, (int)log2(fft_size));
            ifft(section_fft_h3, new_section_h3, (int)log2(fft_size));

            int outIndex = out_write_index;
            //copy output harmonies generated into output buffer
            for(int j=0; j<fft_size; j++)
            {
                if(measure_index+j < samples_per_measure && measure_index+j >= 0)
                {
                     output_buffer[outIndex][0]+=new_section[j].real() + new_section_h2[j].real();
                     output_buffer[outIndex][1]++;
                 }
                outIndex = incrementCircularBuffer(outIndex,1, buffer_size);
                
            }
            out_write_index = incrementCircularBuffer(out_write_index, hop_size, buffer_size);
            //keep track of current chord and measure index
            if(start)
            {
                measure_index+=hop_size;
                if(measure_index + fft_size >= samples_per_measure)
                {
                    measure_index = -hop_size;
                    curr_chord = (curr_chord+1)%num_of_chords;
                }

            }
            
        }
        //copy to new wav file
        newaudioFile.samples[0][i] = outSample;
        newaudioFile.samples[1][i] = outSample;
        
        

    }

    
    newaudioFile.save("twinkle_twinkle_2.wav", AudioFileFormat::Wave);
    return 0;
}

//setup buffers, hamming window, etc
void setup(double* input_buffer, double output_buffer[8192*2][2], int buffer_size, double* phases_in, double* phases_out_h2, double* phases_out_h3, int fft_size)
{
    for(int i=0;i<buffer_size; i++)
    {
        input_buffer[i]=0;
        output_buffer[i][0]=0;
        output_buffer[i][1]=0;
    }
    
    for(int i=0; i<fft_size; i++)
    {
        phases_in[i] = 0;
        phases_out_h2[i] = 0;
        phases_out_h3[i] = 0;
    }

    int r=-57;
    for(int oct = 0; oct < 9; oct++)
    {
        for(int n = 0; n < 12; n++)
        {
            if(n==0) range_of_notes[n][oct].name = "C";
            if(n==1) range_of_notes[n][oct].name = "C#/Db";
            if(n==2) range_of_notes[n][oct].name = "D";
            if(n==3) range_of_notes[n][oct].name = "D#/Eb";
            if(n==4) range_of_notes[n][oct].name = "E";
            if(n==5) range_of_notes[n][oct].name = "F";
            if(n==6) range_of_notes[n][oct].name = "F#/Gb";
            if(n==7) range_of_notes[n][oct].name = "G";
            if(n==8) range_of_notes[n][oct].name = "G#/Ab";
            if(n==9) range_of_notes[n][oct].name = "A";
            if(n==10) range_of_notes[n][oct].name = "A#/Bb";
            if(n==11) range_of_notes[n][oct].name = "B";

            range_of_notes[n][oct].note_freq = 440.0*pow(2.0,(double)r/12.0);
            r++; 
            //cout << range_of_notes[n][oct].name << "   " << r << endl;
        }
    }


    for(int i=0; i< 9; i++)
    {
        for(int j=0; j<12; j++)
        {
            if(i==0 && j==0)
            {
                j++;
            }
            note curr_note =  range_of_notes[j][i];
            int prev_i = i;
            int prev_j = j-1;
            if(prev_j < 0)
            {
                prev_i--;
                prev_j=11;
            }
            note prev_note = range_of_notes[prev_j][prev_i];

            double midpoint = (curr_note.note_freq-prev_note.note_freq)/2;
            double interval = curr_note.note_freq - midpoint;
            range_of_notes[prev_j][prev_i].range_end = interval;
            range_of_notes[j][i].range_start = interval;
            if(j==11 && i==8)
            {
                double end_interval = curr_note.note_freq + midpoint;
                range_of_notes[j][i].range_end = end_interval;
            }
        }
    }
    int ch=0;
    int n=0;
    while(ch < 24)
    {
        if(ch%2==1)
        {
            
        }
        chord curr;
        string chord_name = range_of_notes[n][0].name;
        n=n%12;
        int top_n = (n+7)%12;
        range_of_chords[ch].notes[0]=n;
        range_of_chords[ch].notes[2]=top_n;
        if(ch%2==0)
        {
            chord_name+= "maj";
            int mid_note = (n+4)%12;
            range_of_chords[ch].notes[1]=mid_note;

        }
        else
        {
            chord_name+= "min";
            int mid_note = (n+3)%12;
            range_of_chords[ch].notes[1]=mid_note;
            n++;
        }
        range_of_chords[ch].name = chord_name;
        ch++;
    }

}

int incrementCircularBuffer(int index, int increment, int buffer_size)
{
    index+=increment;
    index = index%buffer_size;
    return index;
}

void fft( complex<double>* f, complex<double>* ftilde, int log2N )                 // Fast Fourier Transform
{
   int N = 1 << log2N;

   // Reorder
   for ( int i = 0; i < N; i++ )
   {
      int ii = 0, x = i;
      for (int j = 0; j < log2N; j++)
      {
         ii <<= 1;
         ii |= ( x & 1 );
         x >>= 1;
      }
      ftilde[ii] = f[i];
   }

   for ( int s = 1; s <= log2N; s++ )
   {
      int m = 1 << s;
      int m2 = m / 2;
      complex<double> w = 1.0;
      complex<double> wm = polar( 1.0, -(M_PI) / m2 );
      for ( int j = 0; j < m2; j++ )
      {
         for ( int k = j; k < N; k += m )
         {
            complex<double> t = w * ftilde[k+m2];
            complex<double> u =     ftilde[k   ];
            ftilde[k   ] = u + t;
            ftilde[k+m2] = u - t;
         }
         w *= wm;
      }
   }
}

void ifft( complex<double>* ftilde, complex<double>* f, int log2N )                // Inverse Fast Fourier Transform
{
   int N = 1 << log2N;

   for ( int m = 0; m < N; m++ ) ftilde[m] = conj( ftilde[m] );      // Apply conjugate (reversed below)

   fft( ftilde, f, log2N );

   double factor = 1.0 / N;
   for ( int m = 0; m < N; m++ ) f[m] = conj( f[m] ) * factor;

   for ( int m = 0; m < N; m++ ) ftilde[m] = conj( ftilde[m] );      // Only necessary to reinstate ftilde
}


note_location processing(complex<double>* section_fft, complex<double>* section_fft_h2, complex<double>* section_fft_h3, double* last_phases_in, double* last_phases_out_h2, double* last_phases_out_h3, note_location prev_note)
{
    double magnitudes[fft_size/2 + 1];
    double frequencies[fft_size/2 + 1];
    //store magnitudes, frequencies
    //find fundamental freq
    double max_amplitude = sqrt(pow(section_fft[0].real(),2.0)+pow(section_fft[0].imag(),2.0));
    int max_index = 0;
    for(int i=0; i<fft_size/2; i++)
    {
        double amplitude = sqrt(pow(section_fft[i].real(),2.0)+pow(section_fft[i].imag(),2.0));
        double phase = atan2(section_fft[i].imag(), section_fft[i].real());

        double phase_diff = phase - last_phases_in[i];
        double bin_centre_freq = 2*M_PI*i/fft_size;
        phase_diff = wrap(phase_diff - bin_centre_freq*hop_size);
        double bin_dev = phase_diff*(double)fft_size/(double)hop_size/(2.0*M_PI);
        frequencies[i] = (double)i+bin_dev;
        magnitudes[i] = amplitude;
        last_phases_in[i]=phase;
        if(max_amplitude < amplitude)
        {
            max_amplitude = amplitude;
            max_index = i;
        }
    }

    if(max_amplitude >= 1.0)
    {
        start=true;
    }
   
    double fundamental_freq = frequencies[max_index]*(double)Fs/(double)fft_size;
    note_location current_note = isInChord(fundamental_freq, song_chords[curr_chord]);
    cout << fundamental_freq << " " << current_note.note_index << " " << current_note.octave << endl;
    //figure out where to pitch to shift to w algorithm
    double pitch_shift_h1 = pow(2.0,-1.0); //pitch to one octave lower
    double pitch_shift_h2 = 0;
    double pitch_shift_h3 = 0;
    if(current_note.note_index < 0) //if note is not in Chord
    {
        int current_note_freq = range_of_notes[prev_note.note_index][prev_note.octave].note_freq;
        int semitones_from_current = floor(log2(fundamental_freq/current_note_freq)*12+0.5);
        note_location next_note = nextNoteInChord(prev_note, song_chords[curr_chord]);
        int semitones_between_chord = 0;
        if(prev_note.note_index > next_note.note_index)
        {
            semitones_between_chord = (11-prev_note.note_index)+(next_note.note_index+1);
        }
        else
        {
            semitones_between_chord = next_note.note_index - prev_note.note_index;
        }

        note_location next_note_h3 = nextNoteInChord(next_note, song_chords[curr_chord]);
        int semitones_between_chord_h3 = 0;
        if(next_note.note_index > next_note_h3.note_index)
        {
            semitones_between_chord_h3 = (11-next_note.note_index)+(next_note_h3.note_index+1);
        }
        else
        {
            semitones_between_chord_h3 = next_note_h3.note_index - next_note.note_index;
        }
        pitch_shift_h2 = 1.0;
        pitch_shift_h3 = 1.0;
    }
    else //is in chord
    {
        note_location next_note = nextNoteInChord(current_note, song_chords[curr_chord]);
        int semitones_between_chord = 0;
        if(current_note.note_index > next_note.note_index)
        {
            semitones_between_chord = (11-current_note.note_index)+(next_note.note_index+1);
        }
        else
        {
            semitones_between_chord = next_note.note_index - current_note.note_index;
        }

        note_location next_note_h3 = nextNoteInChord(next_note, song_chords[curr_chord]);
        int semitones_between_chord_h3 = 0;
        if(next_note.note_index > next_note_h3.note_index)
        {
            semitones_between_chord_h3 = (11-next_note.note_index)+(next_note_h3.note_index+1);
        }
        else
        {
            semitones_between_chord_h3 = next_note_h3.note_index - next_note.note_index;
        }
        pitch_shift_h2 = pow(2.0, (double)(semitones_between_chord)/12.0);
        pitch_shift_h3 = pow(2.0, (double)(semitones_between_chord+semitones_between_chord_h3)/12.0);
        prev_note = current_note;
    }
    
    //multiply freq by pitch shift rato and reset section to match new frequencies and magnitudes
    double new_magnitudes_h2[fft_size];
    double new_frequencies_h2[fft_size];
    double new_magnitudes_h3[fft_size];
    double new_frequencies_h3[fft_size];
    for(int i=0; i< fft_size; i++)
    {
        new_magnitudes_h2[i] = 0;
        new_frequencies_h2[i] = 0;
        new_magnitudes_h3[i] = 0;
        new_frequencies_h3[i] = 0;

    }
    for(int i=0; i<fft_size/2; i++)
    {
        double sound_ratio_h2;
        double sound_ratio_h3;
        if(start==false)
        {
            sound_ratio_h2=0.0;
            sound_ratio_h3=0.0;
        }
        else
        {
            sound_ratio_h2=2.0;
            sound_ratio_h3=0.7;
        }
        int new_index_h2 = floor(i*pitch_shift_h2+ 0.5);

        if(new_index_h2 <= fft_size/2)
        {
            new_magnitudes_h2[new_index_h2] = magnitudes[i]*sound_ratio_h2;
            new_frequencies_h2[new_index_h2] = frequencies[i]*pitch_shift_h2;
        }

        int new_index_h3 = floor(i*pitch_shift_h3+ 0.5);

        if(new_index_h3 <= fft_size/2)
        {
            new_magnitudes_h3[new_index_h3] = magnitudes[i]*sound_ratio_h3;
            new_frequencies_h3[new_index_h3] = frequencies[i]*pitch_shift_h3;
        }
    }
    //convert magnitude and frequency back into complex number
    for(int i=0; i<fft_size/2; i++)
    {
        double bin_centre_freq = (2*M_PI)*(double)i/(double)fft_size;

        double amplitude_h2 = new_magnitudes_h2[i];
        double bin_dev_h2 = new_frequencies_h2[i] - (double)i;
        
        double phase_diff_h2 = bin_dev_h2*(2*M_PI)*(double)hop_size/(double)fft_size;
        phase_diff_h2 += bin_centre_freq*(double)hop_size;
        double phase_out_h2 = wrap(last_phases_out_h2[i] + phase_diff_h2);

        section_fft_h2[i] = complex<double>(amplitude_h2*cos(phase_out_h2),amplitude_h2*sin(phase_out_h2));

        if(i>0 && i < fft_size/2)
        {
            section_fft_h2[fft_size-i] = complex<double>(amplitude_h2*cos(phase_out_h2),-(amplitude_h2*sin(phase_out_h2)));
        }

        last_phases_out_h2[i] = phase_out_h2;

        double amplitude_h3 = new_magnitudes_h3[i];
        double bin_dev_h3 = new_frequencies_h3[i] - (double)i;
        
        double phase_diff_h3 = bin_dev_h3*(2*M_PI)*(double)hop_size/(double)fft_size;
        phase_diff_h3 += bin_centre_freq*(double)hop_size;
        double phase_out_h3 = wrap(last_phases_out_h3[i] + phase_diff_h3);

        section_fft_h3[i] = complex<double>(amplitude_h3*cos(phase_out_h3),amplitude_h2*sin(phase_out_h3));

        if(i>0 && i < fft_size/2)
        {
            section_fft_h3[fft_size-i] = complex<double>(amplitude_h3*cos(phase_out_h3),-(amplitude_h3*sin(phase_out_h3)));
        }

        last_phases_out_h3[i] = phase_out_h3;
    }
    return prev_note;
}

double wrap(int phase)
{
    if (phase >= 0)
        return fmod(phase + M_PI,2.0 * M_PI) - M_PI;
    else
        return fmod(phase - M_PI, -2.0 * M_PI) + M_PI;
}

void printArray(double* array, int length)
{
    for(int i=0; i<length; i++)
    {
        cout << array[i] << " ";
    }
    cout << " " << endl;
}

void printArray(complex<double>* array, int length)
{
    for(int i=0; i<length; i++)
    {
        cout << array[i] << " ";
    }
    cout << " " << endl;
}

//checks if note is in chord
note_location isInChord(double freq, int c)
{
    chord current = range_of_chords[c];
    for(int i=0; i< 3 ;i++)
    {
        for(int j=0; j< 9; j++)
        {
            int note_index = current.notes[i];
            if(freq >= range_of_notes[note_index][j].range_start && freq < range_of_notes[note_index][j].range_end)
            {
                note_location found;
                found.note_index = note_index;
                found.octave = j;
                return found;
            }
        }
    }
    note_location not_there;
    not_there.note_index=-1;
    not_there.octave=-1;

    return not_there;
}

//finds next note in chord
note_location nextNoteInChord(note_location current, int chord_num) //current note tells us where in range_of_notes the note sits, chord_num tells us where in range_of_chords it sits (0-23)
{
    int oct = current.octave;
    int next = 0;
    chord current_chord = range_of_chords[chord_num];
    for(int i=0; i< 3; i++)
    {
        if(current.note_index == current_chord.notes[i])
        {
            next = (i+1)%3;
        }
    }
    if(current.note_index > current_chord.notes[next])
    {
        if(oct < 8)
        {
            oct++;
        }
    }
    note_location next_note;
    next_note.note_index = current_chord.notes[next];
    next_note.octave = oct;
    return next_note;
}
