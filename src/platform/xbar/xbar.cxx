/*
 * xbar.cxx
 *
 *  Created on: Oct 5, 2015
 *      Author: charles
 */

#include "xbar.hxx"

namespace dreamcloud {
namespace platform_sclib {
namespace xbar {

using namespace std;

void XBAR::process() {
	while (true) {
		if (policy == "Full") { // full
			Packet pck;
			for (size_t i = 0; i < params.getDimension(); i++) {
				if (xb_inputs.at(i).nb_read(pck)) {
					pair<int, int> destAddr = pck.get_destination();
					int destActual = ComputeActualDestination(destAddr, params);
					if (xb_outputs.at(destActual).num_free() > 0) {
						pck.set_delivery_time();
						xb_outputs.at(destActual).write(pck);
						TotalPacketExchanged++;
					}
				}
			}
		} else if (policy == "RoundRobin") { //round robin
			Packet pck;
			for (size_t i = 0; i < params.getDimension(); i++) {
				int current = (last_ + i)
						% (params.getRows() * params.getCols());
				if (xb_inputs[current].nb_read(pck)) {
					pair<int, int> destAddr = pck.get_destination();
					int destActual = ComputeActualDestination(destAddr, params);
					if (xb_outputs.at(destActual).num_free() > 0) {
						pck.set_delivery_time();
						xb_outputs.at(destActual).write(pck);
						last_ = current;
						TotalPacketExchanged++;
						break;
					}
				}
			}
		} else if (policy == "Priority") {
			Packet pck;
			int index = 0;
			unsigned int val = 0;
			for (size_t i = 0; i < params.getDimension(); i++) {
				if (xb_inputs.at(i).nb_read_(pck)) { //non_blocking read without removing packet
					if (i == 0) {
						val = pck.get_priority();
						index = i;
					} else {
						if (pck.get_priority() > val) {
							index = i;
							val = pck.get_priority();
						}
					}
				}
			}
			//Read packet from input with packet with highest priority and forward to output
			Packet rPck;
			if (xb_inputs.at(index).nb_read(rPck)) { //non_blocking read with removing packet
				pair<int, int> destAddr = rPck.get_destination();
				int destActual = ComputeActualDestination(destAddr, params);
				if (xb_outputs.at(destActual).num_free() > 0) {
					rPck.set_delivery_time();
					xb_outputs.at(destActual).write(rPck);
					TotalPacketExchanged++;
				}
			}
		}
		wait();
	}
}

int XBAR::ComputeActualDestination(pair<unsigned int, unsigned int> dest,
		dcSimuParams params) {
	int Actual;
	bool DestinationComputed = false;
	int counter = 0;

	for (unsigned int i = 0; i < params.getRows(); i++) {
		if (DestinationComputed == false) {
			for (unsigned int j = 0; j < params.getCols(); j++) {
				if (i == dest.first && j == dest.second) {
					Actual = counter;
					DestinationComputed = true;
					break;
				}
				counter++;
			}
		}
	}
	return Actual;
}

int XBAR::GetTotalPacketExchanged() {
	return TotalPacketExchanged;
}

}
}
}
