/*
    This file is part of the BocchiYM family of cycle-accurate Yamaha FM sound chip emulators.
    Copyright (C) 2026 BueniaDev.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// BocchiYM3014
// Chip Name: YM3014 (1-channel floating point DAC)
//
// Bocchi's Notes:
// This file contains the implementation of the YM3014 DAC.
// Like with a real YM3014, it is typically meant to be used with its corresponding FM sound chip implementation
// (or an equivalent implementation using other libraries).

#include "bocchi3014.h"
using namespace bocchi3014;
using namespace std;

namespace bocchi3014
{
    Bocchi3014::Bocchi3014()
    {
	valid_sample = false;
    }

    Bocchi3014::~Bocchi3014()
    {

    }

    // Initializes the emulated YM3012
    void Bocchi3014::init()
    {
	current_pins = {};
    }

    // Initializes the sample divider variables used for nearest-neighbor resampling
    // (this API function must be called before calling the init function)
    // 
    // Parameters:
    // clock_rate = Desired clock rate of the emulated YM3014, typically 3579545 (3.579545 mHZ)
    // sample_rate = Desired output sample rate (must not be 0)
    void Bocchi3014::setSampleRates(uint32_t clock_rate, uint32_t sample_rate)
    {
	if (sample_rate == 0)
	{
	    cout << "Invalid sample rate detected, choose a different sample rate!" << endl;
	    throw runtime_error("Bocchi3014 error");
	}

	sample_divider = int64_t((float(clock_rate) / float(sample_rate)) * (1 << num_frac_bits));
	counter = sample_divider;
    }

    // Ticks the emulated YM3014 forward one-half clock cycle
    //
    // Parameters:
    // clk = Value of clock cycle pulse (either true or false)
    //
    // Sample psuedo code for ticking the emulated YM3014 for 1 clock cycle:
    //
    // tickCLK(true)
    // tickCLK(false)
    void Bocchi3014::tickCLK(bool clk)
    {
	clk_rise = (!prev_clk && clk);

	if (clk_rise)
	{
	    tickInternal();
	}

	tickValidSample();
	prev_clk = clk;
    }

    void Bocchi3014::tickInternal()
    {
	if (!prev_clock && current_pins.pin_clock)
	{
	    sample_sr = ((sample_sr >> 1) | (current_pins.pin_sd << 13));
	}

	if (!prev_clock && current_pins.pin_clock)
	{
	    load_val = current_pins.pin_load;
	}

	if (prev_clock && !current_pins.pin_clock)
	{
	    prev_load = load_val;
	}

	if (prev_load && !load_val)
	{
	    sample_latch = (sample_sr & 0x1FFF);
	}

	prev_clock = current_pins.pin_clock;

	output = calcSample(sample_latch);
    }

    int16_t Bocchi3014::calcSample(uint16_t latch)
    {
	int exp = ((latch >> 10) & 0x7);
	bool sign = testbit(latch, 9);
	uint16_t mant = (latch & 0x1FF);
	uint16_t mask = 0;

	if (exp == 0)
	{
	    return 0;
	}

	if (!sign)
	{
	    mask = 0xFFFF;
	}

	mant ^= (mask & 0x1FF);
	return (((mant << exp) >> 1) ^ mask);
    }

    void Bocchi3014::tickValidSample()
    {
	if (clk_rise)
	{
	    counter -= (1 << num_frac_bits);

	    if (counter <= 0)
	    {
		counter += sample_divider;
		final_sample = output;
		valid_sample = true;
	    }
	}
    }

    // Returns true if a resampled audio sample is available
    bool Bocchi3014::isValidSample()
    {
	bool is_valid_sample = valid_sample;
	valid_sample = false;
	return is_valid_sample;
    }

    // Returns the value of the resampled audio samples (as a single 16-bit mono sample)
    int16_t Bocchi3014::getSample()
    {
	return final_sample;
    }
};