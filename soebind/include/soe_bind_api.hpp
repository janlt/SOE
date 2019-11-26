/*
 * soe_bind_api.hpp
 *
 *  Created on: Nov 28, 2016
 *      Author: Jan Lisowiec
 */

#ifndef SOE_SOE_SOE_SOE_API_INCLUDE_SOE_SOE_API_HPP_
#define SOE_SOE_SOE_SOE_API_INCLUDE_SOE_SOE_API_HPP_

#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <map>

typedef boost::shared_mutex Lock;
typedef boost::unique_lock< Lock > WriteLock;
typedef boost::shared_lock< Lock > ReadLock;

namespace SoeApi {

//
// This API departs from the traditional way in a sense that it allows binding an arbitrary function
// having an arbitrary signature through one of the available slots provided by the API.
// It's a matter of an agreement between the parties regarding what API functions and signatures
// are registered and how they should be called. API providers can dynamically register new functions
// at known indexes, or they can remove functions when they are no longer needed or required.
//
// An API function can be registered by either number or name. Duplicate name or number will raise
// an exception. See examples in soe_bind_api_test.cpp
//
// This API also performs argument type and signature checking of the registered function against the
// called one.
//

typedef uint32_t ApiFuncNum;
#ifdef __NO_STD_MAP__
const ApiFuncNum apiFuncMax = 10000;
#endif

template <typename T> class Api;

class Exception
{
    std::string error;

public:
    Exception(const std::string err = "Know nothing"): error(err) {}
    ~Exception() {}
    const std::string &what() const { return error; }
};

class ApiBase
{
    public:
        ApiBase(ApiFuncNum fn = 0, const std::string &nm = ""):
            func_no(fn),
            name(nm) { /*std::cout << __FUNCTION__ << std::endl;*/ }
        ApiBase(const std::string &nm) throw(Exception);
        virtual ~ApiBase() { /*std::cout << __FUNCTION__ << std::endl;*/ }
        virtual ApiFuncNum GetFuncNo() { return func_no; }

        class Initializer {
        public:
            static bool initialized;
            Initializer();
        };
        static Initializer initalized;
#ifdef __NO_STD_MAP__
        static std::unique_ptr<ApiBase> functions[apiFuncMax];
#endif
        static std::map<ApiFuncNum, ApiBase*>     functions;
        static std::map<std::string, ApiFuncNum>  function_names;

    protected:
        virtual void Register() throw(Exception) {}
        virtual void Deregister() throw(Exception) {}
        static ApiFuncNum SearchFunctionNo(const std::string &nm) throw(Exception);

    public:
        ApiFuncNum      func_no;
        std::string     name;
        std::string     signature;
        static uint32_t fun_next;
        static Lock     instance_lock;
};

typedef int (ApiBase::*FuncCallBack)(ApiBase &, int);

template <typename T> std::ostream & operator << (std::ostream &os, const Api<T> &r)
{
#ifdef __NO_STD_MAP__
    for ( ApiFuncNum i = 0 ; i < apiFuncMax ; i++ ) {
        os << i << " ";
        if ( r.functions[i].get() && r.func_no == r.functions[i]->func_no ) {
            Api<T> *a = dynamic_cast<Api<T> *>(r.functions[i].get());
            os << " reg: " << a->registered;
        } else {
            os << "get(): " << r.functions[i].get();
        }
        os << std::endl;
    }
#endif
    WriteLock w_lock(ApiBase::instance_lock);

    for ( auto it = r.functions.begin() ; it != r.functions.end() ; ++it ) {
        os << (*it).first << " " << (*it).second << " ";
        Api<T> *a = dynamic_cast<Api<T> *>((*it).second);
        os << " reg: " << a->registered;
    }
    return os << " f_n: " << r.func_no << " name: " << r.name <<
        " sig: " << r.signature << " reg: " << r.registered << " db: " << r.debug << " ";
}

/*
 * Api class template
 */
template <typename T> class Api: public ApiBase
{
    protected:
        virtual void Register() throw(Exception);
        virtual void Deregister() throw(Exception);

    public:
        Api(boost::function<T> f,
                ApiFuncNum f_no,
                bool db = false,
                FuncCallBack dereg_func = NULL) throw(Exception):
            ApiBase(f_no),
            func(f),
            dereg_callback(dereg_func),
            registered(false),
            debug(db)
        {
            Register();
        }

        Api(boost::function<T> f,
                const std::string &nm,
                bool db = false,
                FuncCallBack dereg_func = NULL) throw(Exception):
            ApiBase(nm),
            func(f),
            dereg_callback(dereg_func),
            registered(false),
            debug(db)
        {
            Register();
        }

        virtual ~Api()
        {
            try {
                Deregister();
            } catch (const Exception &ex) {
                ;
            }
        }

        virtual ApiFuncNum GetFuncNo() { return ApiBase::func_no; }
        boost::function<T> &GetFunc() { return func; }

        friend std::ostream & operator << <T>(std::ostream &os, const Api<T> &r);

        static boost::function<T> &GetFunction(ApiFuncNum func_no) throw(Exception)
        {
            WriteLock w_lock(ApiBase::instance_lock);

#ifdef __NO_STD_MAP__
            if ( func_no >= apiFuncMax ) {
                throw Exception("Function number out-of-bounds");
            }
            ApiBase *b = functions[func_no].get();
#endif
            std::map<ApiFuncNum, ApiBase*>::iterator it = functions.find(func_no);
            if ( it == functions.end() ) {
                throw Exception("Function not available");
            }
            ApiBase *b = (*it).second;
            if ( !b ) {
                throw Exception("Function not available");
            }
#ifdef __NO_STD_MAP__
            if ( boost::typeindex::type_id<T>().pretty_name() != functions[func_no]->signature ) {
#endif
            if ( boost::typeindex::type_id<T>().pretty_name() != b->signature ) {
#ifdef __NO_STD_MAP__
                throw Exception("Wrong signature: registered: " + functions[func_no]->signature +
                    " called: " + boost::typeindex::type_id<T>().pretty_name());
#endif
                throw Exception("Wrong signature: registered: " + b->signature +
                    " called: " + boost::typeindex::type_id<T>().pretty_name());
            }
            Api<T> *a = dynamic_cast<Api<T> *>(b);
            return a->GetFunc();
        }

        static std::string &GetFunctionSignature(ApiFuncNum func_no) throw(Exception)
        {
            WriteLock w_lock(ApiBase::instance_lock);

#ifdef __NO_STD_MAP__
            if ( func_no >= apiFuncMax ) {
                throw Exception("Function number out-of-bounds");
            }
            ApiBase *b = functions[func_no].get();
#endif
            std::map<ApiFuncNum, ApiBase*>::iterator it = functions.find(func_no);
            if ( it == functions.end() ) {
                throw Exception("Function not available");
            }
            ApiBase *b = (*it).second;
            if ( !b ) {
                throw Exception("Function not available");
            }
#ifdef __NO_STD_MAP__
            return functions[func_no]->signature;
#endif
            return b->signature;
        }

        static boost::function<T> &SearchFunction(const std::string &nm) throw(Exception)
        {
            ApiFuncNum f_no = ApiBase::SearchFunctionNo(nm);
#ifdef __NO_STD_MAP__
            if ( f_no >= apiFuncMax ) {
                throw Exception("No such function name: " + nm);
            }
            ApiBase *b = functions[f_no].get();
#endif
            WriteLock w_lock(ApiBase::instance_lock);

            std::map<ApiFuncNum, ApiBase*>::iterator it = functions.find(f_no);
            if ( it == functions.end() ) {
                throw Exception("Function not available");
            }
            ApiBase *b = (*it).second;
            Api<T> *a = dynamic_cast<Api<T> *>(b);
            if ( !a ) {
                throw Exception("Function NULL");
            }
            return a->GetFunc();
        }

    private:
        boost::function<T> func;
        FuncCallBack       dereg_callback;
        bool               registered;
        bool               debug;
};

#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

template <typename T> void Api<T>::Register() throw(Exception)
{
    //if ( this->debug == true ) {
        //std::cout << __FUNCTION__ << " " << this->name << " " << this->GetFuncNo() << std::endl << std::flush;
    //}

    WriteLock w_lock(ApiBase::instance_lock);

#ifdef __NO_STD_MAP__
    if ( this->GetFuncNo() >= apiFuncMax ) {
        throw Exception("Function number out-of-bounds");
    }
#endif

    // Make sure there are no duplicates
    std::map<std::string, ApiFuncNum>::iterator it = function_names.find(this->name);
    if ( it != function_names.end() ) {
        throw Exception("Duplicate name detected: name: " + name);
    }

    signature = boost::typeindex::type_id<T>().pretty_name();

#ifdef __NO_STD_MAP__
    functions[this->GetFuncNo()] = std::unique_ptr<ApiBase>(this);
#endif
    functions.insert(std::pair<ApiFuncNum, ApiBase*>(this->GetFuncNo(), this));
    function_names.insert(std::pair<std::string, ApiFuncNum>(this->name, this->GetFuncNo()));

    registered = true;
}

template <typename T> void Api<T>::Deregister() throw(Exception)
{
    //if ( this->debug == true ) {
        //std::cout << __FUNCTION__ << " " << this->name << " " << this->GetFuncNo() << std::endl << std::flush;
    //}

    WriteLock w_lock(ApiBase::instance_lock);

#ifdef __NO_STD_MAP__
    if ( this->GetFuncNo() >= apiFuncMax ) {
        throw Exception("Function number out-of-bounds");
    }
#endif
    if ( dereg_callback ) {
        CALL_MEMBER_FN(*this, dereg_callback)(*this, 0);
    }
    std::map<ApiFuncNum, ApiBase*>::iterator it = functions.find(this->GetFuncNo());
    if ( it == functions.end() ) {
        throw Exception("Function number not found");
    }

    functions.erase(this->GetFuncNo());
    function_names.erase(this->name);
    registered = false;
#ifdef __NO_STD_MAP__
    functions[this->GetFuncNo()].reset();
#endif
}

};

#endif /* SOE_SOE_SOE_SOE_API_INCLUDE_SOE_SOE_API_HPP_ */
