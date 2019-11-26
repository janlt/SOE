/**
 * @file    soelogger.hpp
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
 * soelogger.hpp
 *
 *  Created on: Sep 19, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOE_LOGGER_INCLUDE_SOELOGGER_HPP_
#define SOE_SOE_SOE_SOE_LOGGER_INCLUDE_SOELOGGER_HPP_

//
// The purpose of this library is to facilitate the use of log, in particular
// formatting the arguments, via the C++ ostringstream, which defines extractors
// for all the build-in C/C++ types. The user still has freedom to change the
// formatting on the fly via the formatting flags, e.g. std::hex, etc.
//

using namespace std;

namespace SoeLogger {
class LogClear;
class LogDebug;
class LogError;
class LogInfo;
}

ostream &operator << (ostream &out, const SoeLogger::LogClear &_clear);
ostream &operator << (ostream &out, const SoeLogger::LogDebug &_debug);
ostream &operator << (ostream &out, const SoeLogger::LogError &_error);
ostream &operator << (ostream &out, const SoeLogger::LogInfo &_info);

namespace SoeLogger {

#define CLEAR_DEFAULT_ARGS  __FILE__,__func__, __LINE__

class Logger;

class LogClear
{
    friend class Logger;

public:
    const char *file;
    const char *func;
    int         line;

public:
    LogClear(const char *_file, const char *_func, int _line);
    virtual ~LogClear();

    friend ostream &::operator << (ostream &out, const LogClear &_clear);
};

class LogDebug
{
public:
    LogDebug();
    virtual ~LogDebug();

    friend ostream &::operator << (ostream &out, const LogDebug &_debug);
};

class LogError
{
public:
    LogError();
    virtual ~LogError();

    friend ostream &::operator << (ostream &out, const LogError &_debug);
};

class LogInfo
{
public:
    LogInfo();
    virtual ~LogInfo();

    friend ostream &::operator << (ostream &out, const LogInfo &_debug);
};

class Logger: public ostringstream
{
public:
    Logger();
    virtual ~Logger();

    ostringstream &operator << (const LogClear &_clear);
    ostringstream &operator << (const LogDebug &_debug);
    ostringstream &operator << (const LogError &_error);
    ostringstream &operator << (const LogInfo &_info);

    ostringstream &Clear(const char *_file, const char *_func, int _line);
    void Debug();
    void Error();
    void Info();
};

}

#endif /* SOE_SOE_SOE_SOE_LOGGER_INCLUDE_SOELOGGER_HPP_ */
