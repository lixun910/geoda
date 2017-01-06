
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
 
 Run:
 1.Set env to the path where py files are located. Add / in the end.
 2.py file name contains no "_", DO NOT Add .py in the end, except for the runfile function.
 3. Initialize Python in the very beginning, finalize in the end. Do each once.
 **/

#include "python2.7/Python.h"
#include "python2.7/object.h"
#include <stdlib.h>
#define _DEBUG_

/** Report Errors */
void ReportError(char *msg)
{
    printf("Error: %s\n",msg);
}

/** set the environment. workingspace is where the py files are located */
void InitializePython()
{
    Py_Initialize(); //Initialize Python Interpreter.
}
int InitializePythonEnv(char *workingspace)
{
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
    Py_DECREF(pName);
    if (pModule == nullptr)
    {
        ReportError("Import Module Failed");
        FinalizePython();
        return 0;
    }
    
    pFunc = PyObject_GetAttrString(pModule, moduleName);
    Py_DECREF(pModule);
    if (!(pFunc && PyCallable_Check(pFunc)))
    {
        ReportError("Function not callable");
        FinalizePython();
        return 0;
    }
    
    PyObject *tempResult = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pFunc);
    if (tempResult != nullptr)
    {
        *pResult = tempResult;
        //Result returned correctly
        //printf("Result of call: %ld\n", PyInt_AsLong(tempResult));
    }
    else
    {
        ReportError("Call failed");
        Py_DECREF(tempResult);
        FinalizePython();
        return 0;
    }
    Py_DECREF(tempResult);
    return 1;
}


/************************* Operations on argument pointer  *************************/
/** Set an argument pointer with value pVal at position i*/
void Python_Set_One_Arg(PyObject *pArgs, int position, PyObject *pVal)
{
    PyTuple_SetItem(pArgs, position, pVal);
}

/** Initialize a pointer with n arguments*/
PyObject* Python_Init_Arg_Pointer(int numArgs)
{
    return PyTuple_New(numArgs);
}


/************************* Create certain argument type  *************************/
/** Create an integer argument */
PyObject* Python_Get_Int_From_Int(int num)
{
    return Py_BuildValue("i", num);
}

/** Create an Float argument */
PyObject* Python_Get_Float_From_Float(float num)
{
    return Py_BuildValue("f", num);
}

/** Create an String argument */
PyObject* Python_Get_String_From_String(char *str)
{
    return Py_BuildValue("s", str);
}

/** Create an List argument */
PyObject* Python_Create_List(int numElements)
{
    return PyList_New(numElements);
}
/** Add Element to List */
void Python_Add_List_Element(PyObject*pList, int position, PyObject* element)
{
    PyList_SetItem(pList,position,element);
}
/** Append Element to List */
void Python_Append_List_Element(PyObject*pList, PyObject* element)
{
    PyList_Append(pList,element);
}


#ifdef _DEBUG_
int main(int argc, char *argv[])
{
    
    InitializePython();
    
    PyObject *pValue=nullptr,*pArgs=nullptr,*pValue1,*pValue2,*pValue3,*pValue4;
    PyObject *pResult,*pList1,*pList2;
    
    
    
    printf("Test1:Test RunFile Function\n");
    Python_RunFile("/Users/Enze/Desktop/tst/TestPython/TestPython","/Users/Enze/Desktop/tst/TestPython/TestPython/runFileMain.py");
    printf("\n\n");
    
    
    printf("Test2:Test RunModule Function without Args\n");
    Python_RunModule("/Users/Enze/Desktop/tst/TestPython/TestPython/", "noArgMain", "main", nullptr,&pResult);
    printf("\n\n");
    
    
    
    printf("Test3:Test RunModule Function with Integer Args\n");
    pArgs = Python_Init_Arg_Pointer(2);
    pValue = Python_Get_Int_From_Int(4); //arg1 = 4
    Python_Set_One_Arg(pArgs,0,pValue); //set arg 1
    pValue = Python_Get_Int_From_Int(5); //arg2 = 5
    Python_Set_One_Arg(pArgs,1,pValue); //set arg 2
    Python_RunModule("/Users/Enze/Desktop/tst/TestPython/TestPython/", "intArgMain", "main", pArgs,&pResult); //run
    printf("Result of call: %ld\n", PyInt_AsLong(pResult));
    printf("\n\n");
    
    
    printf("Test4:Test RunModule Function with String Args\n");
    pArgs = Python_Init_Arg_Pointer(2);
    pValue = Python_Get_String_From_String("Hello"); //arg1 = 4
    Python_Set_One_Arg(pArgs,0,pValue); //set arg 1
    pValue = Python_Get_String_From_String("World"); //arg2 = 5
    Python_Set_One_Arg(pArgs,1,pValue); //set arg 2
    
    Python_RunModule("/Users/Enze/Desktop/tst/TestPython/TestPython/", "stringArgMain", "main", pArgs,&pResult); //run
    printf("Result of call: %ld\n", PyInt_AsLong(pResult));
    printf("\n\n");
    
    
    printf("Test5:Test RunModule Function with List Args\n");
    pArgs = Python_Init_Arg_Pointer(2);
    pList1 = Python_Create_List(2);
    pValue1 = Python_Get_String_From_String("List1"); //List1[0] = "List1"
    Python_Add_List_Element(pList1, 0, pValue1);
    pValue2 = Python_Get_Int_From_Int(6);
    Python_Add_List_Element(pList1, 1, pValue2); //List1[1] = 6
    Python_Set_One_Arg(pArgs,0,pList1); //set arg 1 = list1
    
    
    pList2 = Python_Create_List(2);
    pValue3 = Python_Get_Int_From_Int(7); //List2[0] = 7
    Python_Add_List_Element(pList2,0,pValue3);
    pValue4 = Python_Get_String_From_String("List2"); //List2[1] = "List2"
    Python_Add_List_Element(pList2,1,pValue4);
    Python_Set_One_Arg(pArgs,1,pList2); //set arg 2 = list2
    
    Python_RunModule("/Users/Enze/Desktop/tst/TestPython/TestPython/", "listArgMain", "main", pArgs,&pResult); //run
    printf("Result of call: %ld\n", PyInt_AsLong(pResult));
    printf("\n\n");
    
    
    
    FinalizePython();
    return 0;
}
#endif
