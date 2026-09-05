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

    void Bocchi2151::tickCLK(bool clk)
    {
	clk_val = clk;
	clk_rise = (!prev_clk && clk);
	clk_fall = (prev_clk && !clk);
	tickInternal();

	prev_clk = clk;
    }

    // Internal clock tick function
    void Bocchi2151::tickInternal()
    {
	tickTimingGen();
	tickReg();
    }

    // Tick timing generator
    // TODO: Finish implementing this function
    void Bocchi2151::tickTimingGen()
    {
	if (clk_rise)
	{
	    reset_sr = (((reset_sr << 1) | current_pins.pin_icn) & 0x3);
	    is_res_edge = (!testbit(reset_sr, 0) && testbit(reset_sr, 1));
	}

	current_pins.pin_phi1 = phi1p;
	phi1_rise = !(phi1n || !clk_rise);
	phi1_fall = !(phi1p || !clk_rise);

	if (clk_fall || !current_pins.pin_icn)
	{
	    if (is_res_edge || !current_pins.pin_icn)
	    {
		phi1p = true;
		phi1n = true;
	    }
	    else
	    {
		phi1n = phi1p;
		phi1p = !phi1p;
	    }
	}

	if (phi1_fall)
	{
	    synced_mrst_n = testbit(reset_sr, 0);
	}

	mrst_n = (synced_mrst_n && current_pins.pin_icn);
    }

    // Ticks the internal register writes
    // TODO: Implement the following:
    // Write busy flag
    // Status register reads
    // High registers functionality
    // Remaining low registers
    // Dynamic key-on registers
    void Bocchi2151::tickReg()
    {
	addr_ld = areg_rq_synced[2];
	data_ld = dreg_rq_synced[2];

	/*
	if (addr_ld)
	{
	    cout << "Setting address bus to " << hex << int(dbus_latch) << endl;
	}

	if (data_ld)
	{
	    cout << "Setting data bus to " << hex << int(dbus_latch) << endl;
	}
	*/

	// Low registers logic

	loreg_data_en = (loreg_addr_valid && data_ld);

	if (phi1_fall)
	{
	    loreg_addr_valid = ((addr_ld && (dbus_latch < 0x20)) || (loreg_addr_valid && !addr_ld));
	    loreg_addr = dbus_latch;
	}

	if (phi1_rise)
	{
	    if (!mrst_n)
	    {
		noise_enable = false;
		noise_freq = 0;

		timer_a_run = false;
		timer_b_run = false;

		timer_a_irq_en = false;
		timer_b_irq_en = false;

		csm_reg = false;
		key_on_temp = 0;

		lfo_freq = 0;
	    }
	    else if (loreg_data_en)
	    {
		switch (loreg_addr)
		{
		    case 0x08:
		    {
			key_on_temp = (dbus_latch & 0x7F);
		    }
		    break;
		    case 0x0F:
		    {
			noise_enable = testbit(dbus_latch, 7);
			noise_freq = (dbus_latch & 0x1F);
		    }
		    break;
		    case 0x14:
		    {
			timer_a_run = testbit(dbus_latch, 0);
			timer_b_run = testbit(dbus_latch, 1);
			timer_a_irq_en = testbit(dbus_latch, 2);
			timer_b_irq_en = testbit(dbus_latch, 3);
			csm_reg = testbit(dbus_latch, 7);
		    }
		    break;
		    case 0x18:
		    {
			lfo_freq = dbus_latch;
		    }
		    break;
		    case 0x19:
		    {
			if (testbit(dbus_latch, 7))
			{
			    pms_data = (dbus_latch & 0x7F);
			}
			else
			{
			    ams_data = (dbus_latch & 0x7F);
			}
		    }
		    break;
		    default:
		    {
			cout << "Writing value of " << hex << int(dbus_latch) << " to low register of " << hex << int(loreg_addr) << endl;
		    }
		    break;
		}
	    }
	}

	bool dbus_temp_en = !(current_pins.pin_csn || current_pins.pin_wrn);

	if (dbus_temp_en)
	{
	    dbus_temp = current_pins.data;
	}

	if (phi1_fall)
	{
	    if (!mrst_n)
	    {
		dbus_latch = 0;
	    }
	    else if (areg_rq_synced[1] || dreg_rq_synced[1])
	    {
		dbus_latch = dbus_temp;
	    }
	}

	bool areg_rq_latch_set = !((current_pins.pin_csn || current_pins.pin_wrn || current_pins.pin_a0 || !mrst_n) || areg_rq_synced[1]);
	bool areg_rq_latch_rst = (areg_rq_synced[1] || !mrst_n);

	areg_rq_latch = (areg_rq_latch_set && !areg_rq_latch_rst);

	bool dreg_rq_latch_set = !((current_pins.pin_csn || current_pins.pin_wrn || !current_pins.pin_a0 || !mrst_n) || dreg_rq_synced[1]);
	bool dreg_rq_latch_rst = (dreg_rq_synced[1] || !mrst_n);

	dreg_rq_latch = (dreg_rq_latch_set && !dreg_rq_latch_rst);

	if (phi1_fall)
	{
	    if (!mrst_n)
	    {
		areg_rq_synced[0] = false;
		areg_rq_synced[2] = false;

		dreg_rq_synced[0] = false;
		dreg_rq_synced[2] = false;
	    }
	    else
	    {
		areg_rq_synced[0] = areg_rq_latch;
		areg_rq_synced[2] = areg_rq_synced[1];

		dreg_rq_synced[0] = dreg_rq_latch;
		dreg_rq_synced[2] = dreg_rq_synced[1];
	    }
	}

	if (phi1_rise)
	{
	    if (!mrst_n)
	    {
		areg_rq_synced[1] = false;
		dreg_rq_synced[1] = false;
	    }
	    else
	    {
		areg_rq_synced[1] = areg_rq_synced[0];
		dreg_rq_synced[1] = dreg_rq_synced[0];
	    }
	}
    }
};