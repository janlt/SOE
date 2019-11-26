/**
 * @file    soesessiontransaction.hpp
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
 * soesessiontransaction.hh
 *
 *  Created on: Feb 3, 2017
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOE_INCLUDE_SOESESSIONTRANSACTION_HPP_
#define SOE_SOE_SOE_SOE_INCLUDE_SOESESSIONTRANSACTION_HPP_

namespace Soe {

/**
 * @brief Transaction class
 *
 * This class provides transaction control on session operations.
 *
 * An instance of Transaction is created via StartTransaction() method of Session class. There might be
 * multiple instances of Transaction running at any given point in time within an application. Transactions,
 * unlike groups, provide synchronization and checking mechanisms. If transactions "A" reads a key that is
 * later modified by transaction "B" then transaction "A" will fail on Commit, for example. Transaction "A"
 * may lock a key in which case transaction "B" will get an error when trying to modify the key. Transactions
 * are committed or aborted via corresponding Session methods. Upon Committing a transaction all changes made
 * to the underlying store become permanent. Upon aborting a transaction, all changes are discarded.
 *
 */
class Transaction
{
friend class Session;

private:
    uint32_t                       transaction_no;
    const static uint32_t          invalid_transaction = -1;
    std::shared_ptr<Session>       sess;

    Transaction(std::shared_ptr<Session> _sess, uint32_t _gr = Transaction::invalid_transaction);
    ~Transaction();

public:
    Session::Status Put(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error);
    Session::Status Get(const char *key, size_t key_size) throw(std::runtime_error);
    Session::Status Merge(const char *key, size_t key_size, const char *data, size_t data_size) throw(std::runtime_error);
    Session::Status Delete(const char *key, size_t key_size) throw(std::runtime_error);

    Session *GetSession() { return sess.get(); }
};

}

#endif /* SOE_SOE_SOE_SOE_INCLUDE_SOESESSIONTRANSACTION_HPP_ */
