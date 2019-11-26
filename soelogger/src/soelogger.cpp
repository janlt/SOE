/**
 * @file    soelogger.cpp
 * @author  Jan Lisowiec <jlisowiec@gmail.com>
 * @version 1.0
 * @brief   Soe Session API
 *
 *
 * @section Copyright
 *
 *   Copyright Notice:
 *
 *    Copyright 2014-2019 Jan Lisowiec jlisowiec@gmail.com,
 *                   
 *
 *    This product is free software; you can redistribute it and/or,
 *    modify it under the terms of the GNU Library General Public
 *    License as published by the Free Software Foundation; either
 *    version 2 of the License, or (at your option) any later version.
 *    This software is distributed in the hope that it will be useful.
 *
 *
 * @section Licence
 *
 *   GPL 2 or later
 *
 *
 * @section Description
 *
 *   soe_soe
 *
 * @section Source
 *
 *    
 *    
 *
 */

/*
 * soelogger.cpp
 *
 *  Created on: Sep 19, 2017
 *      Author: Jan Lisowiec
 */


#include <iostream>
#include <sstream>
#include <string>

#include "soelogger.hpp"

#include "soeutil.h"

using namespace std;

namespace SoeLogger {
class LogClear;
class LogDebug;
class LogError;
class LogInfo;
}

ostream &operator << (ostream &out, const SoeLogger::LogClear &_clear)
{
    dynamic_cast<ostringstream &>(out).str("");
    out << _clear.file << ":" << _clear.line << " " << _clear.func << "(): ";
    return out;
}

ostream &operator << (ostream &out, const SoeLogger::LogDebug &_debug)
{
    soe_log(SOE_DEBUG, "%s\n", dynamic_cast<ostringstream &>(out).str().c_str());
    return out;
}

ostream &operator << (ostream &out, const SoeLogger::LogError &_error)
{
    soe_log(SOE_ERROR, "%s\n", dynamic_cast<ostringstream &>(out).str().c_str());
    return out;
}

ostream &operator << (ostream &out, const SoeLogger::LogInfo &_info)
{
    soe_log(SOE_INFO, "%s\n", dynamic_cast<ostringstream &>(out).str().c_str());
    return out;
}

namespace SoeLogger {

LogClear::LogClear(const char *_file, const char *_func, int _line)
    :file(_file),
     func(_func),
     line(_line)
{}

LogClear::~LogClear()
{}

LogDebug::LogDebug()
{}

LogDebug::~LogDebug()
{}

LogError::LogError()
{}

LogError::~LogError()
{}

LogInfo::LogInfo()
{}

LogInfo::~LogInfo()
{}

Logger::Logger()
{}

Logger::~Logger()
{}

ostringstream &Logger::Clear(const char *_file, const char *_func, int _line)
{
    str("");
    *this << _file << ":" << _line << " " << _func << "(): ";
    return *this;
}

ostringstream &Logger::operator << (const LogClear &_clear)
{
    *this << _clear.file << ":" << _clear.line << " " << _clear.func << "(): ";
    return *this;
}

ostringstream &Logger::operator << (const LogDebug &_debug)
{
    soe_log(SOE_DEBUG, "%s\n", this->str().c_str());
    return *this;
}

ostringstream &Logger::operator << (const LogError &_error)
{
    soe_log(SOE_ERROR, "%s\n", this->str().c_str());
    return *this;
}

ostringstream &Logger::operator << (const LogInfo &_info)
{
    soe_log(SOE_INFO, "%s\n", this->str().c_str());
    return *this;
}

void Logger::Debug()
{
    soe_log(SOE_DEBUG, "%s\n", this->str().c_str());
}

void Logger::Error()
{
    soe_log(SOE_ERROR, "%s\n", this->str().c_str());
}

void Logger::Info()
{
    soe_log(SOE_INFO, "%s\n", this->str().c_str());
}

}
