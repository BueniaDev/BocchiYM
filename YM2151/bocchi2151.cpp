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
	tickAcc();
	tickOp();
	tickEnv();
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

	if (phi1_fall)
	{
	    uint8_t sh_counter = ((timing_counter + 27) & 0x1F);
	    current_pins.pin_sh1 = (((sh_counter & 24) == 8) && !mrst);
	    current_pins.pin_sh2 = (((sh_counter & 24) == 24) && !mrst);
	}

	phi1_rise = (!phi1_dff_q && clk_rise);
	phi1_fall = (phi1_dff_q && clk_rise);

	current_pins.pin_phi1 = !phi1_dff_q;

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
		// cout << "Writing value of " << hex << int(reg_data_val) << " to hi-reg channel address of " << hex << int(reg_addr_val) << endl;

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
		// cout << "Writing value of " << hex << int(reg_data_val) << " to hi-reg operator address of " << hex << int(reg_addr_val) << endl;
	    }
	}

	if (phi1_rise && data_latch[2] && (addr_val < 0x20))
	{
	    // cout << "Writing value of " << hex << int(data_in) << " to address of " << hex << int(addr_val) << endl;

	    switch (addr_val)
	    {
		case 0x08:
		{
		    for (int i = 0; i < 4; i++)
		    {
			mode_kon_oper.at(i) = testbit(data_in, (3 + i));
		    }

		    mode_kon_ch = (data_in & 0x7);
		}
		break;
	    }
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

	if (phi1_fall)
	{
	    uint8_t slot = ((timing_counter + 8) & 0x1F);

	    if (kon_chmatch)
	    {
		mode_kon.at(slot) = mode_kon_oper.at(0);
		mode_kon.at((slot + 8) & 0x1F) = mode_kon_oper.at(2);
		mode_kon.at((slot + 16) & 0x1F) = mode_kon_oper.at(1);
		mode_kon.at((slot + 24) & 0x1F) = mode_kon_oper.at(3);
	    }
	}

	if (phi1_fall)
	{
	    uint8_t cycles = ((timing_counter + 1) & 0x1F);
	    kon_chmatch = false;

	    if ((mode_kon_ch + 24) == cycles)
	    {
		kon_chmatch = true;
	    }
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
	    pg_reset_latch.at(slot) = pg_reset.at(slot);
	    slot = ((timing_counter + 25) & 0x1F);

	    if (pg_reset_latch.at(slot))
	    {
		pg_delta.at(slot) = 0;
	    }

	    slot = ((timing_counter + 24) & 0x1F);

	    if (pg_reset_latch.at(slot))
	    {
		pg_phase.at(slot) = 0;
	    }

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

    void Bocchi2151::tickEnv()
    {
	// TODO: Finish implementing this

	// Temporary mrst setting until envelope generator is properly implemented
	if (mrst)
	{
	    eg_level.fill(0x3FF);
	}

	if (phi1_fall)
	{
	    eg_out.at(1) = eg_out.at(0);
	}

	if (phi1_fall)
	{
	    uint8_t slot = ((timing_counter + 29) & 0x1F);
	    uint32_t level = eg_level.at(slot);

	    if (eg_mute || mrst)
	    {
		level = 0x3FF;
	    }

	    // Temporary setting until envelope generator is properly implemented
	    if (pg_reset.at(slot))
	    {
		level = 0;
	    }

	    eg_level.at(slot) = uint16_t(level);

	    eg_out.at(0) = eg_out_temp.at(1);

	    if (testbit(eg_out.at(0), 10))
	    {
		eg_out.at(0) = 1023;
	    }
	}

	if (phi1_fall)
	{
	    uint8_t slot = ((timing_counter + 30) & 0x1F);

	    bool kon = (key_on.at(slot) && !key_on2.at(slot));
	    pg_reset.at(slot) = kon;

	    bool eg_off = ((eg_level.at(slot) & 0x3F0) == 0x3F0);
	    eg_mute = (eg_off && !kon);
	}

	if (phi1_fall)
	{
	    uint8_t slot = ((timing_counter + 31) & 0x1F);

	    eg_out_temp.at(1) = eg_out_temp.at(0);
	    eg_out_temp.at(0) = eg_level.at(slot);

	    if (testbit(eg_out_temp.at(0), 10))
	    {
		eg_out_temp.at(0) = 1023;
	    }
	}

	if (phi1_fall)
	{
	    uint8_t slot = ((timing_counter + 2) & 0x1F);
	    bool kon = mode_kon.at(slot);

	    key_on2.at(slot) = key_on.at(slot);
	    key_on.at(slot) = kon;
	}
    }

    void Bocchi2151::tickOp()
    {
	// TODO: Finish implementing this

	// Cycle 53 (WIP)
	if (phi1_fall)
	{
	    uint8_t slot = ((timing_counter + 19) & 0x1F);
	    uint8_t channel = (slot & 0x7);
	    uint8_t rl = channel_rl.at(channel);
	    op_out[5] = op_out[4];
	    op_mix = op_out[4];
	    op_mixl = testbit(rl, 0);
	    op_mixr = testbit(rl, 1);
	}

	// Cycle 52 (WIP)
	if (phi1_fall)
	{
	    op_out[4] = op_out[3];
	}

	// Cycles 50-51
	if (phi1_fall)
	{
	    op_out[3] = op_out[2];
	    op_out[2] = op_out[1];
	}

	// Cycle 49
	if (phi1_fall)
	{
	    int16_t out = op_out[0];

	    if (testbit(op_sign, 6))
	    {
		out ^= 0x3FFF;
		out = ((out + 1) & 0x3FFF);
	    }

	    out <<= 2;
	    out >>= 2;
	    op_out[1] = out;
	}

	// Cycle 48
	if (phi1_fall)
	{
	    int16_t out = ((op_exp[1] << 2) >> op_pow[1]);
	    op_out[0] = out;
	}

	// Cycles 46-47
	if (phi1_fall)
	{
	    op_exp[1] = op_exp[0];
	    op_pow[1] = op_pow[0];

	    op_exp[0] = exp_table.at(op_atten & 0xFF);
	    op_pow[0] = (op_atten >> 8);
	}

	// Cycle 45
	if (phi1_fall)
	{
	    op_atten = (op_logsin[2] + (eg_out[1] << 2));

	    if (testbit(op_atten, 12))
	    {
		op_atten = 4095;
	    }
	}

	// Cycles 42-44
	if (phi1_fall)
	{
	    op_logsin[2] = op_logsin[1];
	    op_logsin[1] = op_logsin[0];

	    uint16_t phase = (op_phase & 0xFF);

	    if (testbit(op_phase, 8))
	    {
		phase ^= 0xFF;
	    }

	    op_logsin[0] = sine_table.at(phase);
	    op_sign = ((op_sign << 1) | testbit(op_phase, 9));
	}

	// Cycle 41
	if (phi1_fall)
	{
	    op_phase = ((op_phase_in + op_mod_in) & 0x3FF);
	}

	// Cycle 40
	if (phi1_fall)
	{
	    uint8_t slot = timing_counter;
	    op_phase_in = (pg_phase.at(slot) >> 10);
	    op_mod_in = 0;
	}
    }

    void Bocchi2151::tickAcc()
    {
	// TODO: Finish implementing this

	if (phi1_fall)
	{
	    current_pins.pin_so = sound_out;
	}

	if (phi1_fall)
	{
	    sound_out = false;

	    switch (timing_counter & 0xF)
	    {
		case 0: sound_out = mix_sign_lock; break;
		case 1: sound_out = testbit(mix_exp_lock, 0); break;
		case 2: sound_out = testbit(mix_exp_lock, 1); break;
		case 3: sound_out = testbit(mix_exp_lock, 2); break;
		default:
		{
		    if (mix_exp_lock != 0)
		    {
			sound_out = testbit(mix_bits, (mix_exp_lock - 1));
		    }
		}
		break;
	    }
	}

	if (phi1_fall)
	{
	    uint8_t slot = ((timing_counter + 30) & 0x1F);

	    bool bit = false;

	    if (slot < 16)
	    {
		bit = mix_left_stream[3];
	    }
	    else
	    {
		bit = mix_right_stream[3];
	    }

	    if ((timing_counter & 0xF) == 1)
	    {
		mix_top_bits = (((mix_bits >> 15) & 0x3F) | (bit << 6));
	    }

	    if ((timing_counter & 0xF) == 7)
	    {
		uint8_t top = (mix_top_bits & 0x3F);
		uint8_t ex = 0;

		if (!testbit(mix_top_bits, 6))
		{
		    top ^= 63;
		}

		if (testbit(top, 5))
		{
		    ex = 7;
		}
		else if (testbit(top, 4))
		{
		    ex = 6;
		}
		else if (testbit(top, 3))
		{
		    ex = 5;
		}
		else if (testbit(top, 2))
		{
		    ex = 4;
		}
		else if (testbit(top, 1))
		{
		    ex = 3;
		}
		else if (testbit(top, 0))
		{
		    ex = 2;
		}
		else
		{
		    ex = 1;
		}

		mix_sign_lock = testbit(mix_top_bits, 6);
		mix_exp_lock = ex;
	    }

	    mix_bits = ((mix_bits >> 1) | (bit << 20));
	}

	if (phi1_fall)
	{
	    mix_left_stream[3] = mix_left_stream[2];
	    mix_left_stream[2] = mix_left_stream[1];
	    mix_left_stream[1] = mix_left_stream[0];

	    mix_right_stream[3] = mix_right_stream[2];
	    mix_right_stream[2] = mix_right_stream[1];
	    mix_right_stream[1] = mix_right_stream[0];
	}

	if (phi1_fall)
	{
	    switch (mix_sat_ctrl[0])
	    {
		case 0:
		case 7: mix_left_stream[0] = testbit(mix_piso[0], 0); break;
		case 1:
		case 2:
		case 3: mix_left_stream[0] = true; break;
		case 4:
		case 5:
		case 6: mix_left_stream[0] = false; break;
	    }

	    switch (mix_sat_ctrl[1])
	    {
		case 0:
		case 7: mix_right_stream[0] = testbit(mix_piso[1], 0); break;
		case 1:
		case 2:
		case 3: mix_right_stream[0] = true; break;
		case 4:
		case 5:
		case 6: mix_right_stream[0] = false; break;
	    }
	}

	if (phi1_fall)
	{
	    if (timing_counter == 13)
	    {
		mix_piso[1] = ((!testbit(mix_accum[1], 17) << 15) | (mix_accum[1] & 0x7FFF));
		mix_sat_ctrl[1] = ((mix_accum[1] >> 15) & 0x7);
	    }
	    else
	    {
		mix_piso[1] >>= 1;
	    }

	    if (timing_counter == 29)
	    {
		mix_piso[0] = ((!testbit(mix_accum[0], 17) << 15) | (mix_accum[0] & 0x7FFF));
		mix_sat_ctrl[0] = ((mix_accum[0] >> 15) & 0x7);
	    }
	    else
	    {
		mix_piso[0] >>= 1;
	    }
	}

	if (phi1_fall)
	{
	    if (timing_counter == 13)
	    {
		mix_accum[1] = (op_mixr) ? int32_t(op_mix) : 0;
	    }
	    else
	    {
		mix_accum[1] += int32_t(op_mix);
	    }

	    if (timing_counter == 29)
	    {
		mix_accum[0] = (op_mixl) ? int32_t(op_mix) : 0;
	    }
	    else if (op_mixl)
	    {
		mix_accum[0] += int32_t(op_mix);
	    }
	}
    }
};