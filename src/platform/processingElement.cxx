////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                           DREAMCLOUD PROJECT                               //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

////////////////////
//    INCLUDES    //
////////////////////
#include "processingElement.hxx"
#include <math.h> 
#include <bitset>


////////////////////
//      USING     //
////////////////////
using namespace std;

namespace dreamcloud {
namespace platform_sclib {

int processingElement::nextPktId = 0;
int processingElement::nextReadRequestId = 0;

/**
 * Add the given runnable execution element to th ready list.
 * This function preserve ready runnables list order according
 * to the priority of the runnable execution element.
 */
void processingElement::addReadyRunnable(runnableExecElement runnableExecElem) {
	readyRunnables.insert(readyRunnables.begin(), runnableExecElem);
	std::sort(readyRunnables.begin(), readyRunnables.end(),
			[](const runnableExecElement &left, const runnableExecElement &right)
			{
				return left.first < right.first;
			});
}

/**
 * Execute the given constant number of instructions by
 * waiting. Also log information for energy model.
 */
void processingElement::executeInstructionsConstant(dcInstruction *inst,
		dcRunnableInstance *run, int instructionId) {
	dcExecutionCyclesConstantInstruction* einst =
			static_cast<dcExecutionCyclesConstantInstruction*>(inst);
	double waitTimeInNano = 1E9 * einst->GetValue()
			* type.getNbCyclesPerInstructions() / type.getFrequencyInHz();
	wait(waitTimeInNano, SC_NS);
	string exeInstS = run->getRunCall()->GetRunClassName() + " ,"
			+ std::to_string(instructionId) + " ,"
			+ std::to_string(einst->GetValue()) + "\n";
	computationTime += waitTimeInNano;
	const char* exeInstS_const = exeInstS.c_str();
	fputs(exeInstS_const, instsCsvFile);
}

/**
 * Execute the given deviation number of instructions by
 * waiting. Also log information for energy model.
 */
void processingElement::executeInstructionsDeviation(dcInstruction *inst,
		dcRunnableInstance *run, int instructionId) {
	dcExecutionCyclesDeviationInstruction* einst =
			static_cast<dcExecutionCyclesDeviationInstruction*>(inst);
	double UpperBound = 0.0;
	double LowerBound = 0.0;
	if (einst->GetUpperBoundValid()) {
		UpperBound = einst->GetUpperBound();
	}
	if (einst->GetLowerBoundValid()) {
		LowerBound = einst->GetLowerBound();
	}
	std::uniform_int_distribution<> distr(LowerBound, UpperBound);
	int compute_duration = distr(gen);
	double waitTimeInNano = 1E9 * compute_duration
			* type.getNbCyclesPerInstructions() / type.getFrequencyInHz();
	wait(waitTimeInNano, SC_NS);
	computationTime += waitTimeInNano;
	string exeInstS = run->getRunCall()->GetRunClassName() + " ,"
			+ std::to_string(instructionId) + " ,"
			+ std::to_string(compute_duration) + "\n";
	const char* exeInstS_const = exeInstS.c_str();
	fputs(exeInstS_const, instsCsvFile);
}

void processingElement::executeRemoteLabelRead(dcRemoteAccessInstruction *rinst,
		dcRunnableInstance *run, int x, int y, int instructionId) {

	// Create packet representing read request
	Packet pck;
	pck.set_id(nextPktId++);
	pck.set_priority(run->getRunCall()->GetPriority());
	pck.set_read_request_id(nextReadRequestId++);
	pck.set_source(make_pair(x_PE, y_PE));
	pck.set_destination(make_pair(x, y));
	pck.set_rd_wr(false);
	pck.set_requestedSize(
			(int) (ceil(
					double(rinst->GetLabel()->GetSize())
							/ double(8 * PACKET_SIZE_IN_BYTES))));
	pck.set_req_resp(false);
	pck.set_write_request_ID(0);
	pck.set_write_rq_size(0);
	pck.set_write_rq_ID(0);
	nbRemRds++;
	bytesRemRds += rinst->GetLabel()->GetSize();

	// Put the runnable in the blocked list
	// and remove it from the ready list
	runnableBlockedOnRemoteRead blockedOnRemoteRead;
	responsePackets waitingParameters;
	runnableExecStatus preemptedRStructure = std::make_pair(instructionId + 1,
			run);
	runnableExecElement preempted = std::make_pair(pck.get_read_request_id(),
			preemptedRStructure);
	waitingParameters.first = pck.get_destination();
	waitingParameters.second = 0;
	blockedOnRemoteRead.first = waitingParameters;
	blockedOnRemoteRead.second.first = pck.get_requestedSize();
	blockedOnRemoteRead.second.second = preempted;
	runnablesBlockedOnRemoteRead.push_back(blockedOnRemoteRead);
	removeReadyRunnable(run);

	//Write packet to XBAR. Block if buffer is full
	pck.set_injection_time();
	pe_outputs.write(pck);

	// Sending a packet has a cost of 32 cycles for now:
	// 1 clock cycle for each flit of the packet
	wait(PACKET_SIZE_IN_BYTES, SC_NS);

	// Log runnable block in VCD file
	if (params.getGenerateWaveforms()) {
		unsigned long int nowInNano = sc_time_stamp().value() * 1E-3;
		*runnablesVcdFile << "#" << nowInNano << endl;
		*runnablesVcdFile << VCD_12_UNDEF << " " << VCD_ACTIVE_RUN_ID << x_PE
				<< y_PE << endl;
		*runnablesVcdFile << "1" << VCD_SUSPENDED_ON_REQUEST << x_PE << y_PE
				<< endl;
	}
}

void processingElement::executeRemoteLabelWrite(
		dcRemoteAccessInstruction *rinst, dcRunnableInstance *run, int writeRequestId,
		int x, int y) {

	int number_of_Packets = (int) (ceil(
			double(rinst->GetLabel()->GetSize()) / double(8 * PACKET_SIZE_IN_BYTES)));
	nbRemWrs++;
	bytesRemWrs += rinst->GetLabel()->GetSize();
	for (int pkts = 0; pkts < number_of_Packets; pkts++) {
		Packet pck;
		pck.set_id(nextPktId++);
		pck.set_priority(run->getRunCall()->GetPriority());
		pck.set_source(make_pair(x_PE, y_PE));
		pck.set_destination(make_pair(x, y));
		pck.set_rd_wr(true);
		pck.set_req_resp(false);
		pck.set_requestedSize(0);
		pck.set_write_rq_ID(pkts);
		if (pkts == 0) {
			pck.set_write_rq_size(number_of_Packets);
		} else {
			pck.set_write_rq_size(0);
		}
		pck.set_write_request_ID(writeRequestId);
		pck.set_injection_time();
		pck.set_read_request_id(-1);
		pe_outputs.write(pck);
		//localPktsOut.push_back(temp_Packet);
		wait(params.getRemoteWriteCost(), SC_NS);
	}

	// Notify packet_creater()
	sendPacket_event.notify();
}

/**
 * SC_METHOD sensitive to the packet_in port which
 * is connected to an sc_buffer. It handles the
 * received packets according to 3 situations:
 *
 *   1 Remote write from other PE
 *   2 Remote read from other PE
 *   3 Answer to a remote read originated from this PE
 *
 *   In the situation number 2, some packets need to be sent back
 *   to the requester.
 *
 */
void processingElement::pktReceiver_method() {
	while (true) {
		while (pe_inputs.num_available() > 0) {

			Packet p = pe_inputs.read();

			// Dump the received packet
			(*nocTrafficCsvFile) << p;
			(*nocTrafficCsvFile).flush();

			// We receive a request for a write to this PE from a remote PE
			// TODO: ensure that we can remove this case
			if (p.isWrite()) {

				if (p.get_write_rq_ID() == 0 && p.get_write_rq_size() > 0) {
					writeRequests_in_process temp;
					temp.first = p.get_write_request_ID();
					temp.second.first = std::make_pair(p.get_source().first,
							p.get_source().second);
					temp.second.second.first = 1; //  1 because current packet itself is the first packet
					temp.second.second.second = p.get_write_rq_size();
					writeRequests.push_back(temp);
					if (p.get_write_rq_size() == 1) {
						goto wr_only_one;
					}
				} else {
					wr_only_one: vector<writeRequests_in_process>::iterator writeRequests_Iterator;
					writeRequests_Iterator =
							std::find_if(writeRequests.begin(),
									writeRequests.end(),
									[&p](const writeRequests_in_process& pair)
									{
										return ((pair.first == p.get_write_request_ID()) &&
												(pair.second.first.first == p.get_source().first) &&
												(pair.second.first.second == p.get_source().second));
									});
					if (writeRequests_Iterator != writeRequests.end()) {
						if (p.get_write_rq_size() == 0
								&& (p.get_write_rq_ID() > 0)
								&& (p.get_write_request_ID()
										== writeRequests_Iterator->first))
							(writeRequests_Iterator->second.second.first)++;
					}
				}
			}

			// We receive a read from a remote PE
			// we must send back the response
			else if (!p.isWrite() && !p.isReadResponse()) {
				int number_of_Packets = p.get_requestedSize();
				for (int pkts = 0; pkts < number_of_Packets; pkts++) {
					Packet pck;
					pck.set_id(nextPktId++);
					pck.set_priority(p.get_priority());
					pck.set_read_request_id(p.get_read_request_id());
					pck.set_source(p.get_destination());
					pck.set_destination(p.get_source());
					pck.set_rd_wr(false);
					pck.set_requestedSize(0);
					pck.set_req_resp(true);

					wait(params.getRemoteReadCost(), SC_NS);
					pck.set_injection_time();
					pe_outputs.write(pck);
					//localPktsOut.push_back(pktElem);
				}
				// Notify packet_creater()
				//sendPacket_event.notify();
			}

// We receive a response from a previous remote read originated from this PE
// We must move the runnable concerned by this read from the "blocked on remote read"
// runnables list to the "ready" runnables one.
			else if (!p.isWrite() && p.isReadResponse()) {
				std::pair<int, int> source = p.get_source();
				int readRequestId = p.get_read_request_id();
				vector<runnableBlockedOnRemoteRead>::iterator it;
				it =
						std::find_if(runnablesBlockedOnRemoteRead.begin(),
								runnablesBlockedOnRemoteRead.end(),
								[&source, &readRequestId](const runnableBlockedOnRemoteRead& pair)
								{
									return ((pair.first.first == source) && (pair.second.second.first == readRequestId));
								});

				// If this PE doesn't have a runnable blocked on the remote read
				if (it == runnablesBlockedOnRemoteRead.end()) {
					cerr << " PE" << x_PE << y_PE
							<< " received a read response to NOBODY :-( with request ID = "
							<< readRequestId << endl;
					exit(-1);
				}

				// We receive a new packet for the remote read
				// If all the packets have been received, then we can
				// unblock the runnable.
				(it->first.second)++;
				if (it->first.second == it->second.first) {
					addReadyRunnable(it->second.second);
					runnablesBlockedOnRemoteRead.erase(it);
					newRunnable_event->notify();
				}
			}
		}
		wait();
	}
}

/**
 * SystemC thread that execute an instruction of the current
 * runnable and then check for preemption.
 */
void processingElement::runnableExecuter_thread() {

	int writeRequestId = 0;

	while (true) {

		// Wait for runnables from dcSystem
		if (readyRunnables.empty() && !newRunnableSignal) {
			wait(*newRunnable_event);
		}

		// If a new runnable has been sent during the execution of the last instruction
		// Put it in the ready list
		// ONE clock cycle
		if (newRunnableSignal) {
			runnableExecStatus execStatus = std::make_pair(0, newRunnable);
			int prio;
			if (sched == PRIO) {
				prio = newRunnable->getRunCall()->GetPriority();
			} else if (sched == FCFS) {
				prio = newRunnable->GetMappingTime();
			}
			newRunnable->SetCoreReceiveTime(sc_time_stamp().value());
			runnableExecElement execElement = std::make_pair(prio, execStatus);
			addReadyRunnable(execElement);

			// Log runnable preemption in VCD file
			unsigned long int nowInNano = sc_time_stamp().value() * 1E-3;
			if (params.getGenerateWaveforms()) {
				if (readyRunnables.size() > 1
						&& readyRunnables.front().second.second->GetUniqueID()
								== newRunnable->GetUniqueID()) {
					*runnablesVcdFile << "#" << nowInNano << endl;
					*runnablesVcdFile << "1" << VCD_PREEMPTION << x_PE << y_PE
							<< endl;
				}
			}

			newRunnableSignal = false;
			wait(1, SC_NS);
		}

		// Choose the runnable with the highest priority among ready ones
		// Operation in ZERO clock cycle
		runnableExecElement execElem = readyRunnables.front();
		runnableExecStatus execStatus = execElem.second;
		dcRunnableInstance *currentRunnable = execStatus.second;
		unsigned int instructionId = execStatus.first;
		vector<dcInstruction *> instructions =
				currentRunnable->getRunCall()->GetAllInstructions();
		unsigned int nbInstructions = instructions.size();
		bool blockedOnRemoteRead = false;

		// Move to next instruction for next time this runnable will be executed
		readyRunnables.front().second.first++;

		// If the next instruction to execute exists.
		// This checks is required because when we block a runnable
		// on a remote read and its the last instruction, when we unblock it
		// we are above the last instruction
		if (instructionId < nbInstructions) {
			dcInstruction* inst = instructions.at(instructionId);
			string instrName = inst->GetName();
			unsigned long int nowInPico = sc_time_stamp().value();
			if (instructionId == 0) {
				currentRunnable->SetStartTime(nowInPico);
			}

			// Log runnable activation in VCD file
			if (params.getGenerateWaveforms()) {
				std::bitset<12> binId(
						currentRunnable->getRunCall()->GetWaveID());
				*runnablesVcdFile << "#" << (nowInPico * 1E-3) << " b" << binId
						<< " " << VCD_ACTIVE_RUN_ID << x_PE << y_PE << endl;
			}

			// Label accesses
			if (instrName == "sw:LabelAccess") {

				// Search where is the label
				dcRemoteAccessInstruction *rinst =
						static_cast<dcRemoteAccessInstruction*>(inst);
				string labelName = rinst->GetLabel()->GetName();
				vector<std::pair<std::pair<int, int>, string> >::iterator it;
				it =
						std::find_if(labelsMappingTable.begin(),
								labelsMappingTable.end(),
								[&labelName](const std::pair< std::pair<int, int>, std::string >& pair)
								{
									return pair.second == labelName;
								});
				unsigned int destX = it->first.first;
				unsigned int destY = it->first.second;

				// Local read and write accesses
				// ONE clock cycle per byte
				if ((x_PE == destX) && (y_PE == destY)) {
					int localAccessSize = (int) (ceil(
							double(rinst->GetLabel()->GetSize()) / double(8)));
					if (rinst->GetWrite()) {
						wait(localAccessSize * params.getLocalWriteCost(), SC_NS);
						nbLocWrs++;
						bytesLocWrs += rinst->GetLabel()->GetSize();
					} else {
						wait(localAccessSize * params.getLocalReadCost(), SC_NS);
						nbLocRds++;
						bytesLocRds += rinst->GetLabel()->GetSize();
					}
				}

				// Remote write: sent packets according to the size of the label
				// nbPackets * PACKET_SIZE clock cycles
				else if (rinst->GetWrite()) {
					executeRemoteLabelWrite(rinst, currentRunnable,
							writeRequestId, destX, destY);
					writeRequestId++;
				}

				// Remote read: send one packet including information allowing
				// receiver to send response back
				// PACKET_SIZE clock cycles
				else if (!rinst->GetWrite()) {
					executeRemoteLabelRead(rinst, currentRunnable, destX, destY,
							instructionId);
					blockedOnRemoteRead = true;
				}
			} // End label access

			// Instruction constant
			else if (instrName == "sw:InstructionsConstant") {
				executeInstructionsConstant(inst, currentRunnable,
						instructionId);
			} else if (instrName == "sw:InstructionsDeviation") {
				executeInstructionsDeviation(inst, currentRunnable,
						instructionId);
			}
		} // End if instructionID < nbInstructions

		// Check if runnable is completed
		if (!blockedOnRemoteRead
				&& (instructionId >= nbInstructions - 1 || nbInstructions == 0)) {

			// Log runnable info in CSV file and arff
			currentRunnable->SetCompletionTime(sc_time_stamp().value());
			unsigned long int runnableExecutionTime =
					currentRunnable->GetCompletionTime()
							- currentRunnable->GetCoreReceiveTime();
			(*runnablesCsvFile) << "PE" << x_PE << y_PE << ",";
			(*runnablesCsvFile)
					<< currentRunnable->getRunCall()->GetRunClassName() << ",";
			(*runnablesCsvFile) << currentRunnable->getRunCall()->GetPriority()
					<< ",";
			(*runnablesCsvFile) << (currentRunnable->GetMappingTime() / 1E3)
					<< ",";
			(*runnablesCsvFile) << (currentRunnable->GetStartTime() / 1E3)
					<< ",";
			(*runnablesCsvFile) << (currentRunnable->GetCompletionTime() / 1E3)
					<< ",";
			(*runnablesCsvFile) << (runnableExecutionTime / 1E3) << ",";
			(*runnablesCsvFile)
					<< (currentRunnable->getRunCall()->GetDeadlineValueInNano())
					<< ",";
			(*runnablesCsvFile)
					<< (currentRunnable->getRunCall()->GetDeadlineValueInNano()
							- (runnableExecutionTime / 1E3)) << endl;
			(*runnablesArffFile) << "PE" << x_PE << y_PE << ",";
			(*runnablesArffFile) << fixed
					<< currentRunnable->getRunCall()->GetRunClassName() << ",";
			(*runnablesArffFile) << fixed
					<< currentRunnable->getRunCall()->GetPriority() << ",";
			(*runnablesArffFile) << fixed
					<< (currentRunnable->GetMappingTime() / 1E3) << ",";
			(*runnablesArffFile) << fixed
					<< (currentRunnable->GetStartTime() / 1E3) << ",";
			(*runnablesArffFile) << fixed
					<< (currentRunnable->GetCompletionTime() / 1E3) << ",";
			(*runnablesArffFile) << fixed << (runnableExecutionTime / 1E3)
					<< ",";
			(*runnablesArffFile) << fixed
					<< (currentRunnable->getRunCall()->GetDeadlineValueInNano()
							/ 1E3) << ",";
			(*runnablesArffFile) << fixed
					<< (currentRunnable->getRunCall()->GetDeadlineValueInNano()
							- (runnableExecutionTime / 1E3)) << endl;

			// Log runnable complete in VCD file
			unsigned long int nowInNano = sc_time_stamp().value() * 1E-3;
			if (params.getGenerateWaveforms()) {
				*runnablesVcdFile << "#" << nowInNano << endl;
				*runnablesVcdFile << VCD_12_UNDEF << " " << VCD_ACTIVE_RUN_ID
						<< x_PE << y_PE << endl;
				*runnablesVcdFile << "1" << VCD_RUNNABLE_COMPLETED << x_PE
						<< y_PE << endl;
			}

			// Check deadline miss
			if (runnableExecutionTime * 1E-3
					> currentRunnable->getRunCall()->GetDeadlineValueInNano()) {
				deadlinesMissed++;
				if (params.getGenerateWaveforms()) {
					*runnablesVcdFile << "1" << VCD_DEADLINE_MISSED << x_PE
							<< y_PE << endl;
				}
			}

			// Add the completed runnable to completed queue and remove it from ready one
			completedRunInstances.push_back(currentRunnable);
			(*runnableCompleted_event).notify(SC_ZERO_TIME);
			removeReadyRunnable(currentRunnable);
		}
	}
}

void processingElement::removeReadyRunnable(dcRunnableInstance *runnable) {
	vector<runnableExecElement>::iterator toRemove = std::find_if(
			readyRunnables.begin(), readyRunnables.end(),
			[&runnable](runnableExecElement& elem)
			{
				return elem.second.second == runnable;
			});
	if (toRemove == readyRunnables.end()) {
		cerr << "ERORRRR " << runnable->getRunCall()->GetRunClassName()
				<< " not found" << endl;
		exit(-1);
	}
	readyRunnables.erase(toRemove);
}

}
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  END OF FILE.                                                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
