/**
 * @file    test_logger.cpp
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
 * test_logger.cpp
 *
 *  Created on: Sep 19, 2017
 *      Author: Jan Lisowiec
 */

#include <iostream>
#include <sstream>
#include <string>

#include <boost/thread.hpp>

#include "soelogger.hpp"

#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)

using namespace SoeLogger;
using namespace std;

void Func(Logger &logger)
{
    logger.Clear(CLEAR_DEFAULT_ARGS);
    logger << " Fifth message " << 100.2234 << SoeLogger::LogDebug() << endl;

    logger.Clear(CLEAR_DEFAULT_ARGS) << " Sixth message " << "bla " << SoeLogger::LogError() << endl;

    logger.Clear(CLEAR_DEFAULT_ARGS);
    logger << " Seventh message " << 23432.37 << LogInfo() << endl << flush;
}

void TestLogger()
{
    Logger logger;

    pid_t tid = gettid();

    logger.Clear(CLEAR_DEFAULT_ARGS) << "tid: " << tid << " Again First message " << 200 << endl << flush;
    logger.Debug();

    logger.Clear(CLEAR_DEFAULT_ARGS);
    logger << "tid: " << tid << " Again Second message " << 300 << endl;
    logger.Debug();

    logger.Clear(CLEAR_DEFAULT_ARGS);
    logger << "tid: " << tid << " Again  Third message " << 400 << SoeLogger::LogError() << endl;

    logger.Clear(CLEAR_DEFAULT_ARGS);
    logger << "tid: " << tid << " Again  Fourth message " << 500.37 << LogDebug() << endl << flush;

    Func(logger);

    Logger().Clear(CLEAR_DEFAULT_ARGS) << "tid: " << tid << " Again First message " << 200 << LogError() << endl << flush;

    Logger().Clear(CLEAR_DEFAULT_ARGS) << "tid: " << tid << " Again Second message " << 300 << LogDebug() << endl << flush;

    Logger().Clear(CLEAR_DEFAULT_ARGS) << "tid: " << tid << " Again Third message " << 400 << LogError() << endl << flush;

    Logger().Clear(CLEAR_DEFAULT_ARGS) << "tid: " << tid << " Again Fourth message " << 500.37 << LogDebug() << endl << flush;
}

int main(int argc, char **argv)
{
    uint32_t num_th = 4;
    if ( argc > 1 ) {
        num_th = static_cast<uint32_t>(atoi(argv[1]));
    }
    vector<boost::thread *> threads;
    threads.resize(num_th);

    for ( uint32_t i = 0 ; i < num_th ; i++ ) {
        threads[i] = new boost::thread(&TestLogger);
    }

    for ( uint32_t i = 0 ; i < num_th ; i++ ) {
        threads[i]->join();
        delete threads[i];
    }

    return 0;
}



