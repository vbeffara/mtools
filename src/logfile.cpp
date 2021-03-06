/** @file logfile.cpp */
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


#include "io/logfile.hpp"
#include "misc/stringfct.hpp"




#if defined (_MSC_VER) 
#pragma warning (push)
#pragma warning (disable:4996)
#endif

namespace mtools
{


        LogFile::LogFile(const std::string & fname, bool append, bool writeheader, bool delayfilecreation, StringEncoding wenc) : _m_filename(fname), _m_wenc(wenc), _m_log(nullptr), _append(append), _header(writeheader)
            {
            if (!delayfilecreation) { _openfile(); }
            return;
            }


        LogFile::~LogFile() { delete _m_log; }


        void LogFile::_openfile()
            {
            if (_m_log != nullptr) return;
            _m_log = new std::ofstream(_m_filename, std::ofstream::out | (_append ? std::ofstream::app : std::ofstream::trunc));
            if (_header)
                {
                std::time_t result = std::time(nullptr);
                (*this) << "\n"
                    << "*************************************************************\n"
                    << "Log file [" + _m_filename + "] created "
                    << std::asctime(std::localtime(&result))
                    << "*************************************************************\n";
                }
            }

      
}


#if defined (_MSC_VER) 
#pragma warning (pop)
#endif

/* end of file */

