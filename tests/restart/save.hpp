/*
A class for saving game of life tests of dccrg to vtk files.

Copyright 2010, 2011 Finnish Meteorological Institute

Dccrg is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License version 3
as published by the Free Software Foundation.

Dccrg is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with dccrg.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SAVE_HPP
#define SAVE_HPP

#include "algorithm"
#include "boost/foreach.hpp"
#include "cstdio"
#include "iostream"
#include "stdint.h"
#include "vector"

#include "../../dccrg.hpp"

#include "cell.hpp"

using namespace std;

/*!
Saves the current state of given game of life grid into the given dc file.

Header format:
uint64_t time step
double   x_start
double   y_start
double   z_start
double   cell_x_size
double   cell_y_size
double   cell_z_size
uint64_t x_length in cells
uint64_t y_length in cells
uint64_t z_length in cells
int      maximum_refinement_level
*/
template<class UserGeometry> class Save
{
public:

	static void save(
		const string& name,
		const uint64_t step,
		const boost::mpi::communicator& comm,
		dccrg::Dccrg<Cell, UserGeometry>& game_grid
	) {
		std::remove(name.c_str());
		
		comm.barrier();

		// MPI_File_open wants a non-constant string
		char* name_c_string = new char [name.size() + 1];
		strncpy(name_c_string, name.c_str(), name.size() + 1);

		MPI_File outfile;

		int result = MPI_File_open(
			comm,
			name_c_string,
			MPI_MODE_CREATE | MPI_MODE_WRONLY,
			MPI_INFO_NULL,
			&outfile
		);

		delete [] name_c_string;

		if (result != MPI_SUCCESS) {
			char mpi_error_string[MPI_MAX_ERROR_STRING + 1];
			int string_length;
			MPI_Error_string(result, mpi_error_string, &string_length);
			mpi_error_string[string_length + 1] = '\0';
			std::cerr << "Couldn't open file " << name_c_string
				<< ": " << mpi_error_string
				<< std::endl;
			return false;
		}

		// process 0 writes the header
		const size_t header_size = sizeof(int) + 4 * sizeof(uint64_t) + 6 * sizeof(double);
		if (comm.rank() == 0) {
			uint8_t* buffer = new uint8_t [header_size];
			assert(buffer != NULL);

			size_t offset = 0;
			memcpy(buffer + offset, &step, sizeof(uint64_t));
			offset += sizeof(uint64_t);

			{
			double value = game_grid->get_x_start();
			memcpy(buffer + offset, &value, sizeof(double));
			offset += sizeof(double);
			value = game_grid->get_y_start();
			memcpy(buffer + offset, &value, sizeof(double));
			offset += sizeof(double);
			value = game_grid->get_z_start();
			memcpy(buffer + offset, &value, sizeof(double));
			offset += sizeof(double);
			value = game_grid->get_cell_x_size(1);
			memcpy(buffer + offset, &value, sizeof(double));
			offset += sizeof(double);
			value = game_grid->get_cell_y_size(1);
			memcpy(buffer + offset, &value, sizeof(double));
			offset += sizeof(double);
			value = game_grid->get_cell_z_size(1);
			memcpy(buffer + offset, &value, sizeof(double));
			offset += sizeof(double);
			}
			{
			uint64_t value = game_grid->get_x_length();
			memcpy(buffer + offset, &value, sizeof(uint64_t));
			offset += sizeof(uint64_t);
			value = game_grid->get_y_length();
			memcpy(buffer + offset, &value, sizeof(uint64_t));
			offset += sizeof(uint64_t);
			value = game_grid->get_z_length();
			memcpy(buffer + offset, &value, sizeof(uint64_t));
			offset += sizeof(uint64_t);
			}
			{
			int value = game_grid->get_maximum_refinement_level();
			memcpy(buffer + offset, &value, sizeof(int));
			offset += sizeof(int);
			}

			result = MPI_File_write_at_all(
				outfile,
				0,
				buffer,
				header_size,
				MPI_BYTE,
				MPI_STATUS_IGNORE
			);

			delete [] buffer;

			if (result != MPI_SUCCESS) {
				char mpi_error_string[MPI_MAX_ERROR_STRING + 1];
				int string_length;
				MPI_Error_string(result, mpi_error_string, &string_length);
				mpi_error_string[string_length + 1] = '\0';
				std::cerr << "Process " << comm.rank()
					<< " Couldn't write cell list to file " << name
					<< ": " << mpi_error_string
					<< std::endl;
				return false;
			}

		} else {
			result = MPI_File_write_at_all(
				outfile,
				0,
				(void*) &step,
				0,
				MPI_BYTE,
				MPI_STATUS_IGNORE
			);
		}

		game_grid.write_grid(name, header_size);
	}
};

#endif

