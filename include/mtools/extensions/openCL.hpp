/** @file openCL.hpp */
//
// Copyright 2015 Arvind Singh
//
// This file is part of the mtools library.
//
// mtools is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with mtools  If not, see <http://www.gnu.org/licenses/>.
#pragma once

// check if OpenCL must be enabled
#include "../mtools_config.hpp"


#if (MTOOLS_USE_OPENCL)

#include "../misc/internal/mtools_export.hpp"

// we want to use C++ exceptions
#define __CL_ENABLE_EXCEPTIONS

#ifndef __CL_ENABLE_EXCEPTIONS
namespace cl
{
class Error
	{
	public:
		const char * what() const { return "exceptions not activated for openCL.."; }
	};
}
#endif


// openCL c++ header
#if defined (_MSC_VER)
 
	#pragma warning( push )				
	#pragma warning( disable : 4996 )	
	#if __has_include(<CL/cl.hpp>)
		#include <CL/cl.hpp>
	#else
		#error "Cannot find OpenCL headers! (Windows)"
	#endif
	#pragma warning( pop )

#elif defined  __APPLE__

	#if __has_include(<OpenCL/cl2.hpp>)
		#include <OpenCL/cl2.hpp>
	#elif __has_include(<CL/cl2.hpp>)
		#include <CL/cl2.hpp>
        #elif __has_include(<OpenCL/cl.hpp>)
                #include <OpenCL/cl.hpp>
        #elif __has_include(<CL/cl.hpp>)
                #include <CL/cl.hpp>
	#else 
		#error "Cannot find OpenCL headers! (OSX)"
	#endif

#else 

	#if __has_include(<CL/cl2.hpp>)
		#include <CL/cl2.hpp>
	#elif __has_include(<CL/cl.hpp>)
		#include <CL/cl.hpp>
	#else
		#error "Cannot find OpenCL headers! (Linux)"
	#endif

	
#endif


namespace mtools
	{


	/** Structure containing the base classes for using openCL */
	struct OpenCLBundle
		{

		// the objects that describe the interanl status of the openCL bundle
		cl::Platform		platform;
		cl::Device			device;	
		cl::Context			context;
		cl::CommandQueue	queue;


		/**
		 * Constructor.
		 *
		 * @param	selectdefault 	true to select default choice for the platform and device.
		 * @param	output		  	true to output to mtools::cout.
		 * @param	showextensions	true to show the supported extensions.
		 *
		 * 	Constructor. Create an openCL bundle *.
		 **/
		OpenCLBundle(bool selectdefault = true, bool output = true, bool showextensions = false);


		/**
		 * Maximum work group size.
		 **/
		int maxWorkGroupSize() const { return (int)device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>(); }


		/**
		* Create an OpensCL programm file a source file.
		*
		* @param	bundle		   	The bundle.
		* @param	filename	   	name of the file.
		* @param	compileroptions	compiler options.
		* @param	output		   	true to output to mtools::cout.
		*
		* @return	A cl::Program. a log file named "filename.log"  is created is the log returned by the
		* 			compiler is not empty.
		**/
		cl::Program createProgramFromFile(const std::string & filename, std::string compileroptions = "", bool output = true);


		/**
		 * Creates program from the source given in a std::string.
		 *
		 * @param	source		   	programm source.
		 * @param	log   		   	std::string to receive the log file.
		 * @param	compileroptions	The compiler options.
		 * @param	output		   	true to output to mtools::cout.
		 *
		 * @return	A cl::Program.
		 **/
		cl::Program createProgramFromString(const std::string & source, std::string & log, std::string compileroptions = "", bool output = true);


		/**
		* Extract a CL kernel from a program
		*
		* @param [in,out]	prog	The programm.
		* @param	kernelName  	Name of the kernel to extract
		* @param	output			true to output to mtools::cout
		*
		* @return	A cl::Kernel.
		**/
		cl::Kernel createKernel(cl::Program & prog, const std::string & kernelName, bool output = true);


		};


	/**
	* Select an openCL platform.
	*
	* @param	selectdefault	true to select the default platform if mor than one.
	* @param	output		 	true to output to mtools:cout.
	*
	* @return	A cl::Platform.
	**/
	cl::Platform openCL_selectPlatform(bool selectdefault = true, bool output = true, bool showextensions = false);


	/**
	* Select an openCL device.
	*
	* @param	platform	 	The platform on which the device must reside.
	* @param	selectdefault	true to select the default GPU device
	* @param	output		 	true to output to mtools:cout.
	*
	* @return	A cl::Device.
	**/
	cl::Device openCL_selectDevice(const cl::Platform  & platform, bool selectdefault = true, bool output = true, bool showextensions = false);


	/**
	* Create an openCL context.
	*
	* @param	device	The device for which the context is created.
	*
	* @return	A cl::Context.
	**/
	cl::Context openCL_createContext(const cl::Device & device, bool output = true);


	/**
	* Create an openCL queue with profiling enabled.
	*
	* @param	device 	The device.
	* @param	context	The context.
	* @param	output 	true to output to mtools::cout
	*
	* @return	A cl::CommandQueue.
	**/
	cl::CommandQueue openCL_createQueue(const cl::Device & device, const cl::Context & context, bool output = true);


	}


#endif

/* end of file */


