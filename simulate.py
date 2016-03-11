#! /usr/bin/env python2

import argparse
import os
import sys
import subprocess
import collections
import re

DEFAULT_ITERATIONS = 1
DEFAULT_APP = 'DC'
DEFAULT_MAPPING_STRATEGY = 'ZigZag'
DEFAULT_OUTPUT_FOLDER = '/OUTPUT_FILES'
DEFAULT_SCHEDULING_STRATEGY = 'fcfs'
DEFAULT_ROWS = 4
DEFAULT_COLS = 4
DEFAULT_FREQ = 1000000000
DEFAULT_XBAR_POLICY = 'Full'
DEFAULT_XBAR_FIFO_SIZE = 16
DEFAULT_XBAR_LOCAL_READ_LAT = 2
DEFAULT_XBAR_LOCAL_WRITE_LAT = 2
DEFAULT_XBAR_REMOTE_READ_LAT = 2
DEFAULT_XBAR_REMOTE_WRITE_LAT = 2


FREQ_UNITS = {
 'GHz' : 1000000000,
 'MHz' : 1000000,
 'KHz' : 1000,
 'Hz'  : 1
}

APP_NAMES_TO_FILES = {
 'DC'  : '/apps/DEMO_CAR/DemoCar-PowerUp.amxmi',
}

MAPPINGS = ['MinComm', 'Static', 'StaticTriCore', 'ZigZag', '3Core']

class ValidateMapping(argparse.Action):
    def __call__(self, parser, args, values, option_string=None):
        mapping = values[0]
        if not mapping in MAPPINGS:
             raise ValueError('invalid mapping {s!r}'.format(s=mapping))
        mappingFile = None
        if mapping.startswith('Static'):
            if len(values) == 1:
                 raise ValueError('{s!r} mapping requires a file option'.format(s=mapping))
            mappingFile = values[1]
        randomfixedSeed = None
        if mapping == 'Randomfixed':
            if len(values) == 1:
                 raise ValueError('{s!r} mapping requires a seed option'.format(s=mapping))
            randomfixedSeed = values[1]
        setattr(args, self.dest, [mapping, mappingFile, randomfixedSeed])

class ValidateFreq(argparse.Action):
    def invalidFreq(self, freq):
         raise ValueError('{s!r} is an invalid frequency. Correct format is a number followed by a unit among {units}'.format(s=freq, units=FREQ_UNITS.keys()))
    def __call__(self, parser, args, values, option_string=True):
        freq=values
        freqRegEx = re.compile('(\d+)(.*)')
        m = freqRegEx.search(freq)
        if m:
            try:
                value = int(m.groups()[0])
            except:
                self.invalidFreq(freq)
            try:
                unit = m.groups()[1]
            except:
                self.invalidFreq(freq)
            freqInHz = value * FREQ_UNITS[unit]
            setattr(args, self.dest, freqInHz)
        else:
            self.invalidFreq(freq)

def main():
    ''' Run the abstract simulator '''
    # Configure parameters parser
    parser = argparse.ArgumentParser(description='Abstract Simulator Runner script')
    parser.add_argument('-d', '--syntax_dependency', action='store_true', help='consider successive runnables in tasks call graph as dependent')
    appGroup = parser.add_mutually_exclusive_group()
    appGroup.add_argument('-da', '--def_application', help='specify the application to be simulated among the default ones', choices=['DC'])
    appGroup.add_argument('-ca', '--custom_application', help='specify a custom application file to be simulated')
    parser.add_argument('-e', '--simuEnd', help='specify the end time of simulation in nanosecond', type=int)
    parser.add_argument('-f', '--freq', help='specify the frequency of all the cores in the platform (i.g 400MHz or 1GHz)', action=ValidateFreq)
    appGroup.add_argument('-mf', '--modes_file', help='specify a modes switching file to be simulated')
    parser.add_argument('-i', '--iterations', type=int, help='specify the number of application to execute (has no effect with -p)')
    parser.add_argument('-m', '--mapping_strategy', help='specify the mapping strategy used to map runnables on cores. Valide strategies are ' + str(MAPPINGS), nargs="+",  action=ValidateMapping)
    parser.add_argument('-np', '--no_periodicity', action='store_true', help='run periodic runnables only once')
    parser.add_argument('-o', '--output_folder', help='specify the absolute path of the output folder where simulation results will be generated')
    parser.add_argument('-r', '--random', action='store_true', help='replace constant seed used to generate instructions timing distributions by a random one based on the time')
    parser.add_argument('-s', '--scheduling_strategy', help='specify the scheduling strategy used by cores to choose the runnable to execute', choices=['fcfs', 'prio'])
    parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output')
    parser.add_argument('-x', '--rows', type=int, help='specify the number of rows in the platform')
    parser.add_argument('-xbp', '--xbarPolicy', help='specify the cross bar arbitration plociy', choices=['Full', 'RoundRobin', 'Priority'])
    parser.add_argument('-xbfs', '--xbarFifoSize', type=int, help='specify the cross bar fifos size')
    parser.add_argument('-xblrl', '--xbarLocalReadLatency', type=int, help='specify the latency of local read')
    parser.add_argument('-xblwl', '--xbarLocalWriteLatency', type=int, help='specify the latency of local write')
    parser.add_argument('-xbrrl', '--xbarRemoteReadLatency', type=int, help='specify the latency of remote read')
    parser.add_argument('-xbrwl', '--xbarRemoteWriteLatency', type=int, help='specify the latency of remote write')
    parser.add_argument('-y', '--cols', type=int, help='specify the number of columns in the platform')

    # Get parameters
    args = parser.parse_args()
    if args.def_application:
        app = os.path.dirname(os.path.realpath(__file__)) + APP_NAMES_TO_FILES[args.def_application]
    elif args.custom_application:
        app = args.custom_application
    else:
        app = os.path.dirname(os.path.realpath(__file__)) + APP_NAMES_TO_FILES['DC']
    mapping = DEFAULT_MAPPING_STRATEGY
    mappingFile = None
    mappingSeed = None
    if args.mapping_strategy:
        mapping = args.mapping_strategy[0]
        mappingFile = args.mapping_strategy[1]
        mappingSeed = args.mapping_strategy[2]
    if mapping == 'StaticModes' and not args.modes_file:
        raise ValueError('StaticModes mapping must be used with mf option')
    out = os.path.dirname(os.path.realpath(__file__)) + DEFAULT_OUTPUT_FOLDER
    if args.output_folder:
        out = args.output_folder
    if not os.path.exists(out):
        os.makedirs(out)
    sched = DEFAULT_SCHEDULING_STRATEGY
    if args.scheduling_strategy:
        sched = args.scheduling_strategy
    freq = DEFAULT_FREQ
    if args.freq:
        freq = args.freq
    rows = DEFAULT_ROWS
    if args.rows:
        rows = args.rows
    cols = DEFAULT_COLS
    if args.cols:
        cols = args.cols
    its = DEFAULT_ITERATIONS
    if args.iterations:
        its = args.iterations
    xbarPolicy = DEFAULT_XBAR_POLICY
    if args.xbarPolicy:
        xbarPolicy = args.xbarPolicy
    xbarFifoSize = DEFAULT_XBAR_FIFO_SIZE
    if args.xbarFifoSize:
        xbarFifoSize = args.xbarFifoSize
    xbarLocalReadLatency = DEFAULT_XBAR_LOCAL_READ_LAT
    if args.xbarLocalReadLatency:
        xbarLocalReadLatency = args.xbarLocalReadLatency
    xbarLocalWriteLatency = DEFAULT_XBAR_LOCAL_WRITE_LAT
    if args.xbarLocalWriteLatency:
        xbarLocalWriteLatency = args.xbarLocalWriteLatency
    xbarRemoteReadLatency = DEFAULT_XBAR_REMOTE_READ_LAT
    if args.xbarRemoteReadLatency:
        xbarRemoteReadLatency = args.xbarRemoteReadLatency
    xbarRemoteWriteLatency = DEFAULT_XBAR_REMOTE_WRITE_LAT
    if args.xbarRemoteWriteLatency:
        xbarRemoteWriteLatency = args.xbarRemoteWriteLatency

    # Add systemc lib to LD_LIBRARY_PATH
    my_env = os.environ.copy()
    sc_home = my_env.get('SYSTEMC_HOME', '')
    if not sc_home:
        raise ValueError('You must define the SYSTEMC_HOME variable to use this script')
    for file in os.listdir(sc_home):
        if 'lib-' in file:
            my_env['LD_LIBRARY_PATH'] = my_env.get('LD_LIBRARY_PATH', '') + ':' + sc_home + '/' + file
            break
    my_env['SC_COPYRIGHT_MESSAGE'] = 'DISABLE'
    xerces_home = my_env.get('XERCES_HOME', '')
    if xerces_home:
         my_env['LD_LIBRARY_PATH'] = my_env.get('LD_LIBRARY_PATH', '') + ':' + xerces_home + '/lib'

    # Run the simulation
    cmd = [os.path.dirname(os.path.realpath(__file__)) + '/obj/mcsim-ca-xbar', '-i', str(its), '-m', mapping]
    if mappingFile:
        cmd.append(mappingFile)
    if mappingSeed:
        cmd.append(mappingSeed)
    cmd.extend(('-freq', str(freq), '-o', out, '-s', sched, '-x', str(rows), '-y', str(cols),
                '-policy', xbarPolicy,
                '-bsize', str(xbarFifoSize),
                '-lrc', str(xbarLocalReadLatency),
                '-lwc',  str(xbarLocalWriteLatency),
                '-rrc', str(xbarRemoteReadLatency),
                '-rwc', str(xbarRemoteWriteLatency)))
    if args.modes_file:
        cmd.extend(('-f', args.modes_file))
    else:
        cmd.extend(('-a', app))
    if args.syntax_dependency:
         cmd.append('-d')
    if args.no_periodicity:
         cmd.append('-np')
    if args.random:
         cmd.append('-r')
    if args.simuEnd:
        cmd.append('-simuEnd')
        cmd.append(str(args.simuEnd))
    cmdStr = ''
    for c in cmd:
        cmdStr = cmdStr + c + ' '
    if args.verbose:
        print cmdStr
    sim = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=my_env)
    stdout, stderr = sim.communicate()
    print stdout,
    outFile = open(out + '/' + 'OUTPUT_Execution_Report.log', 'w')
    outFile.write(stdout)
    outFile.close()
    if sim.returncode != 0:
        print stderr,
        print ('simulation FAILED')
        sys.exit(-1)

    # Run the energy estimator module
    cmd = [os.path.dirname(os.path.realpath(__file__)) + '/obj/energy_estimator', out, os.path.dirname(os.path.realpath(__file__)) + '/src/energy_estimator/']
    if args.verbose:
        print cmd
    nrj = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = nrj.communicate()
    if nrj.wait() != 0:
        print stderr,
        print ('Energy estimation FAILED')
        sys.exit(-1)
    outFile = open(out + '/' + 'OUTPUT_Energy.log', 'w')
    outFile.write(stdout)
    outFile.close()


# This script runs the main
if __name__ == "__main__":
    main()
