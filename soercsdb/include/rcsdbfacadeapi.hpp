/*
 * rcsdbfacadeapi.hpp
 *
 *  Created on: Dec 16, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBFACADEAPI_HPP_
#define SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBFACADEAPI_HPP_

namespace RcsdbFacade {

class Exception
{
public:
    typedef enum eStatus {
        eOk                        = 0,
        eDuplicateKey              = 1,
        eNotFoundKey               = 2,
        eTruncatedValue            = 3,
        eStoreCorruption           = 4,
        eOpNotSupported            = 5,
        eInvalidArgument           = 6,
        eIOError                   = 7,
        eMergeInProgress           = 8,
        eIncomplete                = 9,
        eShutdownInProgress        = 10,
        eOpTimeout                 = 11,
        eOpAborted                 = 12,
        eStoreBusy                 = 13,
        eItemExpired               = 14,
        eTryAgain                  = 15,
        eInternalError             = 16,
        eOsError                   = 17,
        eOsResourceExhausted       = 18,
        eProviderResourceExhausted = 19,
        eFoobar                    = 20,
        eInvalidStoreState         = 21,
        eProviderUnreachable       = 22,
        eChannelError              = 23,
        eNetworkError              = 24,
        eNoFunction                = 25,
        eDuplicateIdDetected       = 26
    } eStatus;

    std::string error;
    eStatus     status;
    int         os_errno;


public:
    Exception(const std::string err = "Know nothing", eStatus sts = eInternalError): error(err), status(sts) { os_errno = errno; }
    virtual ~Exception() {}
    virtual const std::string &what() const { return error; }
};

}



#endif /* SOE_SOE_SOE_RCSDB_INCLUDE_RCSDBFACADEAPI_HPP_ */
