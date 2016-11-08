# How to run this code:
#   mpiexec -n 5 python Examples/Example_120_UndulatorSpectrum_MultiParticle_MPI.py


# In this example MPI is used to calculate the spectrum in a multi-particle
# simulation.  The rank 0 process calculates the ideal single-particle
# spectrum and then waits for the other processes to return their data.
# When a process returns the results are added to the others.  The results
# are then plotted together and saved as a png.  If you want the plot to
# show you can remove the show=False


# Import the OSCARS and helper modules
import OSCARS
from OSCARSPlotsMPL import *

# MPI imports
from mpi4py import MPI
from mpi4py.MPI import ANY_SOURCE
import numpy

# Common MPI communication, rank, size
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# Get an OSCARS object
osc = OSCARS.OSCARS()

# Let's just make sure that each process only uses 1 threads since
# we assume mpi is handeling this
osc.set_nthreads_global(1)

# Set a particle beam with non-zero emittance
osc.set_particle_beam(type='electron',
                      name='beam_0',
                      energy_GeV=3,
                      x0=[0, 0, -1],
                      d0=[0, 0, 1],
                      current=0.500,
                      sigma_energy_GeV=0.001*3,
                      beta=[1.5, 0.8],
                      emittance=[0.9e-9, 0.008e-9],
                      horizontal_direction=[1, 0, 0],
                      lattice_reference=[0, 0, 0])

# Must set the start and stop time for calculations
osc.set_ctstartstop(0, 2)

# Add an undulator field
osc.add_bfield_undulator(bfield=[0, 1, 0], period=[0, 0, 0.049], nperiods=31)

# Number of particles per node of rank > 1
particles_per_node = 20

# Observation point for spectrum
observation_point = [0, 0, 30]

# Number of points in the spectrum
npoints = 500

# Energy range for spectrum
range_eV = [100, 500]




# Now the MPI fun
if rank == 0:
    # For rank 0 we calculate the ideal single-particle spectrum
    osc.set_new_particle(particle='ideal')
    spectrum_se = osc.calculate_spectrum(obs=observation_point, energy_range_eV=range_eV, npoints=npoints)

    # Weight for each spectrum in summation
    weight = 1. / size

    # Get a new OSCARS object to collect the data in
    osc_sum = OSCARS.OSCARS()

    # Now wait and collect data from all other processes when it comes in
    for i in range(1, size):
        # Get incoming data
        data = comm.recv(source=ANY_SOURCE)

        # Sum the spectra (internally this is a compensated sum)
        osc_sum.add_to_spectrum(spectrum=data, weight=weight)

    # Plot the single-particle and multi-particle data and save to file
    plot_spectra([spectrum_se, osc_sum.get_spectrum()], ['single-electron', 'multi-electron'], ofile='UndulatorSpectrum_MPI.png', show=False)



else:
    # If not rank 0, calculate the desired spectrum
    data = osc.calculate_spectrum(obs=observation_point, energy_range_eV=range_eV, npoints=npoints, nparticles=particles_per_node)

    # Send results back to rank 0
    comm.send(data, dest=0)





