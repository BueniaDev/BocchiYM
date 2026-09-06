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

// BocchiYM2151 (WIP)
// Chip Name: YM2151 (8-channel, 4-operator FM sound chip)
//
// Bocchi's Notes:
// This file contains the implementation of the YM2151 sound IC.
// Please note that as this implementation is currently a huge WIP, lots of features are currently unimplemented.

#include "bocchi2151.h"
using namespace bocchi2151;
using namespace std;

namespace bocchi2151
{
    Bocchi2151::Bocchi2151()
    {

    }

    Bocchi2151::~Bocchi2151()
    {

    }

    void Bocchi2151::init()
    {
	current_pins = {};
    }

    void Bocchi2151::tick()
    {
	tickCLK(true);
	tickCLK(false);
    }

    void Bocchi2151::tickCLK(bool clk)
    {
	phim = clk;
	clk_rise = (!prev_clk && clk);
	clk_fall = (prev_clk && !clk);

	if (prev_clk != clk)
	{
	    tickInternal();
	}

	prev_clk = clk;
    }

    // Internal clock tick function
    void Bocchi2151::tickInternal()
    {
	// TODO: Finish implementing this
	tickOp();
	tickPhase();
	tickReg();
	tickTimingGen();
    }

    // Tick function for timing generator
    void Bocchi2151::tickTimingGen()
    {
	if (phi1_fall)
	{
	    mrst = !current_pins.pin_icn;
	}

	if (clk_fall)
	{
	    is_rst1 = current_pins.pin_icn;
	}

	if (clk_rise)
	{
	    is_rst2 = is_rst1;
	}

	if (clk_rise)
	{
	    phi1_dff_q = ((is_rst2 && !current_pins.pin_icn) || !phi1_dff_q);
	} 

	phi1_rise = (phi1_dff_q && clk_rise);
	phi1_fall = (!phi1_dff_q && clk_rise);

	current_pins.pin_phi1 = phi1_dff_q;

	if (phi1_fall)
	{
	    uint8_t sh_counter = ((timing_counter + 27) % 32);
	    current_pins.pin_sh1 = (((sh_counter & 24) == 8) || mrst);
	    current_pins.pin_sh2 = (((sh_counter & 24) == 24) || mrst);
	}

	if (phi1_fall)
	{
	    if (mrst)
	    {
		timing_counter = 0;
	    }
	    else
	    {
		timing_counter = ((timing_counter + 1) & 0x1F);
	    }
	}
    }

    // Tick function for registers
    void Bocchi2151::tickReg()
    {
	// TODO: Finish implementing this
	if (!current_pins.pin_csn && !current_pins.pin_wrn)
	{
	    data_in_temp = current_pins.data;
	}

	if (phi1_fall)
	{
	    if (mrst)
	    {
		data_in = 0;
	    }
	    else if (addr_latch[1] || data_latch[1])
	    {
		data_in = data_in_temp;
	    }
	}

	if (addr_latch[2])
	{
	    addr_val = data_in;
	}

	if (phi1_rise)
	{
	    if (mrst)
	    {
		busy_counter = 0;
	    }
	    else if (is_busy_cnt)
	    {
		busy_counter = ((busy_counter + 1) & 0x1F);
	    }
	}

	if (phi1_rise)
	{
	    is_write_busy = ((is_write_busy && !(mrst || (is_busy_full && is_busy_cnt))) || data_latch[2]);
	}

	if (phi1_fall)
	{
	    is_busy_full = (busy_counter == 0x1F);
	    is_busy_cnt = is_write_busy;
	}

	if (phi1_fall && reg_data_ready)
	{
	    uint8_t slot = timing_counter;
	    uint8_t channel = (slot & 7);

	    if ((reg_addr_val & 0xE7) == (0x20 | channel))
	    {
		cout << "Writing value of " << hex << int(reg_data_val) << " to hi-reg channel address of " << hex << int(reg_addr_val) << endl;

		uint8_t ch_addr = ((reg_addr_val >> 3) & 0x3);

		switch (ch_addr)
		{
		    case 0:
		    {
			channel_rl.at(channel) = ((reg_data_val >> 6) & 0x3);
			channel_fb.at(channel) = ((reg_data_val >> 3) & 0x7);
			channel_alg.at(channel) = (reg_data_val & 0x7);
		    }
		    break;
		    case 1:
		    {
			channel_kc.at(channel) = (reg_data_val & 0x7F);
		    }
		    break;
		    case 2:
		    {
			channel_kf.at(channel) = ((reg_data_val >> 2) & 0x3F);
		    }
		    break;
		    case 3:
		    {
			channel_pms.at(channel) = ((reg_data_val >> 4) & 0x7);
			channel_ams.at(channel) = (reg_data_val & 0x3);
		    }
		    break;
		}
	    }
	    else if ((reg_addr_val & 0x1F) == slot)
	    {
		cout << "Writing value of " << hex << int(reg_data_val) << " to hi-reg operator address of " << hex << int(reg_addr_val) << endl;
	    }
	}

	if (phi1_rise && data_latch[2] && (addr_val < 0x20))
	{
	    cout << "Writing value of " << hex << int(data_in) << " to address of " << hex << int(addr_val) << endl;
	}

	if (phi1_rise)
	{
	    reg_data_ready = (reg_data_ready && !addr_latch[2]);

	    if (reg_addr_ready && data_latch[2])
	    {
		reg_data_val = data_in;
		reg_data_ready = true;
	    }
	}

	if (phi1_fall)
	{
	    reg_addr_ready = (reg_addr_ready && !addr_latch[2]);

	    if (addr_latch[2] && ((addr_val & 0xE0) != 0))
	    {
		reg_addr_val = addr_val;
		reg_addr_ready = true;
	    }
	}

	bool addr_set = (mrst || (!current_pins.pin_a0 && !current_pins.pin_wrn && !current_pins.pin_csn));
	bool data_set = (current_pins.pin_a0 && !current_pins.pin_wrn && !current_pins.pin_csn && !mrst);

	if (phi1_fall)
	{
	    addr_latch[0] = (addr_set && !addr_latch[1]);
	    addr_latch[2] = addr_latch[1];

	    data_latch[0] = (data_set && !data_latch[1]);
	    data_latch[2] = data_latch[1];
	}

	if (phi1_rise)
	{
	    addr_latch[1] = addr_latch[0];
	    data_latch[1] = data_latch[0];
	}

	if (!current_pins.pin_csn && !current_pins.pin_rdn && current_pins.pin_a0 && !mrst)
	{
	    current_pins.data = (is_write_busy << 7);
	}
    }

    uint16_t Bocchi2151::calcKCode()
    {
	uint16_t lfp_val = (lfp_deviance & 0x1FFF);
	if (lfp_sign)
	{
	    lfp_val = (~lfp_val & 0x1FFF);
	}

	uint16_t freq_kcode = ((out_kc << 6) | out_kf);
	uint16_t freq_sum = (freq_kcode + lfp_val + lfp_sign);
	bool freq_overflow = testbit(freq_sum, 13);
	freq_sum &= 0x1FFF;

	uint16_t notegroup_sum = ((freq_kcode & 0xFF) + (lfp_val & 0xFF) + lfp_sign);
	bool notegroup_overflow = testbit(notegroup_sum, 8);

	uint16_t rearranged_sum = freq_sum;

	bool notegroup_no_pitch_mod = (((lfp_val >> 6) & 0x3) == 0);

	if (!lfp_sign && ((((freq_sum >> 6) & 0x3) == 3) || notegroup_overflow))
	{
	    rearranged_sum += 64;
	}

	if (lfp_sign && !notegroup_overflow && !notegroup_no_pitch_mod)
	{
	    rearranged_sum -= 64;
	}

	bool rearranged_overflow = testbit(rearranged_sum, 13);
	rearranged_sum &= 0x1FFF;

	bool sub1 = !(notegroup_no_pitch_mod || notegroup_overflow || !lfp_sign);

	if ((lfp_sign && !freq_overflow) || (sub1 && !rearranged_overflow && (lfp_sign || !freq_overflow)))
	{
	    rearranged_sum = 0;
	}

	if (!lfp_sign && (freq_overflow || rearranged_overflow))
	{
	    rearranged_sum = 8127;
	}

	uint8_t freq_dt2 = 0;

	uint8_t freq_frac = (rearranged_sum & 0x3F);
	uint8_t freq_int = ((rearranged_sum >> 6) & 0x7F);

	switch (freq_dt2)
	{
	    case 2: freq_frac += 52; break;
	    case 3: freq_frac += 32; break;
	    default: break;
	}

	bool freq_frac_carry = testbit(freq_frac, 6);
	freq_frac &= 0x3F;

	uint32_t detune_add_index = ((freq_dt2 << 3) | (freq_frac_carry << 2) | (freq_int & 0x3));

	freq_int += detune_add_table.at(detune_add_index);

	uint16_t final_freq = 0;

	if (testbit(freq_int, 7))
	{
	    final_freq = 8127;
	}
	else
	{
	    final_freq = (((freq_int & 0x7F) << 6) | freq_frac);
	}

	return final_freq;
    }

    // Tick function for phase generator
    void Bocchi2151::tickPhase()
    {
	// TODO: Finish implementing this

	// Cycles 10-17
	if (phi1_fall)
	{
	    uint8_t slot = ((timing_counter + 27) & 0x1F);
	    // phaseReset1();
	    slot = ((timing_counter + 25) & 0x1F);
	    // phaseReset2();
	    slot = ((timing_counter + 24) & 0x1F);
	    pg_phase.at(slot) += pg_delta.at(slot);
	    pg_phase.at(slot) &= 0xFFFFF;
	}

	// Cycles 3-9
	if (phi1_fall)
	{
	    uint8_t slot = timing_counter;
	    uint16_t fnum = pg_fnum.at(slot);
	    uint8_t kcode = pg_kcode.at(slot);
	    uint8_t block = (kcode >> 2);
	    uint32_t freq_num = ((fnum << block) >> 2);
	    pg_delta.at(slot) = (freq_num & 0xFFFFF);
	}

	// Cycles 0-2
	if (phi1_fall)
	{
	    uint8_t slot = ((timing_counter + 7) & 0x1F);
	    uint8_t channel = (slot & 0x7);
	    lfp_deviance = 0;
	    lfp_sign = false;

	    out_kc = channel_kc.at(channel);
	    out_kf = channel_kf.at(channel);
	    uint16_t kcode = calcKCode();
	    pg_fnum.at(slot) = fnum_table.at(kcode & 0x3FF);
	    pg_kcode.at(slot) = (kcode >> 8);
	}

	return;
    }

    void Bocchi2151::tickOp()
    {
	// TODO: Finish implementing this

	// Cycle 40
	if (phi1_fall)
	{
	    uint8_t slot = timing_counter;
	    op_phase_in = (pg_phase.at(slot) >> 10);
	    op_mod_in = 0;
	}
    }
};