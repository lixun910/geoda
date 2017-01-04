
//  PythonIntegration.cpp
//  PythonTest
//
//  Created by Enze on 1/2/17.
//  Copyright (c) 2017 Enze. All rights reserved.


/**
 Readme:
 Things need to be done before building the code.
 1.Link libpython2.7.dylib
 2.Add header path to the building env
 3.Add Snipet cpde to Change the Env, so that the .py files do not have to be in the same folder with the executable files.
 4.//Add libs to building env
 **/

#include "python2.7/Python.h"
#include <stdlib.h>
#define _DEBUG_

/** Report Errors */
void ReportError(char *msg)
{
    printf("Error: %s\n",msg);
}

/** set the environment. workingspace is where the py files are located */
int InitializePythonEnv(char *workingspace)
{
    Py_Initialize(); //Initialize Python Interpreter.
    PyRun_SimpleString("import sys"); //run a simple python code
    
    char *cmd = (char *)malloc((strlen(workingspace)+50)*sizeof(char));
    memset(cmd,0,sizeof((strlen(workingspace)+50)*sizeof(char)));
    strcat(cmd, "sys.path.insert(0,'");
    strcat(cmd,workingspace);
    strcat(cmd,"')");
    //cmd = sys.path.insert(0,path/to/the/pyfiles) so that the file python files can be found in certain path
    
    PyRun_SimpleString(cmd);
    return 1;
}

/** disbale the interpreter */
void FinalizePython()
{
    Py_Finalize();
}

/** 
 Run a entire .py file
 
 envAddress: System path. Use "." if not need to set specifically.
 fileAddress: Address to the python file. Include .py. DO NOT Use "_" for filename
 */
int Python_RunFile(char *envAddress,char *fileAddress)
{
    
    
    if (InitializePythonEnv(envAddress) == 0) //init failed
    {
        ReportError("initializing environment failed");
        FinalizePython();
        return 0;
    }
    
    FILE* pythonFilePtr = nullptr;
    pythonFilePtr = fopen(fileAddress, "r");
    
    if (pythonFilePtr == nullptr) //open failed
    {
        ReportError("Open python file failed");
        FinalizePython();
        return 0;
    }
    
    PyRun_SimpleFile(pythonFilePtr, fileAddress); //execute the py file.
    
    FinalizePython();
    return 1;
}

/**
 Run a module from a python file.
 
 envAddress: System path (where the py files are located). Use "." if not need to set specifically.
 fileAddress: Address to the python file. DONT NOT Add .py after the name
 moduleName: User defined args.
 pArgs: Argument sets
 pResult: should point to the return value
 
 */
int Python_RunModule(char *envAddress,char *fileAddress, char *moduleName,PyObject *pArgs, PyObject **pResult)
{
    
    PyObject *pName = nullptr,*pModule=nullptr,*pFunc=nullptr;
    
    if (InitializePythonEnv(envAddress) == 0)
    {
        ReportError("initializing environment failed");
        FinalizePython();
        return 0;
    }
    
    pName = PyString_FromString(fileAddress);
    if (pName == nullptr)
    {
        ReportError("Translate File Name Failed");
        FinalizePython();
        return 0;
    }
    
    pModule = PyImport_Import(pName);
    if (pModule == nullptr)
    {
        ReportError("Import Module Failed");
        FinalizePython();
        return 0;
    }
    
    pFunc = PyObject_GetAttrString(pModule, moduleName);
    if (!(pFunc && PyCallable_Check(pFunc)))
    {
        ReportError("Function not callable");
        FinalizePython();
        return 0;
    }
    
    PyObject *tempResult = PyObject_CallObject(pFunc, pArgs);
    
    if (tempResult != nullptr)
    {
        *pResult = tempResult;
        //Result returned correctly
        //printf("Result of call: %ld\n", PyInt_AsLong(tempResult));
    }
    else
    {
        ReportError("Call failed");
        FinalizePython();
        return 0;
    }
    
    FinalizePython();
    return 1;
}


/** Set an argument pointer with value pVal at position i*/
void Set_Arg(PyObject *pArgs, int position, PyObject *pVal)
{
    PyTuple_SetItem(pArgs, position, pVal);
}

/** Initialize a pointer with n arguments*/
PyObject* Init_Args(int numArgs)
{
    return PyTuple_New(numArgs);
}

/** Create an integer argument */
PyObject* Get_Int(long num)
{
    return PyInt_FromLong(num);
}


#ifdef _DEBUG_
int main(int argc, char *argv[])
{
    
    
    PyObject *pValue=nullptr,*pArgs=nullptr;
    PyObject *pResult=(PyObject *)malloc(sizeof(PyObject));
    
    printf("Test1:Test RunFile Function\n");
    Python_RunFile("/Users/Enze/Code/geoda/PythonCode/PythonTest/PythonTest/PythonTest/","/Users/Enze/Code/geoda/PythonCode/PythonTest/PythonTest/PythonTest/runFileMain.py");
    printf("\n\n");
    
    
    printf("Test2:Test RunModule Function without Args\n");
    Python_RunModule("/Users/Enze/Code/geoda/PythonCode/PythonTest/PythonTest/PythonTest/", "noArgMain", "main", nullptr,&pResult);
    printf("\n\n");
    
    
    
    printf("Test3:Test RunModule Function with Integer Args\n");
    
    pArgs = Init_Args(2);
    pValue = Get_Int(4); //arg1 = 4
    Set_Arg(pArgs,0,pValue); //set arg 1
    pValue = Get_Int(5); //arg2 = 5
    Set_Arg(pArgs,1,pValue); //set arg 2
    
    Python_RunModule("/Users/Enze/Code/geoda/PythonCode/PythonTest/PythonTest/PythonTest/", "intArgMain", "main", pArgs,&pResult);
    printf("Result of call: %ld\n", PyInt_AsLong(pResult));
    printf("\n\n");
    
    
}
#endif
