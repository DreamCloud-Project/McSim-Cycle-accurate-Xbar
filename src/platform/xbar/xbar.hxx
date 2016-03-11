/*
 * xbar.hxx
 *
 *  Created on: Oct 5, 2015
 *      Author: charles
 */

#ifndef XBAR_HXX_
#define XBAR_HXX_

#include "packet.hxx"
#include <systemc.h>
#include "../dcSimuParams.hxx"
#include "lib/xbar_sc_fifo_ports.h"
#include <iostream>

namespace dreamcloud {
namespace platform_sclib {
namespace xbar {

using dreamcloud::platform_sclib::noc_ppa::Packet;

SC_MODULE(XBAR) {
	sc_vector<xb_sc_fifo_in<Packet>> xb_inputs;
	sc_vector<xb_sc_fifo_out<Packet>> xb_outputs;

	sc_in<bool> clk;
	dcSimuParams params;
	unsigned last_;
	int TotalPacketExchanged;
	std::string policy;

	SC_HAS_PROCESS(XBAR);
	XBAR(sc_module_name name, dcSimuParams params_) :
			sc_module(name), xb_inputs("xb_inputs",
					(params_.getRows() * params_.getCols())), xb_outputs(
					"xb_outputs", (params_.getRows() * params_.getCols())), params(
					params_), last_(0), TotalPacketExchanged(0), policy(
					params_.getXbarPolicy()) {
		SC_CTHREAD(process, clk);

	}
	void process();
	int ComputeActualDestination(std::pair<unsigned int, unsigned int> dest,
			dcSimuParams params);
	int GetTotalPacketExchanged();
};
}
}
}

#endif /* XBAR_HXX_ */
