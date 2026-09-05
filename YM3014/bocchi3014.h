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

#ifndef BOCCHI3014_H
#define BOCCHI3014_H

#include <iostream>
#include <cstdint>
#include <array>
using namespace std;

namespace bocchi3014
{
    struct Bocchi3014Pins
    {
	bool pin_clock = false;
	bool pin_load = false;
	bool pin_sd = false;
    };

    class Bocchi3014
    {
	public:
	    Bocchi3014();
	    ~Bocchi3014();

	    void init();
	    void tickCLK(bool clk);

	    void setSampleRates(uint32_t clock_rate, uint32_t sample_rate);
	    bool isValidSample();
	    int16_t getSample();

	    Bocchi3014Pins &getPins()
	    {
		return current_pins;
	    }

	private:
	    template<typename T>
	    bool testbit(T reg, int bit)
	    {
		return ((reg >> bit) & 0x1) ? true : false;
	    }

	    Bocchi3014Pins current_pins;

	    int64_t sample_divider = 0;
	    int64_t counter = 0;

	    bool valid_sample = false;

	    void tickValidSample();
	    void tickInternal();

	    bool clk_rise = false;
	    bool prev_clk = false;

	    bool prev_clock = false;

	    bool load_val = false;
	    bool prev_load = false;

	    int16_t final_sample = 0;
	    int16_t output = 0;

	    int16_t calcSample(uint16_t latch);

	    uint16_t sample_sr = 0;
	    uint16_t sample_latch = 0;

	    static constexpr int num_frac_bits = 12;

    };

};

#endif // BOCCHI3014_H