////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                           DREAMCLOUD PROJECT                               //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

////////////////////
//    INCLUDES    //
////////////////////
#include <fstream>
#include <utility>
#include "dcSystem.hxx"

namespace dreamcloud {
namespace platform_sclib {

int dcSystem::bufferSize = 0;

/**
 * SystemC method used to get completed runnables from PEs and to enable
 * runnables dependent on the completed ones.
 */
void dcSystem::dependentRunnablesReleaser_method() {

	// Copy all completed dcRunnableInstance ids from PEs into dcSystem and erase them in PE_XY
	vector<dcRunnableInstance *> completedRunInstances;
	for (unsigned int x = 0; x < params.getRows(); x++) {
		for (unsigned int y = 0; y < params.getCols(); y++) {
			processingElement *pe = pes[x][y];
			for (std::vector<dcRunnableInstance *>::const_iterator i =
					pe->completedRunInstances.begin();
					i != pe->completedRunInstances.end(); ++i) {
				completedRunInstances.push_back(*i);
				nbRunnablesCompleted++;
			}
		}
	}

	// When we don't handle periodics stop the simulation when all
	// the runnables (periodic and non periodic) have been executed once
	// or start a new iteration if needed

	//cout << nbRunnablesCompleted << " completed and " << nbRunnablesMapped << " mapped at time " << sc_time_stamp() << endl;

	// Stop simulation after a given number of iterations
	if (simulationEndFromCmdLine == 0 && simulationEndFromMode == 0
			&& (params.dontHandlePeriodic() || hyperPeriod == 0)) {
		if (nbRunnablesCompleted / runnables.size() >= params.getIterations()) {
			stopSimu_event.notify();
			return;
		}
		if ((nbRunnablesCompleted % runnables.size()) == 0) {
			iteration_event.notify(0, SC_NS);
		}
	}

	// Loop over all the completed runnables
	for (vector<dcRunnableInstance *>::size_type i = 0;
			i < completedRunInstances.size(); ++i) {
		// Loop over all runnableCalls that depend on the completed one
		dcRunnableInstance *runInst = completedRunInstances.at(i);
		dcRunnableCall* runCall = runInst->getRunCall();
		vector<dcRunnableCall*> enabledToExecute = runCall->GetListOfEnables();
		for (std::vector<dcRunnableCall*>::iterator it =
				enabledToExecute.begin(); it != enabledToExecute.end(); ++it) {
			// If all the previous runnables of the current dependent are completed, release it (the dependent one)
			if ((*it)->GetEnabledBy() == (*it)->GetListOfEnablers().size()) {
				dcRunnableInstance * runInst = new dcRunnableInstance(*it);
				releaseRunnable(runInst);
			}
			// Else only indicate that one dependency has been satisfied
			else {
				(*it)->SetEnabledBy((*it)->GetEnabledBy() + 1);
			}
		}
	}

	// Delete all completed runnable instances
	for (unsigned int x = 0; x < params.getRows(); x++) {
		for (unsigned int y = 0; y < params.getCols(); y++) {
			processingElement *pe = pes[x][y];
			for (std::vector<dcRunnableInstance *>::const_iterator i =
					pe->completedRunInstances.begin();
					i != pe->completedRunInstances.end(); ++i) {
				delete (*i);
			}
			pe->completedRunInstances.clear();
		}
	}
}

void dcSystem::dumpTaskAndRunnableGraphFile() {
	ofstream dcGraphDotFile(params.getOutputFolder() + "/dcRunGraphFile.gv");
	dcGraphDotFile << "digraph Runnable {" << endl;
	dcGraphDotFile << "\tcompound=true;" << endl;
	unsigned int i = 0;
	for (std::vector<dcTask *>::iterator it = tasks.begin(); it != tasks.end();
			++it) {
		dcGraphDotFile << "\tsubgraph cluster" << i << " {" << endl;
		dcRunnableCall *run = (*it)->GetRunnableCalls();
		while (run != NULL) {
			dcGraphDotFile << "\t\t" << run->GetRunClassName();
			dcGraphDotFile << "[label=\"";
			dcGraphDotFile << run->GetRunClassName();
			dcGraphDotFile << "\\nWave id=" << hex << run->GetWaveID();
			dcGraphDotFile << "\"];" << endl;
			dcRunnableEdge *edge = run->GetEdges();
			while (edge != NULL) {
				if (edge->GetType() == 1) {
					dcGraphDotFile << "\t\t" << run->GetRunClassName() << " -> "
							<< edge->GetConnectTo()->GetRunClassName() << ";"
							<< endl;
				}
				edge = edge->GetNext();
			}
			run = run->GetNext();
		}
		i++;
		dcGraphDotFile << "\t}" << endl;
	}
	dcGraphDotFile << "}";
	dcGraphDotFile.close();
}

void dcSystem::dumpTaskGraphFile() {

	FILE* dcGraphDotFile = fopen(
			(params.getOutputFolder() + "/dcTasksGraphFile.gv").c_str(), "w+");
	fputs("digraph application {\n", dcGraphDotFile);
	for (std::vector<dcTask *>::iterator it = tasks.begin(); it != tasks.end();
			++it) {
		fputs((*it)->GetName().c_str(), dcGraphDotFile);
		dcActEvent* event = (*it)->GetActEvent();
		if (event != NULL) {
			if (event->GetType() == "stimuli:Periodic") {
				dcPeriodicEvent* periodic = static_cast<dcPeriodicEvent*>(event);
				pair<int, string> recurrence = periodic->GetRecurrence();
				fputs("[color=red,fontcolor=red,label=\"", dcGraphDotFile);
				fputs((*it)->GetName().c_str(), dcGraphDotFile);
				fprintf(dcGraphDotFile, "\\nPeriod=%d%s", recurrence.first,
						recurrence.second.c_str());
				fputs("\"]", dcGraphDotFile);
			} else if (event->GetType() == "stimuli:Sporadic") {
				dcSporadicEvent* sporadic = static_cast<dcSporadicEvent*>(event);
				double lowerBound = sporadic->GetLowerBound();
				double upperBound = sporadic->GetUpperBound();
				fputs("[color=blue,fontcolor=blue,label=\"", dcGraphDotFile);
				fputs((*it)->GetName().c_str(), dcGraphDotFile);
				fprintf(dcGraphDotFile, "\\nSporadic=%0.2f,%0.2f", lowerBound,
						upperBound);
				fputs("\"]", dcGraphDotFile);
			}
		}
		fputs(";\n", dcGraphDotFile);
	}
	for (std::vector<dcTask *>::iterator it = tasks.begin(); it != tasks.end();
			++it) {
		dcTaskEdge * edge = (*it)->GetEdges();
		while (edge != NULL) {
			if (edge->GetType()) {
				fputs((*it)->GetName().c_str(), dcGraphDotFile);
				fputs(" -> ", dcGraphDotFile);
				fputs(edge->GetConnectTo()->GetName().c_str(), dcGraphDotFile);
				fputs(";\n", dcGraphDotFile);
			}
			edge = edge->GetNext();
		}
	}
	fputs("}", dcGraphDotFile);
	fclose(dcGraphDotFile);
}

/**
 * This SC_METHOD is called each time a mode switch occur
 * using dynamic sensitivity
 */
void dcSystem::modeSwitcher_thread() {
	vector<mode_t>::size_type modIdx = 0;
	while (true) {
		unsigned long int nowInNano = sc_time_stamp().value() * 1E-3;
		if (modes.at(modIdx).name.compare("end")) {
			mappingHeuristic->switchMode(nowInNano, modes.at(modIdx).file,
					modes.at(modIdx).name);
			labelsMapper_method();
			updateInstructionsExecTimeBounds(modes.at(modIdx).file);
		}
		assert(
				modes.at(modIdx).time == nowInNano
						&& "Invalid time in modeSwitcher_thread");
		if (modIdx + 1 < modes.size()) {
			unsigned long int waitTime = modes.at(modIdx + 1).time - nowInNano;
			modIdx++;
			wait(waitTime, SC_NS);
		} else {
			stopSimu_event.notify();
			break;
		}
	}
}

/**
 * SC_METHOD that release all independent runnables.
 * This method is called first at time zero and then on each
 * iteration_event notification.
 */
void dcSystem::nonPeriodicIndependentRunnablesReleaser_method() {
	vector<dcRunnableCall *> runs;
	if (params.dontHandlePeriodic()) {
		runs = application->GetIndependentRunnables(taskGraph);
	} else {
		runs = application->GetIndependentNonPeriodicRunnables(taskGraph);
	}
	if (params.dontHandlePeriodic() && runs.size() == 0) {
		cerr << params.getAppXml()
				<< " doesn't contain any independent runnable to start, please check it !"
				<< endl;
		exit(-1);
	}
	for (std::vector<dcRunnableCall *>::iterator it = runs.begin();
			it != runs.end(); ++it) {
		dcRunnableInstance * runInst = new dcRunnableInstance(*it);
		releaseRunnable(runInst);
	}
}

/**
 * SystemC thread used to release periodic runnables.
 * This thread is triggered at initialization time and then dynamically
 * computes its next occurrence.
 */
void dcSystem::periodicRunnablesReleaser_thread() {

	// Only used in periodic handling case
	if (params.dontHandlePeriodic()) {
		return;
	}

	vector<int>::size_type nbPeriodicRunnables =
			periodicAndSporadicRunnables.size();
	vector<unsigned long int> remainingTimeBeforeRelease(nbPeriodicRunnables);
	for (vector<int>::size_type i = 0; i < nbPeriodicRunnables; ++i) {
		dcRunnableCall *runnable = periodicAndSporadicRunnables.at(i);
		remainingTimeBeforeRelease[i] = runnable->GetOffsetInNano();
	}

	while (true) {

		// Stop the simu if hyper period reached and we are not using neither mode switch
		// nor simu duration from command line
		unsigned long int currentTimeInNano = (sc_time_stamp().value() / 1E3);
		if (simulationEndFromCmdLine == 0 && simulationEndFromMode == 0
				&& !params.dontHandlePeriodic() && hyperPeriod > 0
				&& currentTimeInNano >= hyperPeriod) {
			stopSimu_event.notify();
			return;
		}

		// Stop the simu if specified simu time is done
		if (simulationEndFromCmdLine != 0
				&& (sc_time_stamp().value() / 1E3)
						>= simulationEndFromCmdLine) {
			stopSimu_event.notify();
			return;
		}

		unsigned long int waitTime = ULONG_MAX;
		for (vector<int>::size_type i = 0; i < nbPeriodicRunnables; ++i) {
			if (remainingTimeBeforeRelease[i] == 0) {
				dcRunnableCall *runnable = periodicAndSporadicRunnables.at(i);
				dcRunnableInstance * runInst = new dcRunnableInstance(runnable);
				releaseRunnable(runInst);
				remainingTimeBeforeRelease[i] = runnable->GetPeriodInNano();
			}
			if (remainingTimeBeforeRelease[i] < waitTime) {
				waitTime = remainingTimeBeforeRelease[i];
			}
		}
		for (vector<int>::size_type i = 0; i < nbPeriodicRunnables; ++i) {
			remainingTimeBeforeRelease[i] = remainingTimeBeforeRelease[i]
					- waitTime;
		}
		if (simulationEndFromCmdLine != 0) {
			unsigned long int toEnd = simulationEndFromCmdLine
					- (sc_time_stamp().value() / 1E3);
			if (toEnd < waitTime) {
				waitTime = toEnd;
			}
		}
		wait(waitTime, SC_NS);
	}
}

/**
 * Return a human readable string of the given frequency.
 */
string dcSystem::getFrequencyString(unsigned long int freqInHertz) {
	if (freqInHertz < 1E3) {
		return std::to_string(freqInHertz) + " Hz";
	} else if (freqInHertz >= 1E3 && freqInHertz < 1E6) {
		return std::to_string((unsigned long int) (freqInHertz / 1E3)) + " KHz";
	} else if (freqInHertz >= 1E6 && freqInHertz < 1E9) {
		return std::to_string((unsigned long int) (freqInHertz / 1E6)) + " MHz";
	} else if (freqInHertz >= 1E9) {
		return std::to_string((unsigned long int) (freqInHertz / 1E9)) + " GHz";
	} else {
		cerr << "Invalid Frequency in hertz" << freqInHertz << endl;
		exit(-1);
	}
}

/**
 * SystemC method called once only during initialization.
 */
void dcSystem::labelsMapper_method() {
	for (vector<dcLabel*>::size_type i = 0; i < labels.size(); i++) {
		dcMappingHeuristicI::dcMappingLocation loc = mappingHeuristic->mapLabel(
				labels.at(i)->GetID(), sc_time_stamp().value() * 1E-3,
				labels.at(i)->GetName());
		// All the PEs have a local copy of the labels mapping table
		for (unsigned int row(0); row < params.getRows(); ++row) {
			for (unsigned int col(0); col < params.getCols(); ++col) {
				pes[row][col]->labelsMappingTable.push_back(
						std::make_pair(loc, labels.at(i)->GetName()));
			}
		}
	}
}

void dcSystem::runnablesMapper_thread() {

	int appIterations = 0;
	int remainingNbRunnablesToMap = runnables.size();
	for (unsigned int row(0); row < params.getRows(); ++row) {
		for (unsigned int col(0); col < params.getCols(); ++col) {
			pes[row][col]->newRunnableSignal = false;
		}
	}

	// Map runnables until application is "finished"
	while (true) {

		// Wait until a runnable can be executed or we reached hyper period
		if (readyRunnables.empty()) {
			wait();
			continue;
		}

		// Ask the mapping heuristic where to map the current ready runnable
		dcRunnableCall * runCall = readyRunnables.front()->getRunCall();
		dcMappingHeuristicI::dcMappingLocation pe =
				mappingHeuristic->mapRunnable(sc_time_stamp().value(),
						runCall->GetRunClassId(), runCall->GetRunClassName(),
						runCall->GetTask()->GetName(),
						runCall->GetTask()->GetID(), runCall->GetIdInTask(),
						readyRunnables.front()->GetPeriodId());
		int x = pe.first;
		int y = pe.second;

		// If the PE has completed receiving the previous runnable,
		// map the new one on it
		// ONE clock cycle
		if (!pes[x][y]->newRunnableSignal) {
			runnablesMappingCsvFile
					<< readyRunnables.front()->getRunCall()->GetRunClassId()
					<< ","
					<< readyRunnables.front()->getRunCall()->GetRunClassName()
					<< "," << x << y << endl;
			nbRunnablesMapped++;
			readyRunnables.front()->SetMappingTime(sc_time_stamp().value());
			pes[x][y]->newRunnable = readyRunnables.front();
			pes[x][y]->newRunnableSignal = true;
			newRunnable_event[x][y].notify();
			readyRunnables.erase(readyRunnables.begin());
			remainingNbRunnablesToMap--;
			if (remainingNbRunnablesToMap == 0) {
				appIterations++;
				remainingNbRunnablesToMap = runnables.size();
			}
			wait(1, SC_NS);
		}

		// Else move the current runnable to the end of the independent queue
		// ONE clock cycle
		else {
			readyRunnables.push_back(readyRunnables.front());
			readyRunnables.erase(readyRunnables.begin());
			wait(1, SC_NS);
		}
	}
}

void dcSystem::parseModeFile(string file) {
	std::ifstream infile(file);
	std::string line;
	while (std::getline(infile, line)) {
		istringstream is(line);
		string timeString;
		string modeName;
		string modeFile;
		getline(is, timeString, ';');
		getline(is, modeName, ';');
		getline(is, modeFile, ';');
		string::size_type sz;
		unsigned long int timeInNano = stod(timeString, &sz) * 1E9;
		mode_t mode { timeInNano, modeName, modeFile };
		modes.push_back(mode);
	}
}

void dcSystem::releaseRunnable(dcRunnableInstance *runnable) {
	runnable->SetReleaseTime(sc_time_stamp().value());
	readyRunnables.push_back(runnable);
	runnableReleased_event.notify();
}

void dcSystem::stopSimu_thread() {

	// In the case of periodic runnables
	// handling,  wait for all aperiodic ones to be
	// completed
	if (simulationEndFromCmdLine == 0 && !params.dontHandlePeriodic()) {
		while (nbRunnablesCompleted < nbRunnablesMapped) {
			wait(runnableCompleted_event);
		}
	}

	// Get end time
	clock_t end = std::clock();

	// Prints label access results
	ofstream f(params.getOutputFolder() + "/labels.csv");
	f << "PE,";
	f << "Nb bytes for local reads,";
	f << "Nb of local reads,";
	f << "Nb bytes for local writes,";
	f << "Nb of local writes,";
	f << "Nb bytes for remote reads,";
	f << "Nb of remote reads,";
	f << "Nb bytes for remote writes,";
	f << "Nb of remote write,";
	f << "Total computation time" << endl;
	unsigned int nbLocRds = 0;
	unsigned int nbLocWrs = 0;
	unsigned int nbRemRds = 0;
	unsigned int nbRemWrs = 0;
	unsigned long int bytesLocRds = 0;
	unsigned long int bytesLocWrs = 0;
	unsigned long int bytesRemRds = 0;
	unsigned long int bytesRemWrs = 0;
	unsigned long int computationTime = 0;
	for (unsigned int row(0); row < params.getRows(); ++row) {
		for (unsigned int col(0); col < params.getCols(); ++col) {
			f << "PE" << row << col << ",";
			f << pes[row][col]->bytesLocRds << ",";
			f << pes[row][col]->nbLocRds << ",";
			f << pes[row][col]->bytesLocWrs << ",";
			f << pes[row][col]->nbLocWrs << ",";
			f << pes[row][col]->bytesRemRds << ",";
			f << pes[row][col]->nbRemRds << ",";
			f << pes[row][col]->bytesRemWrs << ",";
			f << pes[row][col]->nbRemWrs << ",";
			f << pes[row][col]->computationTime << endl;
			bytesLocRds += pes[row][col]->bytesLocRds;
			bytesLocWrs += pes[row][col]->bytesLocWrs;
			bytesRemRds += pes[row][col]->bytesRemRds;
			bytesRemWrs += pes[row][col]->bytesRemWrs;
			nbLocRds += pes[row][col]->nbLocRds;
			nbLocWrs += pes[row][col]->nbLocWrs;
			nbRemRds += pes[row][col]->nbRemRds;
			nbRemWrs += pes[row][col]->nbRemWrs;
			computationTime += pes[row][col]->computationTime;
		}
	}
	f << "Total Nb bytes for local reads = " << bytesLocRds << endl;
	f << "Total Nb of local reads = " << nbLocRds << endl;
	f << "Total Nb bytes for local writes = " << bytesLocWrs << endl;
	f << "Total Nb of local writes = " << nbLocWrs << endl;
	f << "Total Nb bytes for remote reads = " << bytesRemRds << endl;
	f << "Total Nb of remote reads = " << nbRemRds << endl;
	f << "Total Nb bytes for remote writes = " << bytesRemWrs << endl;
	f << "Total Nb of remote writes = " << nbRemWrs << endl;
	f.close();

	// Print some results in a file for energy estimation
	FILE *Parameters = fopen(
			(params.getOutputFolder() + "/Parameters.txt").c_str(), "w+");
	unsigned long int endTimeInNano = sc_time_stamp().value() / 1E3;
	string timeString = static_cast<ostringstream*>(&(ostringstream()
			<< endTimeInNano))->str();
	float systemFreq = 1.0 / params.getCoresPeriodInNano();
	std::ostringstream freqVal;
	freqVal << systemFreq;
	string freqString = freqVal.str();
	string parameter_row = static_cast<ostringstream*>(&(ostringstream()
			<< (params.getRows())))->str();
	string parameter_col = static_cast<ostringstream*>(&(ostringstream()
			<< (params.getCols())))->str();
	string s_temp = "Execution time of the application(ns): " + timeString
			+ "\nClock frequency(GHz) : " + freqString + "\nROWS : "
			+ parameter_row + "\nCOLUMNS : " + parameter_col;
	fputs(s_temp.c_str(), Parameters);

	// Print some results on stdout
	cout << " ##Simulation results##" << endl << endl;
	cout << "    Number of runnable instances executed         : "
			<< nbRunnablesCompleted << endl;
	if (params.dontHandlePeriodic()) {
		cout << "    Number of application iterations executed     : "
				<< (nbRunnablesCompleted / runnables.size()) << endl;
	}
	cout << "    Execution time of the application             : "
			<< endTimeInNano << " ns" << endl;
	int nbDeadlineMisses = 0;
	for (unsigned int row(0); row < params.getRows(); ++row) {
		for (unsigned int col(0); col < params.getCols(); ++col) {
			nbDeadlineMisses = nbDeadlineMisses
					+ pes[row][col]->deadlinesMissed;
		}
	}
	cout << "    Number of runnables which missed deadlines    : "
			<< nbDeadlineMisses << endl;
	cout << "    Number of packets exchanged through XBAR      : "
			<< (xbar->GetTotalPacketExchanged()) << endl;
	cout << "    Simulation time                               : "
			<< (double) (end - start) / CLOCKS_PER_SEC << " s" << endl;
	string policy = "";
	if (params.getXbarPolicy() == "Full") {
		policy = "Full Duplex";
	} else if (params.getXbarPolicy() == "RoundRobin") {
		policy = "Round Robin";
	} else {
		policy = "Priority";
	}
	cout << "    XBAR arbitration policy                       : " << policy
			<< endl;

	sc_stop();
}

void dcSystem::updateInstructionsExecTimeBounds(string newAppFile) {

	// Parse new application
	dcAmaltheaParser amaltheaParser;
	AmApplication* amApplication = new AmApplication();
	amaltheaParser.ParseAmaltheaFile(newAppFile, amApplication);
	dcApplication *app = new dcApplication();
	dcTaskGraph * taskGraph = app->createGraph("dcTaskGraph");
	app->CreateGraphEntities(taskGraph, amApplication, params.getSeqDep());

	// Compare and update instructions
	vector<dcInstruction *> instructions = application->GetAllInstructions(
			taskGraph);
	vector<dcInstruction *> newInstructions = app->GetAllInstructions(
			taskGraph);
	for (vector<dcInstruction *>::size_type i = 0; i < instructions.size();
			i++) {
		dcInstruction *newInst = newInstructions.at(i);
		dcInstruction *oldInst = instructions.at(i);
		if (newInst->GetName() != oldInst->GetName()) {
			cerr << "error while updating instructions execution time bounds"
					<< endl;
			exit(-1);
		}
		string instrName = newInst->GetName();
		if (instrName == "sw:InstructionsDeviation") {
			dcExecutionCyclesDeviationInstruction *newInstDev =
					static_cast<dcExecutionCyclesDeviationInstruction*>(newInst);
			dcExecutionCyclesDeviationInstruction *oldInstDev =
					static_cast<dcExecutionCyclesDeviationInstruction*>(oldInst);
			oldInstDev->SetLowerBound(newInstDev->GetLowerBound());
			oldInstDev->SetUpperBound(newInstDev->GetUpperBound());
			oldInstDev->SetKappa(newInstDev->GetKappa());
			oldInstDev->SetLambda(newInstDev->GetLambda());
			oldInstDev->SetMean(newInstDev->GetMean());
			oldInstDev->SetRemainPromille(newInstDev->GetRemainPromille());
			oldInstDev->SetSD(newInstDev->GetSD());
		}
		if (instrName == "sw:InstructionsConstant") {
			dcExecutionCyclesConstantInstruction *newInstCst =
					static_cast<dcExecutionCyclesConstantInstruction*>(newInst);
			dcExecutionCyclesConstantInstruction *oldInstCst =
					static_cast<dcExecutionCyclesConstantInstruction*>(oldInst);
			oldInstCst->SetValue(newInstCst->GetValue());
		}
	}

	// Delete objects
	delete taskGraph;
	delete app;
	delete amApplication;
}

}
}

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//  END OF FILE.                                                              //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
