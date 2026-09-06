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
	    void tick();
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
	    void tickPhase();
	    void tickOp();
	    /*
	    void tickAcc();
	    */

	    bool phim = false;

	    bool mrst = false;

	    bool clk_rise = false;
	    bool clk_fall = false;
	    bool prev_clk = false;

	    bool is_rst1 = false;
	    bool is_rst2 = false;

	    bool phi1_dff_q = false;

	    bool phi1_rise = false;
	    bool phi1_fall = false;

	    uint8_t prev_timing_counter = 0;
	    uint8_t timing_counter = 0;

	    bool addr_ld = false;
	    bool addr_ld_next = false;

	    bool addr_latch[3] = {false, false, false};
	    bool data_latch[3] = {false, false, false};

	    uint8_t addr_val = 0;

	    uint8_t hireg_counter = 0;

	    uint8_t busy_counter = 0;
	    bool is_busy_cnt = false;
	    bool is_busy_full = false;
	    bool is_busy_ov = false;
	    bool is_write_busy = false;

	    uint8_t data_in = 0;
	    uint8_t data_in_temp = 0;

	    bool reg_addr_ready = false;
	    uint8_t reg_addr_val = 0;
	    bool reg_data_ready = false;
	    uint8_t reg_data_val = 0;

	    bool loreg_addr_valid = false;

	    uint16_t calcKCode();

	    uint16_t lfp_deviance = 0;
	    bool lfp_sign = false;
	    uint8_t out_kc = 0;
	    uint8_t out_kf = 0;

	    array<uint16_t, 32> pg_fnum = {{0}};
	    array<uint8_t, 32> pg_kcode = {{0}};

	    uint16_t op_phase_in = 0;
	    uint16_t op_mod_in = 0;

	    array<uint32_t, 32> pg_phase = {{0}};
	    array<uint32_t, 32> pg_delta = {{0}};

	    array<uint8_t, 8> channel_rl = {{0}};
	    array<uint8_t, 8> channel_fb = {{0}};
	    array<uint8_t, 8> channel_alg = {{0}};

	    array<uint8_t, 8> channel_kc = {{0}};
	    array<uint8_t, 8> channel_kf = {{0}};
	    array<uint8_t, 8> channel_pms = {{0}};
	    array<uint8_t, 8> channel_ams = {{0}};

	    #include "bocchi2151_tables.inl"
    };
};



#endif // BOCCHI2151_H