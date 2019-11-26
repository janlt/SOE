/*
 * soe_soe_api_test.cpp
 *
 *  Created on: Nov 28, 2016
 *      Author: Jan Lisowiec
 */

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_index.hpp>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#include "soe_bind_api.hpp"

using namespace std;

class ClassA
{
public:
    int mem;
    ClassA(int a): mem(a + 100) {}
};

class TestClass
{
    int mem1;
    std::string mem2;

public:
    TestClass(int m1 = 0, std::string m2 = "dupek"): mem1(m1), mem2(m2) {}
    int MyFunc1(const char *s);
    int MyFunc2(const char *s, int i);
    void MyFunc3(const char *s, const ClassA &a, int i);
    void MyFuncVoid();
};

int TestClass::MyFunc1(const char *s)
{
    std::cout << "mem1: " << mem1 << " mem2: " << mem2 <<
        " s: " << s << " ";
    return 22;
}

int TestClass::MyFunc2(const char *s, int i)
{
    std::cout << "mem1: " << mem1 << " mem2: " << mem2 <<
        " s: " << s << " i: " << i << " ";
    return 356;
}

void TestClass::MyFunc3(const char *s, const ClassA &a, int i)
{
    std::cout << "mem1: " << mem1 << " mem2: " << mem2 <<
        " s: " << s << " i: " << i << " a.mem: " << a.mem << " ";
}

int main(int argc, char **argv)
{
    TestClass my_dupek;

    // bind functions to boost functors
    boost::function<int(const char*)> f1 = boost::bind(&TestClass::MyFunc1, &my_dupek, _1);
    boost::function<int(const char*, int)> f2 = boost::bind(&TestClass::MyFunc2, &my_dupek, _1, _2);
    boost::function<void(const char*, const ClassA&, int)> f3 = boost::bind(&TestClass::MyFunc3, &my_dupek, _1, _2, _3);

    try {
        new SoeApi::Api<int(const char *)>(f1, 0, true);
        new SoeApi::Api<int(const char *, int)>(f2, 1, true);
        new SoeApi::Api<void(const char *, const ClassA&, int)>(f3, 2, true);

        std::cout << f1("Test1") << std::endl << std::flush;
        std::cout << f2("Test2", 20) << std::endl << std::flush;
        ClassA a1(20);
        f3("Test3", a1, 20); std::cout << std::endl << std::flush;

        // call through SoeApi
        boost::function<int(const char*)> fun1 = SoeApi::Api<int(const char*)>::GetFunction(0);
        std::cout << fun1("Test11") << std::endl << std::flush;

        boost::function<int(const char*, int)> fun2 = SoeApi::Api<int(const char *, int)>::GetFunction(1);
        std::cout << fun2("Test22", 1234) << std::endl << std::flush;

        boost::function<void(const char*, const ClassA&, int)> fun3 =
            SoeApi::Api<void(const char *, const ClassA &, int)>::GetFunction(2);
        ClassA a2(40);
        fun3("Test33", a2, 1234); std::cout << std::endl << std::flush;

        // get functions that hasn't been set - should cause an exception
        boost::function<int(const char*, int, const char *)> bad_fun = SoeApi::Api<int(const char *, int, const char *)>::GetFunction(5);
        std::cout << bad_fun("fuka", 345, "ruka") << std::endl << std::flush;
    } catch (SoeApi::Exception &e) {
        std::cout << "SoeApi::Exception: " << e.what() << std::endl << std::flush;
    } catch (...) {
        std::cout << "Exception ..." << std::endl << std::flush;
    }

    // call SoeApi with wrong function signature
    try {
        std::cout << "Registered signature: " << SoeApi::Api<void()>::GetFunctionSignature(0) << std::endl << std::flush;
        boost::function<int(const char*, int)> fun3 = SoeApi::Api<int(const char*, int)>::GetFunction(0);
        std::cout << fun3("fail", 1) << std::endl << std::flush;
    } catch (const SoeApi::Exception &e) {
        std::cout << "SoeApi::Exception: " << e.what() << std::endl << std::flush;
    } catch (...) {
        std::cout << "Exception ..." << std::endl << std::flush;
    }

    // test by name
    TestClass your_dupek;

    // bind functions to boost functors
    boost::function<void(const char*, const ClassA&, int)> my_api_function = boost::bind(&TestClass::MyFunc3, &your_dupek, _1, _2, _3);

    try {
        new SoeApi::Api<void(const char *, const ClassA&, int)>(my_api_function, typeid(my_api_function).name(), true);

        ClassA a3(50);
        my_api_function("Test4", a3, 60); std::cout << std::endl << std::flush;

        // call it via SoeApi
        boost::function<void(const char*, const ClassA&, int)> searched_fun =
         SoeApi::Api<void(const char *, const ClassA &, int)>::SearchFunction(typeid(my_api_function).name());
        ClassA a2(40);
        searched_fun("Test33", a2, 1234); std::cout << std::endl << std::flush;

        // call it via SoeApi with dab name
        boost::function<void(const char*, const ClassA&, int)> searched_fun_bad =
         SoeApi::Api<void(const char *, const ClassA &, int)>::SearchFunction("BadNameFunction");
        ClassA a4(40);
        searched_fun_bad("fail", a4, 1234); std::cout << std::endl << std::flush;
    } catch (const SoeApi::Exception &e) {
        std::cout << "SoeApi::Exception: " << e.what() << std::endl << std::flush;
    } catch (...) {
        std::cout << "Exception ..." << std::endl << std::flush;
    }

    // test duplicate name
    TestClass fail_dupek;

    // bind functions to boost functors
    boost::function<void(const char*, const ClassA&, int)> my_api_handler = boost::bind(&TestClass::MyFunc3, &fail_dupek, _1, _2, _3);
    boost::function<void(const char*, const ClassA&, int)> my_dupl_function = boost::bind(&TestClass::MyFunc3, &fail_dupek, _1, _2, _3);

    try {
        new SoeApi::Api<void(const char *, const ClassA&, int)>(my_api_handler, typeid(my_api_handler).name(), true);
        new SoeApi::Api<void(const char *, const ClassA&, int)>(my_dupl_function, typeid(my_api_handler).name(), true);

        ClassA a6(50);
        my_api_handler("Test6", a6, 60); std::cout << std::endl << std::flush;
        my_dupl_function("Test7", a6, 60); std::cout << std::endl << std::flush;
    } catch (const SoeApi::Exception &e) {
        std::cout << "SoeApi::Exception: " << e.what() << std::endl << std::flush;
    } catch (...) {
        std::cout << "Exception ..." << std::endl << std::flush;
    }
    return 0;
}

