/*
    This file is part of the BocchiYM family of cycle-accurate Yamaha FM sound chip emulators.
    Copyright (C) 2024 BueniaDev.

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

#ifndef BOCCHI2151_H
#define BOCCHI2151_H

#include <iostream>
#include <cstdint>
#include <array>
#include <algorithm>
using namespace std;

namespace bocchi2151
{
    struct Bocchi2151Pins
    {
	bool pin_phi1 = true;
	bool pin_irqn = true;
	bool pin_icn = true;
	bool pin_csn = true;
	bool pin_a0 = false;
	uint8_t data = 0;
	bool pin_rdn = true;
	bool pin_wrn = true;
	bool pin_sh1 = false;
	bool pin_sh2 = false;
	bool pin_so = false;
	bool pin_ct1 = false;
	bool pin_ct2 = false;
    };

    class Bocchi2151
    {
	public:
	    Bocchi2151();
	    ~Bocchi2151();

	    void init();
	    void tickCLK(bool clk);

	    Bocchi2151Pins &getPins()
	    {
		return current_pins;
	    }

	private:
	    template<typename T>
	    bool testbit(T reg, int bit)
	    {
		return ((reg >> bit) & 0x1) ? true : false;
	    }

	    void reset();

	    Bocchi2151Pins current_pins;

	    void tickInternal();
	    void tickTimingGen();
	    void tickReg();

	    bool clk_val = false;

	    bool prev_clk = false;
	    bool ic_val = false;
	    bool prev_ic = false;

	    bool clk_rise = false;
	    bool clk_fall = false;

	    bool is_reset = false;
	    bool is_res_edge = true;

	    int reset_sr = 0;

	    bool phi1p = true;
	    bool phi1n = true;

	    bool phi1_rise = false;
	    bool phi1_fall = false;

	    bool synced_mrst_n = false;
	    bool mrst_n = false;

	    int timing_ctr = 0;

	    bool cycle_01 = false;
	    bool cycle_31 = false;

	    bool areg_rq_latch = false;
	    bool dreg_rq_latch = false;

	    array<bool, 3> areg_rq_synced = {{false}};
	    array<bool, 3> dreg_rq_synced = {{false}};

	    bool addr_ld = false;
	    bool data_ld = false;

	    uint8_t dbus_temp = 0;
	    uint8_t dbus_latch = 0;

	    bool write_busy = false;

	    uint8_t busy_cntr = 0;
	    bool busycntr_cnt = false;
	    bool busycntr_ovfl = false;
	    bool busyctr_full = false;

	    bool loreg_addr_valid = false;
	    bool loreg_data_en = false;
	    uint8_t loreg_addr = 0;

	    bool noise_enable = false;
	    uint8_t noise_freq = 0;

	    bool timer_a_run = false;
	    bool timer_b_run = false;
	    bool timer_a_irq_en = false;
	    bool timer_b_irq_en = false;
	    bool csm_reg = false;

	    uint8_t key_on_temp = 0;

	    uint8_t lfo_freq = 0;

	    uint8_t pms_data = 0;
	    uint8_t ams_data = 0;

	    #include "opm_tables.inl"
    };
};



#endif // BOCCHI2151_H