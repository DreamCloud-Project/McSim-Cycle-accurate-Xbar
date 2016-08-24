# McSim-Cycle-accurate-Xbar: Manycore platform Simulation tool for crossbar-based platform at a Cycle-accurate level

This repository contains a simulator able to simulate embedded applications described using
the [AMALTHEA](http://www.amalthea-project.org/) application model on top of crossbar-based manycore architectures in a cycle accurate way. This simulator has been developed by the LIRMM laboratory of Montpellier.


To get the simulator you must clone this repository and its submodules. To clone the repository if you have a GitHub account with an SSH key registered use `git clone git@github.com:DreamCloud-Project/McSim-Cycle-accurate-Xbar.git`. Else use `git clone https://github.com/DreamCloud-Project/McSim-Cycle-accurate-Xbar.git`. Then do `cd McSim-Cycle-accurate-Xbar` followed by `git submodule init` followed by `git submodule update` to clone submodules.

## Using the simulator

To ease the usage of the simulator, two python scripts are provided:  

- `compile.py` for compilation  
- `simulate.py` for launching the simulation  

The requirements for using these scripts are the following ones:  

- have a Linux system
- have a C++ compiler
- have CMake installed on your system ([https://cmake.org](https://cmake.org/))
- have SystemC 2.3.1
    * download SystemC (Core SystemC Language and Examples) from here http://accellera.org/downloads/standards/systemc/systemc-license-agreement
    * decompress the archive
    * run `configure` and then `make install` 
    * you should see a new `include` folder and a new `lib-linux64` folder in your root folder of SystemC 
    * define the `SYSTEMC_HOME` environment variable to the root folder of SystemC 
- have the xerces-c-dev library installed in standard includes and libs folders (using `apt-get install libxerces-c-dev` for example on Ubuntu)
  or have xerces-c-dev library in a custom folder and define `XERCES_HOME` environment variable

### Compiling the simulator

Compilation is done through the `compile.py` script which documentation is the following:  

```
>> ./compile.py --help
usage: compile.py [-h] [-v] {build,clean} ...

Crossbar simulator compiler script

optional arguments:
  -h, --help     show this help message and exit
  -v, --verbose  enable verbose output

valid subcommands:
  {build,clean}
```

### Running the simulator

To run a particular simulation, just run the `simulate.py` script. By
default this script runs one iteration of the Demo Car application on
a 4x4 crossbar-based many core using ZigZag mapping, First Come First Serve (fcfs)
scheduling and without repeating periodic runnables.  You can play
with all these parameters which documentation is the following:

```
>>./simulate.py --help
usage: simulate.py [-h] [-d] [-da {DC}] [-ca CUSTOM_APPLICATION] [-e SIMUEND]
                   [-f FREQ] [-mf MODES_FILE] [-i ITERATIONS]
                   [-m MAPPING_STRATEGY [MAPPING_STRATEGY ...]] [-np]
                   [-o OUTPUT_FOLDER] [-r] [-s {fcfs,prio}] [-v] [-x ROWS]
                   [-xbp {Full,RoundRobin,Priority}] [-xbfs XBARFIFOSIZE]
                   [-xblrl XBARLOCALREADLATENCY]
                   [-xblwl XBARLOCALWRITELATENCY]
                   [-xbrrl XBARREMOTEREADLATENCY]
                   [-xbrwl XBARREMOTEWRITELATENCY] [-y COLS]

Crossbar simulator runner script

optional arguments:
  -h, --help            show this help message and exit
  -d, --syntax_dependency
                        consider successive runnables in tasks call graph as
                        dependent
  -da {DC}, --def_application {DC}
                        specify the application to be simulated among the
                        default ones
  -ca CUSTOM_APPLICATION, --custom_application CUSTOM_APPLICATION
                        specify a custom application file to be simulated
  -e SIMUEND, --simuEnd SIMUEND
                        specify the end time of simulation in nanosecond
  -f FREQ, --freq FREQ  specify the frequency of all the cores in the platform
                        (i.g 400MHz or 1GHz)
  -mf MODES_FILE, --modes_file MODES_FILE
                        specify a modes switching file to be simulated
  -i ITERATIONS, --iterations ITERATIONS
                        specify the number of application to execute (has no
                        effect with -p)
  -m MAPPING_STRATEGY [MAPPING_STRATEGY ...], --mapping_strategy MAPPING_STRATEGY [MAPPING_STRATEGY ...]
                        specify the mapping strategy used to map runnables on
                        cores. Valide strategies are ['MinComm', 'Static',
                        'StaticTriCore', 'ZigZag', '3Core']
  -np, --no_periodicity
                        run periodic runnables only once
  -o OUTPUT_FOLDER, --output_folder OUTPUT_FOLDER
                        specify the absolute path of the output folder where
                        simulation results will be generated
  -r, --random          replace constant seed used to generate instructions
                        timing distributions by a random one based on the time
  -s {fcfs,prio}, --scheduling_strategy {fcfs,prio}
                        specify the scheduling strategy used by cores to
                        choose the runnable to execute
  -v, --verbose         enable verbose output
  -x ROWS, --rows ROWS  specify the number of rows in the platform
  -xbp {Full,RoundRobin,Priority}, --xbarPolicy {Full,RoundRobin,Priority}
                        specify the cross bar arbitration plociy
  -xbfs XBARFIFOSIZE, --xbarFifoSize XBARFIFOSIZE
                        specify the cross bar fifos size
  -xblrl XBARLOCALREADLATENCY, --xbarLocalReadLatency XBARLOCALREADLATENCY
                        specify the latency of local read
  -xblwl XBARLOCALWRITELATENCY, --xbarLocalWriteLatency XBARLOCALWRITELATENCY
                        specify the latency of local write
  -xbrrl XBARREMOTEREADLATENCY, --xbarRemoteReadLatency XBARREMOTEREADLATENCY
                        specify the latency of remote read
  -xbrwl XBARREMOTEWRITELATENCY, --xbarRemoteWriteLatency XBARREMOTEWRITELATENCY
                        specify the latency of remote write
  -y COLS, --cols COLS  specify the number of columns in the platform
```

## Licence

This software is made available under the  GNU Lesser General Public License v3.0

Report bugs at: mcsim-support@lirmm.fr  

(C)2016 CNRS and Université de Montpellier
