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

#ifndef BOCCHI3012_H
#define BOCCHI3012_H

#include <iostream>
#include <cstdint>
#include <array>
using namespace std;

namespace bocchi3012
{
    struct Bocchi3012Pins
    {
	bool pin_nicl = true;
	bool pin_sy = false;
	bool pin_sh1 = false;
	bool pin_sh2 = false;
	bool pin_so = false;
    };

    class Bocchi3012
    {
	public:
	    Bocchi3012();
	    ~Bocchi3012();

	    void init();
	    void reset();
	    void tickCLK(bool clk);

	    void setSampleRates(uint32_t clock_rate, uint32_t sample_rate);
	    bool isValidSample();
	    array<int16_t, 2> getSamples();

	    Bocchi3012Pins &getPins()
	    {
		return current_pins;
	    }

	private:
	    template<typename T>
	    bool testbit(T reg, int bit)
	    {
		return ((reg >> bit) & 0x1) ? true : false;
	    }

	    Bocchi3012Pins current_pins;

	    int64_t sample_divider = 0;
	    int64_t counter = 0;

	    bool valid_sample = false;

	    void tickValidSample();

	    array<int16_t, 2> final_samples;
	    array<int16_t, 2> output;

	    uint16_t reg_sr = 0;

	    uint16_t left_sr = 0;
	    uint16_t right_sr = 0;

	    uint16_t left_latch = 0;
	    uint16_t right_latch = 0;

	    bool prev_clk = false;
	    bool prev_res = true;

	    bool clk_rise = false;

	    bool sh1_val = false;
	    bool sh2_val = false;

	    bool prev_sy = true;
	    bool prev_sh1 = false;
	    bool prev_sh2 = false;

	    void tickInternal();

	    int16_t calcSample(uint16_t latch);

	    static constexpr int num_frac_bits = 12;
    };
};



#endif // BOCCHI3012_H