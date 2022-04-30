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

struct harmony
{
    double freq;
    double length;

};

void setup(double* input_buffer, double output_buffer[8192][2], double output_buffer_h1[8192][2], double output_buffer_h2[8192][2], double output_buffer_h3[8192][2], double* phases_in, double* phases_out_h1, double* phases_out_h2, double* phases_out_h3);

int incrementCircularBuffer(int index, int increment, int buffer_size);

void fft( complex<double>* f, complex<double>* ftilde, int log2N );

void ifft( complex<double>* ftilde, complex<double>* f, int log2N );

double wrap(int phase);

double processing(complex<double>* section_fft, complex<double>* section_fft_h1, complex<double>* section_fft_h2, complex<double>* section_fft_h3, double* last_phases_in, double* last_phases_out_h1, double* last_phases_out_h2, double* last_phases_out_h3);

void printArray(double* array, int length);

void printArray(complex<double>* array, int length);


int buffer_size = 8192;
int hop_size = 512;
int fft_size = 2048;
int Fs = 44100;
bool start = false;
int beats_per_measure =4;
int beats_per_min = 120;


vector<harmony> harmony_1;
vector<harmony> harmony_2;
vector<harmony> harmony_3;

int h1_index=0;
int h2_index=0;
int h3_index=0;
int harmonies = 1;

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
    double output_buffer_h1[buffer_size][2];
    double output_buffer_h2[buffer_size][2];
    double output_buffer_h3[buffer_size][2];
    int in_write_index=0;
    int out_read_index=0;
    int out_write_index= 2*hop_size;
    int hop_count =0;
    double ratio = (double)hop_size/(double)fft_size;
    int note_index_h1 = hop_size - fft_size;
    int note_index_h2 = hop_size - fft_size;
    int note_index_h3 = hop_size - fft_size;

    double analysis_magnitudes[fft_size/2 + 1];
    double analysis_frequencies[fft_size/2 + 1];
    double last_phases_in[fft_size];
    double last_phases_out_h1[fft_size];
    double last_phases_out_h2[fft_size];
    double last_phases_out_h3[fft_size];
    

    for(int i=0; i<length;i++)
    {
        audio[i] = audioFile.samples[0][i];

    }

    setup(input_buffer, output_buffer, output_buffer_h1, output_buffer_h2, output_buffer_h3, last_phases_in, last_phases_out_h1, last_phases_out_h2, last_phases_out_h3);
    

    double previous_note=0;
    for(int i=0;i<length;i++) // loop through all samples in wav file
    {
        input_buffer[in_write_index] = audio[i]; // copy input sample into input buffer
        in_write_index = incrementCircularBuffer(in_write_index, 1, buffer_size);
        double outSample;
        if(output_buffer_h1[out_read_index][1] != 0) //if not processing inital zero sample
        {
            outSample = output_buffer[out_read_index][0]/output_buffer[out_read_index][1];
        }
        //copy harmony sample onto output sample
        if(harmonies >= 1)
        {
            if(output_buffer_h1[out_read_index][1] != 0)
            {
                outSample += output_buffer_h1[out_read_index][0]/output_buffer_h1[out_read_index][1];
                
            }
            output_buffer_h1[out_read_index][0]=0;
            output_buffer_h1[out_read_index][1]=0;
            
        }
        if(harmonies >=2)
        {
            cout<< "shouldnt be here" << endl;
            outSample += output_buffer_h2[out_read_index][0]/output_buffer_h2[out_read_index][1];
            output_buffer_h2[out_read_index][0]=0;
            output_buffer_h2[out_read_index][1]=0;
        }
        if(harmonies >= 3)
        {
            cout << "shouldnt be here" << endl; 
            outSample += output_buffer_h3[out_read_index][0]/output_buffer_h3[out_read_index][1];
            output_buffer_h3[out_read_index][0]=0;
            output_buffer_h3[out_read_index][1]=0;
        }
        output_buffer[out_read_index][0]=0;
        output_buffer[out_read_index][1]=0;
        out_read_index = incrementCircularBuffer(out_read_index, 1, buffer_size);
        
        //ready for processing
        if(++hop_count >= hop_size)
        {
            hop_count = 0;
            //copy section from input buffer
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
            complex<double> section_fft_h1[fft_size];
            complex<double> section_fft_h2[fft_size];
            complex<double> section_fft_h3[fft_size];
            fft(section, section_fft, (int)log2(fft_size));
            
            double current_note = processing(section_fft, section_fft_h1, section_fft_h2, section_fft_h3, last_phases_in, last_phases_out_h1, last_phases_out_h2, last_phases_out_h3);
            complex<double> new_section[fft_size];
            complex<double> new_section_h1[fft_size];
            complex<double> new_section_h2[fft_size];
            complex<double> new_section_h3[fft_size];
            ifft(section_fft, new_section, (int)log2(fft_size));
            ifft(section_fft_h1, new_section_h1, (int)log2(fft_size));
            ifft(section_fft_h2, new_section_h2, (int)log2(fft_size));
            ifft(section_fft_h3, new_section_h3, (int)log2(fft_size));

            int outIndex = out_write_index;

            int samples_in_note_h1 =0;
            int samples_in_note_h2 =0;
            int samples_in_note_h3 =0;
            
            //check how many samples in current note of each harmony
            if(harmonies >= 1)
            {
                samples_in_note_h1 = harmony_1.at(h1_index).length*Fs*60/beats_per_min;
            }
            if(harmonies >= 2)
            {
                samples_in_note_h2 = harmony_2.at(h2_index).length*Fs*60/beats_per_min;
            }
            if(harmonies >= 3)
            {
                samples_in_note_h3 = harmony_3.at(h3_index).length*Fs*60/beats_per_min;
            }
            //loop through sections to create output buffers of harmonies
            for(int j=0; j<fft_size; j++)
            {
                if(note_index_h1+j < samples_in_note_h1 && note_index_h1+j >= 0 && harmonies >= 1)
                {
                    if(floor(log2(previous_note/current_note)*12+0.5) != 0 && note_index_h1+fft_size-1 <= samples_in_note_h1) //if theres an unexpected note change (ie not at the interval given by the user)
                    {
                        if(j>=hop_size)
                        {
                            output_buffer_h1[outIndex][0]=new_section_h1[j].real();
                            output_buffer_h1[outIndex][1]=1;

                        }
                    }
                    else
                    {
                        output_buffer_h1[outIndex][0]+=new_section_h1[j].real();
                        output_buffer_h1[outIndex][1]++;
                    }
                     
                }
                if(note_index_h2+j < samples_in_note_h2 && note_index_h2+j >= 0 && harmonies >= 2)
                {
                     if(floor(log2(previous_note/current_note)*12+0.5) != 0 && note_index_h2+fft_size-1 <= samples_in_note_h2) //if theres an unexpected note change (ie not at the interval given by the user)
                    {
                        if(j>=hop_size)
                        {
                            output_buffer_h2[outIndex][0]=new_section_h2[j].real();
                            output_buffer_h2[outIndex][1]=1;

                        }
                    }
                    else
                    {
                        output_buffer_h2[outIndex][0]+=new_section_h2[j].real();
                        output_buffer_h2[outIndex][1]++;
                    }
                }
                if(note_index_h3+j < samples_in_note_h3 && note_index_h3+j >= 0 && harmonies >= 3)
                {
                     if(floor(log2(previous_note/current_note)*12+0.5) != 0 && note_index_h3+fft_size-1 <= samples_in_note_h3) //if theres an unexpected note change (ie not at the interval given by the user)
                    {
                        if(j>=hop_size)
                        {
                            output_buffer_h3[outIndex][0]=new_section_h3[j].real();
                            output_buffer_h3[outIndex][1]=1;

                        }
                    }
                    else
                    {
                        output_buffer_h3[outIndex][0]+=new_section_h3[j].real();
                        output_buffer_h3[outIndex][1]++;
                    }
                }
                output_buffer[outIndex][0]+=new_section[j].real();
                output_buffer[outIndex][1]++;
                outIndex = incrementCircularBuffer(outIndex,1, buffer_size);
                
            }
            previous_note=current_note;
            out_write_index = incrementCircularBuffer(out_write_index, hop_size, buffer_size);
            // increment note in user's list of notes
            if(start)
            {
                note_index_h1+=hop_size;
                note_index_h2+=hop_size;
                note_index_h3+=hop_size;
                if(note_index_h1 + fft_size >= samples_in_note_h1 && harmonies >= 1)
                {
                    note_index_h1 = -hop_size;
                    h1_index++;
                    if(h1_index >= harmony_1.size())
                    {
                        harmonies = -1;
                    }
                }
                if(note_index_h2 + fft_size >= samples_in_note_h2 && harmonies >= 2)
                {
                    note_index_h2 = -hop_size;
                    h2_index++;
                }
                if(note_index_h3 + fft_size >= samples_in_note_h3 && harmonies >= 3)
                {
                    note_index_h3 = -hop_size;
                    h3_index++;
                }

            }
            
        }
        //copy into new wav file
        newaudioFile.samples[0][i] = outSample;
        newaudioFile.samples[1][i] = outSample;
    
    newaudioFile.save("twinkle_twinkle_3.wav", AudioFileFormat::Wave);
    return 0;




}

//setup buffers, harmonies, etc
void setup(double* input_buffer, double output_buffer[8192][2], double output_buffer_h1[8192][2], double output_buffer_h2[8192][2], double output_buffer_h3[8192][2], double* phases_in, double* phases_out_h1, double* phases_out_h2, double* phases_out_h3)
{
    for(int i=0;i<buffer_size; i++)
    {
        input_buffer[i]=0;
        output_buffer[i][0]=0;
        output_buffer[i][1]=0;

        output_buffer_h1[i][0]=0;
        output_buffer_h1[i][1]=0;

        output_buffer_h2[i][0]=0;
        output_buffer_h2[i][1]=0;

        output_buffer_h3[i][0]=0;
        output_buffer_h3[i][1]=0;
    }
    
    for(int i=0; i<fft_size; i++)
    {
        phases_in[i] = 0;
        phases_out_h1[i] = 0;
        phases_out_h2[i] = 0;
        phases_out_h3[i] = 0;
    }
    harmony c;
    c.freq = 164.8;
    c.length = 2;
    harmony_1.push_back(c);
    harmony_1.push_back(c);
    harmony e;
    e.freq = 261.626;
    e.length=2;
    harmony_1.push_back(e);
    harmony_1.push_back(e);
    harmony_1.push_back(e);
    harmony_1.push_back(e);
    e.length=4;
    harmony_1.push_back(e);
    harmony f;
    f.freq = 220;
    f.length =2;
    harmony_1.push_back(f);
    harmony_1.push_back(f);
    harmony g;
    g.freq = 195.998;
    g.length = 2;
    harmony_1.push_back(g);
    harmony_1.push_back(g);
    harmony_1.push_back(g);
    harmony_1.push_back(g);
    c.length=4;
    harmony_1.push_back(c);

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


double processing(complex<double>* section_fft, complex<double>* section_fft_h1, complex<double>* section_fft_h2, complex<double>* section_fft_h3, double* last_phases_in, double* last_phases_out_h1, double* last_phases_out_h2, double* last_phases_out_h3)
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
    //calculate how many semitones needed for harmony pitch shift
    int semitones_h1 = 0;
    int semitones_h2 = 0;
    int semitones_h3 = 0;
    if(harmonies >= 1)
    {
        semitones_h1 = floor(log2(fundamental_freq/harmony_1.at(h1_index).freq)*12+0.5);
    }
    if(harmonies >= 2)
    {
        semitones_h2 = floor(log2(fundamental_freq/harmony_2.at(h2_index).freq)*12+0.5);
    }
    if(harmonies >= 3)
    {
        semitones_h3 = floor(log2(fundamental_freq/harmony_3.at(h3_index).freq)*12+0.5);
    }

    double pitch_shift_h1=pow(2.0, (double)(semitones_h1)/12.0);
    double pitch_shift_h2=pow(2.0, (double)(semitones_h2)/12.0);
    double pitch_shift_h3=pow(2.0, (double)(semitones_h3)/12.0);
    //cout << start << " " << semitones_h1 << endl;


    double new_magnitudes_h1[fft_size];
    double new_frequencies_h1[fft_size];
    double new_magnitudes_h2[fft_size];
    double new_frequencies_h2[fft_size];
    double new_magnitudes_h3[fft_size];
    double new_frequencies_h3[fft_size];
    for(int i=0; i< fft_size; i++)
    {
        if(harmonies >= 1)
        {
            new_magnitudes_h1[i] = 0;
            new_frequencies_h1[i] = 0;
        }
        if(harmonies >= 2)
        {
            new_magnitudes_h2[i] = 0;
            new_frequencies_h2[i] = 0;
        }
        if(harmonies >= 3)
        {
            new_magnitudes_h3[i] = 0;
            new_frequencies_h3[i] = 0;
        }
    }
    //choose sound ratio for harmonies
    for(int i=0; i<fft_size/2; i++)
    {
        double sound_ratio_h1;
        double sound_ratio_h2;
        double sound_ratio_h3;
        if(start==false)
        {
            sound_ratio_h2=0.1;
            sound_ratio_h3=0.1;
            sound_ratio_h1=0.1;
        }
        else
        {
            if(harmonies >= 1)
            {
                sound_ratio_h1=1.5;
            }
            if(harmonies >= 2)
            {
                sound_ratio_h2=2.0;
            }
            if(harmonies >= 3)
            {
                sound_ratio_h3=0.7;
            }
            
        }
        //multiply freq by pitch shift rato and reset section to match new frequencies and magnitudes
        int new_index_h1 = floor(i*pitch_shift_h1+ 0.5);

        if(new_index_h1 <= fft_size/2 && harmonies >= 1)
        {
            new_magnitudes_h1[new_index_h1] = magnitudes[i]*sound_ratio_h1;
            new_frequencies_h1[new_index_h1] = frequencies[i]*pitch_shift_h1;
        }

        int new_index_h2 = floor(i*pitch_shift_h2+ 0.5);

        if(new_index_h2 <= fft_size/2 && harmonies >= 2) 
        {
            new_magnitudes_h2[new_index_h2] = magnitudes[i]*sound_ratio_h2;
            new_frequencies_h2[new_index_h2] = frequencies[i]*pitch_shift_h2;
        }

        int new_index_h3 = floor(i*pitch_shift_h3+ 0.5);

        if(new_index_h3 <= fft_size/2 && harmonies >= 3)
        {
            new_magnitudes_h3[new_index_h3] = magnitudes[i]*sound_ratio_h3;
            new_frequencies_h3[new_index_h3] = frequencies[i]*pitch_shift_h3;
        }
    }
    //convert magnitude and frequency back into complex number
    for(int i=0; i<fft_size/2; i++)
    {
        double bin_centre_freq = (2*M_PI)*(double)i/(double)fft_size;
        if(harmonies >= 1)
        {
            double amplitude_h1 = new_magnitudes_h1[i];
            double bin_dev_h1 = new_frequencies_h1[i] - (double)i;
            
            double phase_diff_h1 = bin_dev_h1*(2*M_PI)*(double)hop_size/(double)fft_size;
            phase_diff_h1 += bin_centre_freq*(double)hop_size;
            double phase_out_h1 = wrap(last_phases_out_h1[i] + phase_diff_h1);

            section_fft_h1[i] = complex<double>(amplitude_h1*cos(phase_out_h1),amplitude_h1*sin(phase_out_h1));

            if(i>0 && i < fft_size/2)
            {
                section_fft_h1[fft_size-i] = complex<double>(amplitude_h1*cos(phase_out_h1),-(amplitude_h1*sin(phase_out_h1)));
            }

            last_phases_out_h1[i] = phase_out_h1;
        }
        if(harmonies >= 2)
        {
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
        }
        if(harmonies >= 3)
        {
            double amplitude_h3 = new_magnitudes_h3[i];
            double bin_dev_h3 = new_frequencies_h3[i] - (double)i;
            
            double phase_diff_h3 = bin_dev_h3*(2*M_PI)*(double)hop_size/(double)fft_size;
            phase_diff_h3 += bin_centre_freq*(double)hop_size;
            double phase_out_h3 = wrap(last_phases_out_h3[i] + phase_diff_h3);

            section_fft_h3[i] = complex<double>(amplitude_h3*cos(phase_out_h3),amplitude_h3*sin(phase_out_h3));

            if(i>0 && i < fft_size/2)
            {
                section_fft_h3[fft_size-i] = complex<double>(amplitude_h3*cos(phase_out_h3),-(amplitude_h3*sin(phase_out_h3)));
            }

            last_phases_out_h3[i] = phase_out_h3;
        }   
    }
    return fundamental_freq;
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