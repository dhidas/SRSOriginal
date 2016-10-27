////////////////////////////////////////////////////////////////////
//
// Dean Andrew Hidas <dhidas@bnl.gov>
//
// Created on: Fri Jun 17 10:39:12 EDT 2016
//
// This is the python-C extension which allows access to the c++
// class OSCARS.
//
////////////////////////////////////////////////////////////////////

// Include Python.h first!
#include <Python.h>

#include "OSCARS_Python.h"

#include "OSCARS.h"

#include "TSurfacePoints_Rectangle.h"
#include "TSurfacePoints_3D.h"
#include "T3DScalarContainer.h"
#include "TFieldPythonFunction.h"
#include "TField3D_Gaussian.h"
#include "TField3D_UniformBox.h"
#include "TField3D_IdealUndulator.h"
#include "TRandomA.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>


// External global random generator
extern TRandomA* gRandomA;





static void OSCARS_dealloc(OSCARSObject* self)
{
  // Python needs to know how to deallocate things in the struct

  delete self->obj;
  self->ob_type->tp_free((PyObject*)self);
}




static PyObject* OSCARS_new (PyTypeObject* type, PyObject* args, PyObject* kwds)
{
  // Python needs to know how to create things in this struct

  OSCARSObject* self = (OSCARSObject*) type->tp_alloc(type, 0);
  if (self != NULL) {

    // Create the new object for self
    self->obj = new OSCARS();
  }

  // Return myself
  return (PyObject*) self;
}







static TVector2D OSCARS_ListAsTVector2D (PyObject* List)
{
  TVector2D V;
  if (PyList_Size(List) == 2) {
    Py_INCREF(List);
    V.SetXY(PyFloat_AsDouble(PyList_GetItem(List, 0)),
             PyFloat_AsDouble(PyList_GetItem(List, 1)));
    Py_DECREF(List);
  } else {
    throw std::length_error("number of elements not 2");
  }

  // Return the python list
  return V;
}







static TVector3D OSCARS_ListAsTVector3D (PyObject* List)
{
  TVector3D V;
  if (PyList_Size(List) == 3) {
    Py_INCREF(List);
    V.SetXYZ(PyFloat_AsDouble(PyList_GetItem(List, 0)),
             PyFloat_AsDouble(PyList_GetItem(List, 1)),
             PyFloat_AsDouble(PyList_GetItem(List, 2)));
    Py_DECREF(List);
  } else {
    throw std::length_error("number of elements not 3");
  }

  // Return the python list
  return V;
}







static PyObject* OSCARS_TVector3DAsList (TVector3D const& V)
{
  // Turn a TVector3D into a list (like a vector)

  // Create a python list
  PyObject *PList = PyList_New(0);

  PyList_Append(PList, Py_BuildValue("f", V.GetX()));
  PyList_Append(PList, Py_BuildValue("f", V.GetY()));
  PyList_Append(PList, Py_BuildValue("f", V.GetZ()));

  // Return the python list
  return PList;
}







static PyObject* OSCARS_Pi (OSCARSObject* self, PyObject* arg)
{
  // Return the internal OSCARS number constant pi
  return Py_BuildValue("d", TOSCARS::Pi());
}



static PyObject* OSCARS_Qe (OSCARSObject* self, PyObject* arg)
{
  // Return the internal OSCARS number for elementary charge
  return Py_BuildValue("d", TOSCARS::Qe());
}




static PyObject* OSCARS_Me (OSCARSObject* self, PyObject* arg)
{
  // Return the internal OSCARS number for mass of the electron [kg]
  return Py_BuildValue("d", TOSCARS::Me());
}




static PyObject* OSCARS_Random (OSCARSObject* self, PyObject* arg)
{
  return Py_BuildValue("d", gRandomA->Uniform());
}

static PyObject* OSCARS_RandomNormal (OSCARSObject* self, PyObject* arg)
{
  return Py_BuildValue("d", gRandomA->Normal());
}

static PyObject* OSCARS_SetSeed (OSCARSObject* self, PyObject* arg)
{
  // Grab the value from input
  double Seed = PyFloat_AsDouble(arg);

  gRandomA->SetSeed(Seed);

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}





static PyObject* OSCARS_SetGPUGlobal (OSCARSObject* self, PyObject* arg)
{
  // Grab the value from input
  int const GPU = (int) PyInt_AsLong(arg);

  self->obj->SetUseGPUGlobal(GPU);

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}





static PyObject* OSCARS_CheckGPU (OSCARSObject* self, PyObject* arg)
{

  int const NGPUStatus = self->obj->CheckGPU();

  if (NGPUStatus == -1) {
    // Print copyright notice
    PyObject* sys = PyImport_ImportModule( "sys");
    PyObject* s_out = PyObject_GetAttrString(sys, "stdout");
    std::string Message = "It appears this binary version of OSCARS was not compiled with GPU capability turned on.";
    PyObject_CallMethod(s_out, "write", "s", Message.c_str());
  }

  return PyInt_FromLong((long) NGPUStatus);
}








static PyObject* OSCARS_SetNThreadsGlobal (OSCARSObject* self, PyObject* arg)
{
  // Grab the value from input
  int const NThreads = (int) PyInt_AsLong(arg);

  self->obj->SetNThreadsGlobal(NThreads);

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}














static PyObject* OSCARS_GetCTStart (OSCARSObject* self)
{
  // Get the start time in [m] for calculation
  return Py_BuildValue("d", self->obj->GetCTStart());
}




static PyObject* OSCARS_GetCTStop (OSCARSObject* self)
{
  // Get the CTStop variable from OSCARS
  return Py_BuildValue("d", self->obj->GetCTStop());
}




static PyObject* OSCARS_SetCTStartStop (OSCARSObject* self, PyObject* args)
{
  // Set the start and stop times for OSCARS in [m]

  // Grab the values
  double Start, Stop;
  if (!PyArg_ParseTuple(args, "dd", &Start, &Stop)) {
    PyErr_SetString(PyExc_ValueError, "Incorrect format");
    return NULL;
  }

  // Set the object variable
  self->obj->SetCTStartStop(Start, Stop);

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}






static PyObject* OSCARS_GetNPointsTrajectory (OSCARSObject* self)
{
  // Get the numper of points for trajectory calculaton
  return PyInt_FromSize_t(self->obj->GetNPointsTrajectory());
}




static PyObject* OSCARS_SetNPointsTrajectory (OSCARSObject* self, PyObject* arg)
{
  // Set the number of points for trajectory calculation

  // Grab the value from input
  size_t N = PyInt_AsSsize_t(arg);

  // Set the object variable
  self->obj->SetNPointsTrajectory(N);

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}








static PyObject* OSCARS_AddMagneticField (OSCARSObject* self, PyObject* args, PyObject* keywds)
{
  // Add a magnetic field from a file.
  // UPDATE: add binary file reading


  // Grab the values
  char const* FileName = "";
  char const* FileFormat = "";
  PyObject*   List_Rotations   = PyList_New(0);
  PyObject*   List_Translation = PyList_New(0);
  PyObject*   List_Scaling     = PyList_New(0);

  TVector3D Rotations(0, 0, 0);
  TVector3D Translation(0, 0, 0);
  std::vector<double> Scaling;


  // Input variables and parsing
  static char *kwlist[] = {"ifile", "iformat", "rotations", "translation", "scale", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss|OOO", kwlist,
                                                           &FileName,
                                                           &FileFormat,
                                                           &List_Rotations,
                                                           &List_Translation,
                                                           &List_Scaling)) {
    return NULL;
  }

  // Check that filename and format exist
  if (std::strlen(FileName) == 0 || std::strlen(FileFormat) == 0) {
    PyErr_SetString(PyExc_ValueError, "'ifile' or 'iformat' is blank");
    return NULL;
  }

  // Check for Rotations in the input
  if (PyList_Size(List_Rotations) != 0) {
    try {
      Rotations = OSCARS_ListAsTVector3D(List_Rotations);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'rotations'");
      return NULL;
    }
  }


  // Check for Translation in the input
  if (PyList_Size(List_Translation) != 0) {
    try {
      Translation = OSCARS_ListAsTVector3D(List_Translation);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'translation'");
      return NULL;
    }
  }

  // Get any scaling factors
  // UPDATE: Check against fileformat number of strings
  for (size_t i = 0; i < PyList_Size(List_Scaling); ++i) {
    Scaling.push_back(PyFloat_AsDouble(PyList_GetItem(List_Scaling, i)));
  }

  // Add the magnetic field to the OSCARS object
  try {
    self->obj->AddMagneticField(FileName, FileFormat, Rotations, Translation, Scaling);
  } catch (...) {
    PyErr_SetString(PyExc_ValueError, "Could not import magnetic field.  Check 'ifile' and 'iformat' are correct");
    return NULL;
  }

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}








static PyObject* OSCARS_AddMagneticFieldFunction (OSCARSObject* self, PyObject* args)
{
  // Add a python function as a magnetic field object

  // Grab the function
  PyObject* Function;
  if (! PyArg_ParseTuple(args, "O:set_callback", &Function)) {
    return NULL;
  }

  // Increment ref to function for python
  Py_INCREF(Function);

  // Add the function as a field to the OSCARS object
  try {
    self->obj->AddMagneticField( (TField*) new TFieldPythonFunction(Function));
  } catch (std::invalid_argument e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return NULL;
  }

  // Decrement reference to function for python
  Py_DECREF(Function);

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}








static PyObject* OSCARS_AddMagneticFieldGaussian (OSCARSObject* self, PyObject* args, PyObject* keywds)
{
  // Add a magnetic field that is a gaussian

  // Lists and variables
  PyObject* List_BField       = PyList_New(0);
  PyObject* List_Translation  = PyList_New(0);
  PyObject* List_Rotations    = PyList_New(0);
  PyObject* List_Sigma        = PyList_New(0);

  TVector3D BField(0, 0, 0);
  TVector3D Sigma(0, 0, 0);
  TVector3D Rotations(0, 0, 0);
  TVector3D Translation(0, 0, 0);


  // Input variables and parsing
  static char *kwlist[] = {"bfield", "sigma", "rotations", "translation", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "OO|OO", kwlist,
                                                          &List_BField,
                                                          &List_Sigma,
                                                          &List_Rotations,
                                                          &List_Translation)) {
    return NULL;
  }



  // Check BField
  try {
    BField = OSCARS_ListAsTVector3D(List_BField);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, "Incorrect format in 'bfield'");
    return NULL;
  }

  // Check Width
  try {
    Sigma = OSCARS_ListAsTVector3D(List_Sigma);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, "Incorrect format in 'sigma'");
    return NULL;
  }

  // Check for Rotations in the input
  if (PyList_Size(List_Rotations) != 0) {
    try {
      Rotations = OSCARS_ListAsTVector3D(List_Rotations);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'rotations'");
      return NULL;
    }
  }

  // Check for Translation in the input
  if (PyList_Size(List_Translation) != 0) {
    try {
      Translation = OSCARS_ListAsTVector3D(List_Translation);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'translation'");
      return NULL;
    }
  }

  // Add field
  self->obj->AddMagneticField( (TField*) new TField3D_Gaussian(BField, Translation, Sigma, Rotations));

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}








static PyObject* OSCARS_AddMagneticFieldUniform (OSCARSObject* self, PyObject* args, PyObject* keywds)
{
  // Add a uniform field with a given width in a given direction, or for all space

  // Lists and vectors
  PyObject* List_BField      = PyList_New(0);
  PyObject* List_Translation = PyList_New(0);
  PyObject* List_Rotations   = PyList_New(0);
  PyObject* List_Width       = PyList_New(0);

  TVector3D BField(0, 0, 0);
  TVector3D Width (0, 0, 0);
  TVector3D Rotations(0, 0, 0);
  TVector3D Translation(0, 0, 0);


  // Input variables and parsing
  static char *kwlist[] = {"bfield", "width", "rotations", "translation", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|OOO", kwlist,
                                                          &List_BField,
                                                          &List_Width,
                                                          &List_Rotations,
                                                          &List_Translation)) {
    return NULL;
  }


  // Check BField
  try {
    BField = OSCARS_ListAsTVector3D(List_BField);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, "Incorrect format in 'bfield'");
    return NULL;
  }

  // Check Width
  if (PyList_Size(List_Width) != 0) {
    try {
      Width = OSCARS_ListAsTVector3D(List_Width);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'width'");
      return NULL;
    }
  }

  // Check for Rotations in the input
  if (PyList_Size(List_Rotations) != 0) {
    try {
      Rotations = OSCARS_ListAsTVector3D(List_Rotations);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'rotations'");
      return NULL;
    }
  }

  // Check for Translation in the input
  if (PyList_Size(List_Translation) != 0) {
    try {
      Translation = OSCARS_ListAsTVector3D(List_Translation);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'translation'");
      return NULL;
    }
  }

  // Add the field
  self->obj->AddMagneticField((TField*) new TField3D_UniformBox(BField, Width, Translation, Rotations));

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}





static PyObject* OSCARS_ClearMagneticFields (OSCARSObject* self)
{
  // Clear all magnetic fields in the OSCARS object
  self->obj->ClearMagneticFields();

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}










static PyObject* OSCARS_AddMagneticFieldIdealUndulator (OSCARSObject* self, PyObject* args, PyObject* keywds)
{
  // Add a magnetic field for undulator

  // Lists and variables
  PyObject* List_Field      = PyList_New(0);
  PyObject* List_Period      = PyList_New(0);
  PyObject* List_Rotations   = PyList_New(0);
  PyObject* List_Translation = PyList_New(0);
  int       NPeriods = 0;
  double    Phase = 0;

  TVector3D Field(0, 0, 0);
  TVector3D Period(0, 0, 0);
  TVector3D Rotations(0, 0, 0);
  TVector3D Translation(0, 0, 0);

  // Input variables and parsing
  static char *kwlist[] = {"bfield", "period", "nperiods", "phase", "rotations", "translation", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "OOi|dOO", kwlist,
                                                            &List_Field,
                                                            &List_Period,
                                                            &NPeriods,
                                                            &Phase,
                                                            &List_Rotations,
                                                            &List_Translation)) {
    return NULL;
  }


  // Check Field
  try {
    Field = OSCARS_ListAsTVector3D(List_Field);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, "Incorrect format in 'bfield'");
    return NULL;
  }

  // Check Period
  try {
    Period = OSCARS_ListAsTVector3D(List_Period);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, "Incorrect format in 'period'");
    return NULL;
  }

  // Check for Rotations in the input
  if (PyList_Size(List_Rotations) != 0) {
    try {
      Rotations = OSCARS_ListAsTVector3D(List_Rotations);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'rotations'");
      return NULL;
    }
  }

  // Check for Translation in the input
  if (PyList_Size(List_Translation) != 0) {
    try {
      Translation = OSCARS_ListAsTVector3D(List_Translation);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'translation'");
      return NULL;
    }
  }


  // Rotate field and sigma
  // UPDATE: check this
  Field.RotateSelfXYZ(Rotations);
  Period.RotateSelfXYZ(Rotations);


  // Add field
  self->obj->AddMagneticField( (TField*) new TField3D_IdealUndulator(Field, Period, NPeriods, Translation, Phase));

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}








static PyObject* OSCARS_GetBField (OSCARSObject* self, PyObject* args)
{
  // Get the magnetic field at a point as a 3D list [Bx, By, Bz]

  // Python list object
  PyObject * List;

  // Grab the python list from the input
  if (! PyArg_ParseTuple( args, "O!", &PyList_Type, &List)) {
    return NULL;
  }

  // Has to have the correct number of arguments
  if (PyList_Size(List) != 3) {
    return NULL;
  }

  // Grab the values
  TVector3D X;
  try {
    X = OSCARS_ListAsTVector3D(List);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, "Incorrect format in 'rotations'");
    return NULL;
  }

  // Set the object variable
  TVector3D const B = self->obj->GetB(X);

  // Create a python list
  PyObject *PList = OSCARS_TVector3DAsList(B);

  // Return the python list
  return PList;
}














static PyObject* OSCARS_AddElectricField (OSCARSObject* self, PyObject* args, PyObject* keywds)
{
  // Add a magnetic field from a file.
  // UPDATE: add binary file reading


  // Grab the values
  char const* FileName = "";
  char const* FileFormat = "";
  PyObject*   List_Rotations   = PyList_New(0);
  PyObject*   List_Translation = PyList_New(0);
  PyObject*   List_Scaling     = PyList_New(0);

  TVector3D Rotations(0, 0, 0);
  TVector3D Translation(0, 0, 0);
  std::vector<double> Scaling;


  // Input variables and parsing
  static char *kwlist[] = {"ifile", "iformat", "rotations", "translation", "scale", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss|OOO", kwlist,
                                                           &FileName,
                                                           &FileFormat,
                                                           &List_Rotations,
                                                           &List_Translation,
                                                           &List_Scaling)) {
    return NULL;
  }

  // Check that filename and format exist
  if (std::strlen(FileName) == 0 || std::strlen(FileFormat) == 0) {
    PyErr_SetString(PyExc_ValueError, "'ifile' or 'iformat' is blank");
    return NULL;
  }

  // Check for Rotations in the input
  if (PyList_Size(List_Rotations) != 0) {
    try {
      Rotations = OSCARS_ListAsTVector3D(List_Rotations);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'rotations'");
      return NULL;
    }
  }


  // Check for Translation in the input
  if (PyList_Size(List_Translation) != 0) {
    try {
      Translation = OSCARS_ListAsTVector3D(List_Translation);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'translation'");
      return NULL;
    }
  }

  // Get any scaling factors
  // UPDATE: Check against fileformat number of strings
  for (size_t i = 0; i < PyList_Size(List_Scaling); ++i) {
    Scaling.push_back(PyFloat_AsDouble(PyList_GetItem(List_Scaling, i)));
  }

  // Add the magnetic field to the OSCARS object
  try {
    self->obj->AddElectricField(FileName, FileFormat, Rotations, Translation, Scaling);
  } catch (...) {
    PyErr_SetString(PyExc_ValueError, "Could not import electric field.  Check 'ifile' and 'iformat' are correct");
    return NULL;
  }

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}








static PyObject* OSCARS_AddElectricFieldFunction (OSCARSObject* self, PyObject* args)
{
  // Add a python function as an electric field object

  // Grab the function
  PyObject* Function;
  if (! PyArg_ParseTuple(args, "O:set_callback", &Function)) {
    return NULL;
  }

  // Increment ref to function for python
  Py_INCREF(Function);

  // Add the function as a field to the OSCARS object
  try {
    self->obj->AddElectricField( (TField*) new TFieldPythonFunction(Function));
  } catch (std::invalid_argument e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return NULL;
  }

  // Decrement reference to function for python
  Py_DECREF(Function);

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}








static PyObject* OSCARS_AddElectricFieldGaussian (OSCARSObject* self, PyObject* args, PyObject* keywds)
{
  // Add an electric field that is a gaussian

  // Lists and variables
  PyObject* List_Field       = PyList_New(0);
  PyObject* List_Translation  = PyList_New(0);
  PyObject* List_Rotations    = PyList_New(0);
  PyObject* List_Sigma        = PyList_New(0);

  TVector3D Field(0, 0, 0);
  TVector3D Sigma(0, 0, 0);
  TVector3D Rotations(0, 0, 0);
  TVector3D Translation(0, 0, 0);


  // Input variables and parsing
  static char *kwlist[] = {"efield", "sigma", "rotations", "translation", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "OO|OO", kwlist,
                                                          &List_Field,
                                                          &List_Sigma,
                                                          &List_Rotations,
                                                          &List_Translation)) {
    return NULL;
  }



  // Check Field
  try {
    Field = OSCARS_ListAsTVector3D(List_Field);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, "Incorrect format in 'efield'");
    return NULL;
  }

  // Check Width
  try {
    Sigma = OSCARS_ListAsTVector3D(List_Sigma);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, "Incorrect format in 'sigma'");
    return NULL;
  }

  // Check for Rotations in the input
  if (PyList_Size(List_Rotations) != 0) {
    try {
      Rotations = OSCARS_ListAsTVector3D(List_Rotations);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'rotations'");
      return NULL;
    }
  }

  // Check for Translation in the input
  if (PyList_Size(List_Translation) != 0) {
    try {
      Translation = OSCARS_ListAsTVector3D(List_Translation);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'translation'");
      return NULL;
    }
  }

  // Add field
  self->obj->AddElectricField( (TField*) new TField3D_Gaussian(Field, Translation, Sigma, Rotations));

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}








static PyObject* OSCARS_AddElectricFieldUniform (OSCARSObject* self, PyObject* args, PyObject* keywds)
{
  // Add a uniform field with a given width in a given direction, or for all space

  // Lists and vectors
  PyObject* List_Field       = PyList_New(0);
  PyObject* List_Translation = PyList_New(0);
  PyObject* List_Rotations   = PyList_New(0);
  PyObject* List_Width       = PyList_New(0);

  TVector3D Field(0, 0, 0);
  TVector3D Width (0, 0, 0);
  TVector3D Rotations(0, 0, 0);
  TVector3D Translation(0, 0, 0);


  // Input variables and parsing
  static char *kwlist[] = {"efield", "width", "rotations", "translation", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|OOO", kwlist,
                                                          &List_Field,
                                                          &List_Width,
                                                          &List_Rotations,
                                                          &List_Translation)) {
    return NULL;
  }


  // Check Field
  try {
    Field = OSCARS_ListAsTVector3D(List_Field);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, "Incorrect format in 'efield'");
    return NULL;
  }

  // Check Width
  if (PyList_Size(List_Width) != 0) {
    try {
      Width = OSCARS_ListAsTVector3D(List_Width);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'width'");
      return NULL;
    }
  }

  // Check for Rotations in the input
  if (PyList_Size(List_Rotations) != 0) {
    try {
      Rotations = OSCARS_ListAsTVector3D(List_Rotations);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'rotations'");
      return NULL;
    }
  }

  // Check for Translation in the input
  if (PyList_Size(List_Translation) != 0) {
    try {
      Translation = OSCARS_ListAsTVector3D(List_Translation);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'translation'");
      return NULL;
    }
  }

  // Add the field
  self->obj->AddElectricField((TField*) new TField3D_UniformBox(Field, Width, Translation, Rotations));

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}











static PyObject* OSCARS_AddElectricFieldIdealUndulator (OSCARSObject* self, PyObject* args, PyObject* keywds)
{
  // Add an electric field undulator to OSCARS

  // Lists and variables
  PyObject* List_Field       = PyList_New(0);
  PyObject* List_Period      = PyList_New(0);
  PyObject* List_Rotations   = PyList_New(0);
  PyObject* List_Translation = PyList_New(0);
  int       NPeriods = 0;
  double    Phase = 0;

  TVector3D Field(0, 0, 0);
  TVector3D Period(0, 0, 0);
  TVector3D Rotations(0, 0, 0);
  TVector3D Translation(0, 0, 0);

  // Input variables and parsing
  static char *kwlist[] = {"efield", "period", "nperiods", "phase", "rotations", "translation", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "OOi|dOO", kwlist,
                                                            &List_Field,
                                                            &List_Period,
                                                            &NPeriods,
                                                            &Phase,
                                                            &List_Rotations,
                                                            &List_Translation)) {
    return NULL;
  }


  // Check Field
  try {
    Field = OSCARS_ListAsTVector3D(List_Field);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, "Incorrect format in 'efield'");
    return NULL;
  }

  // Check Period
  try {
    Period = OSCARS_ListAsTVector3D(List_Period);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, "Incorrect format in 'period'");
    return NULL;
  }

  // Check for Rotations in the input
  if (PyList_Size(List_Rotations) != 0) {
    try {
      Rotations = OSCARS_ListAsTVector3D(List_Rotations);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'rotations'");
      return NULL;
    }
  }

  // Check for Translation in the input
  if (PyList_Size(List_Translation) != 0) {
    try {
      Translation = OSCARS_ListAsTVector3D(List_Translation);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'translation'");
      return NULL;
    }
  }


  // Rotate field and sigma
  // UPDATE: check this
  Field.RotateSelfXYZ(Rotations);
  Period.RotateSelfXYZ(Rotations);


  // Add field
  self->obj->AddElectricField( (TField*) new TField3D_IdealUndulator(Field, Period, NPeriods, Translation, Phase));

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}








static PyObject* OSCARS_GetEField (OSCARSObject* self, PyObject* args)
{
  // Get the magnetic field at a point as a 3D list [Ex, Ey, Ez]

  // Python list object
  PyObject * List;

  // Grab the python list from the input
  if (! PyArg_ParseTuple( args, "O!", &PyList_Type, &List)) {
    return NULL;
  }

  // Has to have the correct number of arguments
  if (PyList_Size(List) != 3) {
    return NULL;
  }

  // Grab the values
  TVector3D X;
  try {
    X = OSCARS_ListAsTVector3D(List);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, "Incorrect format in input");
    return NULL;
  }

  // Set the object variable
  TVector3D const F = self->obj->GetE(X);

  // Create a python list
  PyObject *PList = OSCARS_TVector3DAsList(F);

  // Return the python list
  return PList;
}



static PyObject* OSCARS_ClearElectricFields (OSCARSObject* self)
{
  // Clear all magnetic fields in the OSCARS object
  self->obj->ClearElectricFields();

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}















static PyObject* OSCARS_AddFieldGaussian (OSCARSObject* self, PyObject* args, PyObject* keywds)
{
  // Add a magnetic field that is a gaussian

  // Lists and variables
  PyObject* List_BField       = PyList_New(0);
  PyObject* List_EField       = PyList_New(0);
  PyObject* List_Translation  = PyList_New(0);
  PyObject* List_Rotations    = PyList_New(0);
  PyObject* List_Sigma        = PyList_New(0);

  TVector3D Field(0, 0, 0);
  TVector3D Sigma(0, 0, 0);
  TVector3D Rotations(0, 0, 0);
  TVector3D Translation(0, 0, 0);


  // Input variables and parsing
  static char *kwlist[] = {"sigma", "bfield", "efield", "rotations", "translation", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|OOOO", kwlist,
                                                          &List_Sigma,
                                                          &List_BField,
                                                          &List_EField,
                                                          &List_Rotations,
                                                          &List_Translation)) {
    return NULL;
  }

  bool const HasBField = PyList_Size(List_BField) > 0;
  bool const HasEField = PyList_Size(List_EField) > 0;

  // Check that you don't define an e and a b field
  if (HasBField && HasEField) {
    PyErr_SetString(PyExc_ValueError, "You cannot define both 'bfield' and 'efield'");
    return NULL;
  }

  // Check Field
  if (HasBField ) {
    try {
      Field = OSCARS_ListAsTVector3D(List_BField);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'bfield'");
      return NULL;
    }
  } else if (HasEField) {
    try {
      Field = OSCARS_ListAsTVector3D(List_EField);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'efield'");
      return NULL;
    }
  } else {
    PyErr_SetString(PyExc_ValueError, "You must define either 'bfield' or 'efield'");
    return NULL;
  }

  // Check Width
  try {
    Sigma = OSCARS_ListAsTVector3D(List_Sigma);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, "Incorrect format in 'sigma'");
    return NULL;
  }

  // Check for Rotations in the input
  if (PyList_Size(List_Rotations) != 0) {
    try {
      Rotations = OSCARS_ListAsTVector3D(List_Rotations);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'rotations'");
      return NULL;
    }
  }

  // Check for Translation in the input
  if (PyList_Size(List_Translation) != 0) {
    try {
      Translation = OSCARS_ListAsTVector3D(List_Translation);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'translation'");
      return NULL;
    }
  }

  // Add field
  if (HasBField) {
    //self->obj->AddMagneticField( (TField*) new TField3D_Gaussian(Field, Translation, Sigma, Rotations));
  } else {
    self->obj->AddElectricField( (TField*) new TField3D_Gaussian(Field, Translation, Sigma, Rotations));
  }

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}





static PyObject* OSCARS_WriteMagneticField (OSCARSObject* self, PyObject* args, PyObject* keywds)
{
  // Add a magnetic field that is a gaussian

  const char* OutFileName = "";
  const char* OutFormat   = "";
  const char* Comment     = "";

  // Lists and variables
  PyObject* List_XLim = PyList_New(0);
  PyObject* List_YLim = PyList_New(0);
  PyObject* List_ZLim = PyList_New(0);

  int NX = 0;
  int NY = 0;
  int NZ = 0;

  TVector2D XLim;
  TVector2D YLim;
  TVector2D ZLim;


  // Input variables and parsing
  static char *kwlist[] = {"ofile", "oformat", "xlim", "nx", "ylim", "ny", "zlim", "nz", "comment", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss|OiOiOis", kwlist,
                                                               &OutFileName,
                                                               &OutFormat,
                                                               &List_XLim,
                                                               &NX,
                                                               &List_YLim,
                                                               &NY,
                                                               &List_ZLim,
                                                               &NZ,
                                                               &Comment)) {
    return NULL;
  }


  // Check that filename and format exist
  if (std::strlen(OutFileName) == 0 || std::strlen(OutFormat) == 0) {
    PyErr_SetString(PyExc_ValueError, "'ofile' or 'oformat' is blank");
    return NULL;
  }

  // Check for XLim in the input
  if (PyList_Size(List_XLim) != 0) {
    try {
      XLim = OSCARS_ListAsTVector2D(List_XLim);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'xlim'");
      return NULL;
    }
  }

  // Check for YLim in the input
  if (PyList_Size(List_YLim) != 0) {
    try {
      YLim = OSCARS_ListAsTVector2D(List_YLim);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'ylim'");
      return NULL;
    }
  }

  // Check for ZLim in the input
  if (PyList_Size(List_ZLim) != 0) {
    try {
      ZLim = OSCARS_ListAsTVector2D(List_ZLim);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'zlim'");
      return NULL;
    }
  }


  try {
    self->obj->WriteField("B", OutFileName, OutFormat, XLim, NX, YLim, NY, ZLim, NZ, Comment);
  } catch (...) {
    PyErr_SetString(PyExc_ValueError, "could not write output file");
    return NULL;
  }

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}





static PyObject* OSCARS_WriteElectricField (OSCARSObject* self, PyObject* args, PyObject* keywds)
{
  // Add a magnetic field that is a gaussian

  const char* OutFileName = "";
  const char* OutFormat   = "";
  const char* Comment     = "";

  // Lists and variables
  PyObject* List_XLim = PyList_New(0);
  PyObject* List_YLim = PyList_New(0);
  PyObject* List_ZLim = PyList_New(0);

  int NX = 0;
  int NY = 0;
  int NZ = 0;

  TVector2D XLim;
  TVector2D YLim;
  TVector2D ZLim;


  // Input variables and parsing
  static char *kwlist[] = {"ofile", "oformat", "xlim", "nx", "ylim", "ny", "zlim", "nz", "comment", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss|OiOiOis", kwlist,
                                                               &OutFileName,
                                                               &OutFormat,
                                                               &List_XLim,
                                                               &NX,
                                                               &List_YLim,
                                                               &NY,
                                                               &List_ZLim,
                                                               &NZ,
                                                               &Comment)) {
    return NULL;
  }


  // Check that filename and format exist
  if (std::strlen(OutFileName) == 0 || std::strlen(OutFormat) == 0) {
    PyErr_SetString(PyExc_ValueError, "'ofile' or 'oformat' is blank");
    return NULL;
  }

  // Check for XLim in the input
  if (PyList_Size(List_XLim) != 0) {
    try {
      XLim = OSCARS_ListAsTVector2D(List_XLim);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'xlim'");
      return NULL;
    }
  }

  // Check for YLim in the input
  if (PyList_Size(List_YLim) != 0) {
    try {
      YLim = OSCARS_ListAsTVector2D(List_YLim);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'ylim'");
      return NULL;
    }
  }

  // Check for ZLim in the input
  if (PyList_Size(List_ZLim) != 0) {
    try {
      ZLim = OSCARS_ListAsTVector2D(List_ZLim);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'zlim'");
      return NULL;
    }
  }


  try {
    self->obj->WriteField("E", OutFileName, OutFormat, XLim, NX, YLim, NY, ZLim, NZ, Comment);
  } catch (...) {
    PyErr_SetString(PyExc_ValueError, "could not write output file");
    return NULL;
  }

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}

























































static PyObject* OSCARS_SetParticleBeam (OSCARSObject* self, PyObject* args, PyObject* keywds)
{
  // Clear all particle beams, add this beam, and set a new particle

  self->obj->ClearParticleBeams();

  PyObject* ret = OSCARS_AddParticleBeam(self, args, keywds);
  if (ret == NULL) {
    return ret;
  }

  self->obj->SetNewParticle("", "ideal");

  return ret;
}





static PyObject* OSCARS_AddParticleBeam (OSCARSObject* self, PyObject* args, PyObject* keywds)
{
  // Add a particle beam to the experiment

  // Lists and variables some with initial values
  char const* Type                       = "";
  char const* Name                       = "";
  double      Energy_GeV                 = 0;
  double      Sigma_Energy_GeV           = 0;
  double      T0                         = 0;
  double      Current                    = 0;
  double      Weight                     = 1;
  double      Mass                       = 0;
  double      Charge                     = 0;
  PyObject*   List_Position       = PyList_New(0);
  PyObject*   List_Direction             = PyList_New(0);
  PyObject*   List_Rotations             = PyList_New(0);
  PyObject*   List_Translation           = PyList_New(0);
  PyObject*   List_Horizontal_Direction  = PyList_New(0);
  PyObject*   List_Beta                  = PyList_New(0);
  PyObject*   List_Emittance             = PyList_New(0);
  PyObject*   List_Lattice_Reference     = PyList_New(0);

  TVector3D Position(0, 0, 0);
  TVector3D Direction;
  TVector3D Rotations(0, 0, 0);
  TVector3D Translation(0, 0, 0);
  TVector3D Horizontal_Direction;
  TVector2D Beta(0, 0);
  TVector2D Emittance(0, 0);
  TVector3D Lattice_Reference(0, 0, 0);


  // Input variables and parsing
  static char *kwlist[] = {"type", "name", "energy_GeV", "d0", "x0", "sigma_energy_GeV", "t0", "current", "weight", "rotations", "translation", "horizontal_direction", "beta", "emittance", "lattice_reference", "mass", "charge", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "ssdOO|ddddOOOOOOdd", kwlist,
                                                                       &Type,
                                                                       &Name,
                                                                       &Energy_GeV,
                                                                       &List_Direction,
                                                                       &List_Position,
                                                                       &Sigma_Energy_GeV,
                                                                       &T0,
                                                                       &Current,
                                                                       &Weight,
                                                                       &List_Rotations,
                                                                       &List_Translation,
                                                                       &List_Horizontal_Direction,
                                                                       &List_Beta,
                                                                       &List_Emittance,
                                                                       &List_Lattice_Reference,
                                                                       &Mass,
                                                                       &Charge)) {
    return NULL;
  }


  // Check that type and name exist
  if (std::strlen(Type) == 0 || std::strlen(Name) == 0) {
    PyErr_SetString(PyExc_ValueError, "'type' or 'name' is blank");
    return NULL;
  }

  // Initial position
  if (PyList_Size(List_Position) != 0) {
    try {
      Position = OSCARS_ListAsTVector3D(List_Position);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'x0'");
      return NULL;
    }
  }

  // Initial direction
  if (PyList_Size(List_Direction) != 0) {
    try {
      Direction = OSCARS_ListAsTVector3D(List_Direction);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'd0'");
      return NULL;
    }
  }

  if (Sigma_Energy_GeV == 0) {
    // Do nothing.  zero energy diff is alright
  } else if (Sigma_Energy_GeV < 0) {
    PyErr_SetString(PyExc_ValueError, "'sigma_energy_GeV' cannot be less than zero");
    return NULL;
  }

  // Check for Rotations in the input
  if (PyList_Size(List_Rotations) != 0) {
    try {
      Rotations = OSCARS_ListAsTVector3D(List_Rotations);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'rotations'");
      return NULL;
    }
  }


  // Check for Translation in the input
  if (PyList_Size(List_Translation) != 0) {
    try {
      Translation = OSCARS_ListAsTVector3D(List_Translation);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'translation'");
      return NULL;
    }
  }



  // Check for Horizontal_Direction in the input
  if (PyList_Size(List_Horizontal_Direction) != 0) {
    try {
      Horizontal_Direction = OSCARS_ListAsTVector3D(List_Horizontal_Direction);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'horizontal_direction'");
      return NULL;
    }
  } else {
    Horizontal_Direction = Direction.Orthogonal().UnitVector();
  }
  Horizontal_Direction = Horizontal_Direction.UnitVector();


  // Check for Beta in the input
  if (PyList_Size(List_Beta) != 0) {
    try {
      Beta = OSCARS_ListAsTVector2D(List_Beta);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'beta'");
      return NULL;
    }
  }


  // Check for Emittance in the input
  if (PyList_Size(List_Emittance) != 0) {
    try {
      Emittance = OSCARS_ListAsTVector2D(List_Emittance);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'emittance'");
      return NULL;
    }
  }


  // Check for Lattice reference in the input
  if (PyList_Size(List_Lattice_Reference) != 0) {
    try {
      Lattice_Reference = OSCARS_ListAsTVector3D(List_Lattice_Reference);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'lattice_reference'");
      return NULL;
    }
  }


  // If emittance and beta are defined get RMS values
  TVector2D SigmaU(0, 0);
  TVector2D SigmaUPrime(0, 0);
  if (PyList_Size(List_Emittance) != 0 && PyList_Size(List_Beta) != 0) {
    SigmaU[0]      = sqrt(Emittance[0] * Beta[0]);
    SigmaU[1]      = sqrt(Emittance[1] * Beta[1]);
    SigmaUPrime[0] = sqrt(Emittance[0] / Beta[0]);
    SigmaUPrime[1] = sqrt(Emittance[1] / Beta[1]);
  }




  // Rotate beam parameters
  Position.RotateSelfXYZ(Rotations);
  Position += Translation;

  // UPDATE: An idea for later, use new variable "velocity"
  //if (Energy_GeV == 0) {
  //  if (Direction.Mag() >= 1) {
  //    throw;
  //  }
  //  Energy_GeV = sqrt(1.0 / (1.0 - Direction.Mag2())) * Mass;
  //}


  // Add the particle beam
  try {
    if (std::string(Type) == "custom") {
      if (Mass == 0 || Charge == 0) {
        PyErr_SetString(PyExc_ValueError, "'mass' or 'charge' is zero");
        return NULL;
      }
      // UPDATE: for custom beams
      self->obj->AddParticleBeam(Type, Name, Position, Direction, Energy_GeV, T0, Current, Weight, Charge, Mass);
    } else {
      self->obj->AddParticleBeam(Type, Name, Position, Direction, Energy_GeV, T0, Current, Weight);
    }
  } catch (std::invalid_argument e) {
    PyErr_SetString(PyExc_ValueError, "invalid argument in adding particle beam.  possibly 'name' already exists");
    return NULL;
  }

  // UPDATE: Change me to include sigma of beam
  self->obj->GetParticleBeam(Name).SetSigma(Horizontal_Direction, SigmaU, SigmaUPrime, Lattice_Reference, Sigma_Energy_GeV);

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}




















static PyObject* OSCARS_SetNewParticle (OSCARSObject* self, PyObject* args, PyObject* keywds)
{
  // Set a new particle within the OSCARS object

  char const* Beam_IN = "";
  char const* Particle_IN = "";

  static char *kwlist[] = {"beam", "particle", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "|ss", kwlist, &Beam_IN, &Particle_IN)) {
    return NULL;
  }

  // Check if a beam is at least defined
  if (self->obj->GetNParticleBeams() < 1) {
    PyErr_SetString(PyExc_ValueError, "No particle beam defined");
    return NULL;
  }

  std::string Beam = Beam_IN;
  std::string Particle = Particle_IN;

  std::transform(Beam.begin(), Beam.end(), Beam.begin(), ::tolower);
  std::transform(Particle.begin(), Particle.end(), Particle.begin(), ::tolower);

  if (Particle != "" && !(Particle == "ideal" || Particle == "random")) {
    PyErr_SetString(PyExc_ValueError, "'particle' must be 'ideal' or 'random'");
    return NULL;
  }

  try {
    if ((Beam) == "" && (Particle) == "") {
      self->obj->SetNewParticle();
    } else if (Beam != "" && Particle != "") {
      self->obj->SetNewParticle(Beam, Particle);
    } else if (Beam == "" && Particle != "") {
      self->obj->SetNewParticle(Beam, Particle);
    } else if (Beam != "" && Particle == "") {
      self->obj->SetNewParticle(Beam, Particle);
    }
  } catch (std::out_of_range e) {
    PyErr_SetString(PyExc_ValueError, "'beam' name not found");
    return NULL;
  }

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}






static PyObject* OSCARS_GetParticleX0 (OSCARSObject* self)
{
  // Get the particle position at particle t0
  return OSCARS_TVector3DAsList( self->obj->GetCurrentParticle().GetX0() );
}




static PyObject* OSCARS_GetParticleBeta0 (OSCARSObject* self)
{
  // Get the particle beta at particle t0
  return OSCARS_TVector3DAsList( self->obj->GetCurrentParticle().GetB0() );
}




static PyObject* OSCARS_GetParticleE0 (OSCARSObject* self)
{
  // Get the particle beta at particle t0
  return Py_BuildValue("f", (self->obj->GetCurrentParticle().GetE0()));
}




static PyObject* OSCARS_ClearParticleBeams (OSCARSObject* self)
{
  // Clear the contents of the particle beam container in OSCARS

  self->obj->ClearParticleBeams();

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}






static PyObject* OSCARS_CalculateTrajectory (OSCARSObject* self)
{
  // Get the CTStop variable from OSCARS

  try {
    self->obj->CalculateTrajectory();
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return NULL;
  } catch (std::out_of_range e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return NULL;
  }

  // Return the trajectory
  return OSCARS_GetTrajectory(self);
}






static PyObject* OSCARS_GetTrajectory (OSCARSObject* self)
{
  // Get the Trajectory as 2 3D lists [[x, y, z], [BetaX, BetaY, BetaZ]]

  // Create a python list
  PyObject *PList = PyList_New(0);

  // Grab trajectory
  TParticleTrajectoryPoints const& T = self->obj->GetTrajectory();

  // Number of points in trajectory calculation
  size_t NTPoints = T.GetNPoints();

  // Loop over all points in trajectory
  for (int iT = 0; iT != NTPoints; ++iT) {
    // Create a python list for X and Beta
    PyObject *PList2 = PyList_New(0);

    // Add position and Beta to list
    PyList_Append(PList2, OSCARS_TVector3DAsList(T.GetX(iT)));
    PyList_Append(PList2, OSCARS_TVector3DAsList(T.GetB(iT)));
    PyList_Append(PList, PList2);
  }

  // Return the python list
  return PList;
}












static PyObject* OSCARS_GetSpectrumAsList (OSCARSObject* self, TSpectrumContainer const& Spectrum)
{
  // Get the spectrum as a list format for python output

  // Create a python list
  PyObject *PList = PyList_New(0);

  // Number of points in trajectory calculation
  size_t NSPoints = Spectrum.GetNPoints();

  // Loop over all points in trajectory
  for (int iS = 0; iS != NSPoints; ++iS) {
    // Create a python list for X and Beta
    PyObject *PList2 = PyList_New(0);

    // Add position and Beta to list
    PyList_Append(PList2, Py_BuildValue("f", Spectrum.GetEnergy(iS)));
    PyList_Append(PList2, Py_BuildValue("f", Spectrum.GetFlux(iS)));
    PyList_Append(PList, PList2);
  }

  // Return the python list
  return PList;
}






static PyObject* OSCARS_GetT3DScalarAsList (OSCARSObject* self, T3DScalarContainer const& C)
{
  // Get the spectrum as a list format for python output

  // Create a python list
  PyObject *PList = PyList_New(0);

  // Number of points in trajectory calculation
  size_t NPoints = C.GetNPoints();

  // Loop over all points
  for (int i = 0; i != NPoints; ++i) {
    // Create a python list
    PyObject *PList2 = PyList_New(0);

    PyObject *X = OSCARS_TVector3DAsList(C.GetPoint(i).GetX());
    double const V = C.GetPoint(i).GetV();

    // Add position and Beta to list
    PyList_Append(PList2, X);
    PyList_Append(PList2, Py_BuildValue("f", V));
    PyList_Append(PList, PList2);
  }

  // Return the python list
  return PList;
}






TSpectrumContainer OSCARS_GetSpectrumFromList (PyObject* List)
{
  // Take an input list in spectrum format and convert it to TSpectrumContainer object

  // Increment reference for list
  Py_INCREF(List);

  // Get size of input list
  size_t const NPoints = PyList_Size(List);
  if (NPoints <= 0) {
    throw;
  }

  TSpectrumContainer S;

  for (size_t ip = 0; ip != NPoints; ++ip) {
    PyObject* List_Point = PyList_GetItem(List, ip);
    if (PyList_Size(List_Point) == 2) {
      S.AddPoint(PyFloat_AsDouble(PyList_GetItem(List_Point, 0)), PyFloat_AsDouble(PyList_GetItem(List_Point, 1)));
    } else {
      throw;
    }
  }


  // Increment reference for list
  Py_DECREF(List);

  // Return the object
  return S;
}







T3DScalarContainer OSCARS_GetT3DScalarContainerFromList (PyObject* List)
{
  // Take an input list and convert it to T3DScalarContainer object

  // Increment reference for list
  Py_INCREF(List);

  // Get size of input list
  size_t const NPoints = PyList_Size(List);
  if (NPoints <= 0) {
    throw;
  }

  T3DScalarContainer F;

  for (int ip = 0; ip != NPoints; ++ip) {
    PyObject* List_Point = PyList_GetItem(List, ip);
    if (PyList_Size(List_Point) == 2) {
      F.AddPoint(OSCARS_ListAsTVector3D(PyList_GetItem(List_Point, 0)), PyFloat_AsDouble(PyList_GetItem(List_Point, 1)));
    } else {
      throw;
    }
  }


  // Increment reference for list
  Py_DECREF(List);

  // Return the object
  return F;
}

































static PyObject* OSCARS_CalculateSpectrum (OSCARSObject* self, PyObject* args, PyObject* keywds)
{
  // Calculate the spectrum given an observation point, and energy range


  PyObject* List_Obs = PyList_New(0);
  int NPoints = 0;
  PyObject* List_EnergyRange_eV = PyList_New(0);
  PyObject* List_Points_eV      = PyList_New(0);
  int NParticles = 0;
  int NThreads = 0;
  int GPU = 0;
  char* OutFileName = "";
  char* OutFileNameBinary = "";



  static char *kwlist[] = {"obs", "npoints", "energy_range_eV", "points_eV", "nparticles", "nthreads", "gpu", "ofile", "bofile", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|iOOiiiss", kwlist,
                                                               &List_Obs,
                                                               &NPoints,
                                                               &List_EnergyRange_eV,
                                                               &List_Points_eV,
                                                               &NParticles,
                                                               &NThreads,
                                                               &GPU,
                                                               &OutFileName,
                                                               &OutFileNameBinary)) {
    return NULL;
  }

  // Check if a beam is at least defined
  //if (self->obj->GetNParticleBeams() < 1) {
  //  PyErr_SetString(PyExc_ValueError, "No particle beam defined");
  //  return NULL;
  //}


  // Check number of particles
  if (NParticles < 0) {
    PyErr_SetString(PyExc_ValueError, "'nparticles' must be >= 1 (sort of)");
    return NULL;
  }


  // Add all values to a vector
  std::vector<double> VPoints_eV;
  for (size_t i = 0; i < PyList_Size(List_Points_eV); ++i) {
    VPoints_eV.push_back(PyFloat_AsDouble(PyList_GetItem(List_Points_eV, i)));
  }

  double EStart;
  double EStop;

  if (PyList_Size(List_EnergyRange_eV) != 0) {
    if (PyList_Size(List_EnergyRange_eV) == 2) {
      EStart = PyFloat_AsDouble(PyList_GetItem(List_EnergyRange_eV, 0));
      EStop  = PyFloat_AsDouble(PyList_GetItem(List_EnergyRange_eV, 1));
    } else {
      PyErr_SetString(PyExc_ValueError, "'energy_range_eV' must be a list of length 2");
      return NULL;
    }
  }




  // Observation point
  TVector3D Obs(0, 0, 0);
  try {
    Obs = OSCARS_ListAsTVector3D(List_Obs);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, "Incorrect format in 'obs'");
    return NULL;
  }


  // Check GPU parameter
  if (GPU != 0 && GPU != 1) {
    PyErr_SetString(PyExc_ValueError, "'gpu' must be 0 or 1");
    return NULL;
  }


  // Check NThreads parameter
  if (NThreads < 0) {
    PyErr_SetString(PyExc_ValueError, "'nthreads' must be > 0");
    return NULL;
  }

  // Check you are not trying to use threads and GPU
  if (NThreads > 0 && GPU == 1) {
    PyErr_SetString(PyExc_ValueError, "gpu is 1 and nthreads > 0.  Both are not currently allowed.");
    return NULL;
  }

  // Container for spectrum
  TSpectrumContainer SpectrumContainer;

  if (VPoints_eV.size() == 0) {
    SpectrumContainer.Init(NPoints, EStart, EStop);
  } else {
    SpectrumContainer.Init(VPoints_eV);
  }

  // Actually calculate the spectrum
  try {
    self->obj->CalculateSpectrum(Obs, SpectrumContainer, NParticles, NThreads, GPU);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return NULL;
  } catch (std::out_of_range e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return NULL;
  }


  if (std::string(OutFileName) != "") {
    SpectrumContainer.WriteToFileText(OutFileName);
  }

  if (std::string(OutFileNameBinary) != "") {
    SpectrumContainer.WriteToFileBinary(OutFileNameBinary);
  }

  // Return the spectrum
  return OSCARS_GetSpectrumAsList(self, SpectrumContainer);
}

















static PyObject* OSCARS_CalculateTotalPower (OSCARSObject* self)
{
  // Calculate the total power radiated by the current particle

  double Power = 0;

  // Check if a beam is at least defined
  if (self->obj->GetNParticleBeams() < 1) {
    PyErr_SetString(PyExc_ValueError, "No particle beam defined");
    return NULL;
  }

  // Return the total power
  // UPDATE: This does not fail when no beam defined
  try {
    Power = self->obj->CalculateTotalPower();
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return NULL;
  } catch (std::out_of_range e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return NULL;
  }

  return Py_BuildValue("f", Power);
}








static PyObject* OSCARS_CalculatePowerDensity (OSCARSObject* self, PyObject* args, PyObject *keywds)
{
  // Calculate the power density given a list of points

  PyObject*   List_Translation = PyList_New(0);
  PyObject*   List_Rotations   = PyList_New(0);
  PyObject*   List_Points      = PyList_New(0);
  int         NormalDirection = 0;
  int const   Dim = 3;
  int         NParticles = 0;
  int         GPU = 0;
  int         NThreads = 0;
  char const* OutFileName = "";


  static char *kwlist[] = {"points", "normal", "rotations", "translation", "nparticles", "gpu", "nthreads", "ofile", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|iOOiiis", kwlist,
                                                              &List_Points,
                                                              &NormalDirection,
                                                              &List_Rotations,
                                                              &List_Translation,
                                                              &NParticles,
                                                              &GPU,
                                                              &NThreads,
                                                              &OutFileName)) {
    return NULL;
  }

  // Check if a beam is at least defined
  if (self->obj->GetNParticleBeams() < 1) {
    PyErr_SetString(PyExc_ValueError, "No particle beam defined");
    return NULL;
  }


  // Check requested dimension
  if (Dim != 2 & Dim != 3) {
    PyErr_SetString(PyExc_ValueError, "'dim' must be 2 or 3");
    return NULL;
  }


  // Vectors for rotations and translations.  Default to 0
  TVector3D Rotations(0, 0, 0);
  TVector3D Translation(0, 0, 0);


  // Check for Rotations in the input
  if (PyList_Size(List_Rotations) != 0) {
    try {
      Rotations = OSCARS_ListAsTVector3D(List_Rotations);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'rotations'");
      return NULL;
    }
  }


  // Check for Translation in the input
  if (PyList_Size(List_Translation) != 0) {
    try {
      Translation = OSCARS_ListAsTVector3D(List_Translation);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'translation'");
      return NULL;
    }
  }

  // Look for arbitrary shape 3D points
  TSurfacePoints_3D Surface;
  for (size_t i = 0; i < PyList_Size(List_Points); ++i) {
    PyObject* LXN = PyList_GetItem(List_Points, i);
    TVector3D X;
    TVector3D N;
    if (PyList_Size(LXN) == 2) {

      try {
        X = OSCARS_ListAsTVector3D(PyList_GetItem(LXN, 0));
        N = OSCARS_ListAsTVector3D(PyList_GetItem(LXN, 1));
      } catch (std::length_error e) {
        PyErr_SetString(PyExc_ValueError, "Incorrect format in 'points': Point or Normal does not have 3 elements");
        return NULL;
      }

      // Rotate point and normal
      X.RotateSelfXYZ(Rotations);
      N.RotateSelfXYZ(Rotations);

      // Translate point, normal does not get translated
      X += Translation;

      Surface.AddPoint(X, N);
    } else {
      // input format error
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'points'");
      return NULL;
    }
  }

  // Check number of particles
  if (NParticles < 0) {
    PyErr_SetString(PyExc_ValueError, "'nparticles' must be >= 1 (sort of)");
    return NULL;
  }


  // Check GPU parameter
  if (GPU != 0 && GPU != 1) {
    PyErr_SetString(PyExc_ValueError, "'gpu' must be 0 or 1");
    return NULL;
  }

  // Check NThreads parameter
  if (NThreads < 0) {
    PyErr_SetString(PyExc_ValueError, "'nthreads' must be > 0");
    return NULL;
  }

  // Check you are not trying to use threads and GPU
  if (NThreads > 0 && GPU == 1) {
    PyErr_SetString(PyExc_ValueError, "gpu is 1 and nthreads > 0.  Both are not currently allowed.");
    return NULL;
  }
    


  // Container for Point plus scalar
  T3DScalarContainer PowerDensityContainer;


  // Actually calculate the spectrum
  bool const Directional = NormalDirection == 0 ? false : true;

  try {
    self->obj->CalculatePowerDensity(Surface, PowerDensityContainer, Dim, Directional, NParticles, OutFileName, NThreads, GPU);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return NULL;
  } catch (std::out_of_range e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return NULL;
  }


  // Build the output list of: [[[x, y, z], PowerDensity], [...]]
  // Create a python list
  PyObject *PList = PyList_New(0);

  size_t const NPoints = PowerDensityContainer.GetNPoints();

  for (size_t i = 0; i != NPoints; ++i) {
    T3DScalar P = PowerDensityContainer.GetPoint(i);

    // Inner list for each point
    PyObject *PList2 = PyList_New(0);


    // Add position and value to list
    PyList_Append(PList2, OSCARS_TVector3DAsList(P.GetX()));
    PyList_Append(PList2, Py_BuildValue("f", P.GetV()));
    PyList_Append(PList, PList2);

  }

  return PList;
}









static PyObject* OSCARS_CalculatePowerDensityRectangle (OSCARSObject* self, PyObject* args, PyObject *keywds)
{
  // Calculate the spectrum given an observation point, and energy range

  char const* SurfacePlane = "";
  size_t      NX1 = 0;
  size_t      NX2 = 0;
  double      Width_X1 = 0;
  double      Width_X2 = 0;
  PyObject*   List_NPoints     = PyList_New(0);
  PyObject*   List_Width       = PyList_New(0);
  PyObject*   List_Translation = PyList_New(0);
  PyObject*   List_Rotations   = PyList_New(0);
  PyObject*   List_X0X1X2      = PyList_New(0);
  int         NormalDirection = 0;
  int         NParticles = 0;
  int         GPU = 0;
  int         NThreads = 0;
  int         Dim = 2;
  char*       OutFileName = "";


  static char *kwlist[] = {"npoints", "plane", "width", "x0x1x2", "rotations", "translation", "ofile", "normal", "nparticles", "gpu", "nthreads", "dim",  NULL};

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|sOOOOsiiiii", kwlist,
                                                                  &List_NPoints,
                                                                  &SurfacePlane,
                                                                  &List_Width,
                                                                  &List_X0X1X2,
                                                                  &List_Rotations,
                                                                  &List_Translation,
                                                                  &OutFileName,
                                                                  &NormalDirection,
                                                                  &NParticles,
                                                                  &GPU,
                                                                  &NThreads,
                                                                  &Dim)) {
    return NULL;
  }

  // Check if a beam is at least defined
  if (self->obj->GetNParticleBeams() < 1) {
    PyErr_SetString(PyExc_ValueError, "No particle beam defined");
    return NULL;
  }


  // Check requested dimension
  if (Dim != 2 & Dim != 3) {
    PyErr_SetString(PyExc_ValueError, "'dim' must be 2 or 3");
    return NULL;
  }


  // The rectangular surface object we'll use
  TSurfacePoints_Rectangle Surface;

  if (PyList_Size(List_NPoints) == 2) {
    // NPoints in [m]
    NX1 = PyInt_AsSsize_t(PyList_GetItem(List_NPoints, 0));
    NX2 = PyInt_AsSsize_t(PyList_GetItem(List_NPoints, 1));
  } else {
    PyErr_SetString(PyExc_ValueError, "'npoints' must be [int, int]");
    return NULL;
  }



  if (NX1 <= 0 || NX2 <= 0) {
    PyErr_SetString(PyExc_ValueError, "an entry in 'npoints' is <= 0");
    return NULL;
  }

  // Vectors for rotations and translations.  Default to 0
  TVector3D Rotations(0, 0, 0);
  TVector3D Translation(0, 0, 0);

  // Check for Rotations in the input
  if (PyList_Size(List_Rotations) != 0) {
    try {
      Rotations = OSCARS_ListAsTVector3D(List_Rotations);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'rotations'");
      return NULL;
    }
  }


  // Check for Translation in the input
  if (PyList_Size(List_Translation) != 0) {
    try {
      Translation = OSCARS_ListAsTVector3D(List_Translation);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'translation'");
      return NULL;
    }
  }

  if (PyList_Size(List_Width) == 2) {
    // Width in [m]
    Width_X1 = PyFloat_AsDouble(PyList_GetItem(List_Width, 0));
    Width_X2 = PyFloat_AsDouble(PyList_GetItem(List_Width, 1));
  }



  // If you are requesting a simple surface plane, check that you have widths
  if (std::strlen(SurfacePlane) != 0 && Width_X1 > 0 && Width_X2 > 0) {
    try {
      Surface.Init(SurfacePlane, (int) NX1, (int) NX2, Width_X1, Width_X2, Rotations, Translation, NormalDirection);
    } catch (std::invalid_argument e) {
      PyErr_SetString(PyExc_ValueError, e.what());
      return NULL;
    }
  }



  // If X0X1X2 defined
  std::vector<TVector3D> X0X1X2;

  if (PyList_Size(List_X0X1X2) != 0) {
    if (PyList_Size(List_X0X1X2) == 3) {
      for (int i = 0; i != 3; ++i) {
        PyObject* List_X = PyList_GetItem(List_X0X1X2, i);

        try {
          X0X1X2.push_back(OSCARS_ListAsTVector3D(List_X));
        } catch (std::length_error e) {
          PyErr_SetString(PyExc_ValueError, "Incorrect format in 'x0x1x2'");
          return NULL;
        }
      }
    } else {
      PyErr_SetString(PyExc_ValueError, "'x0x1x2' must have 3 XYZ points defined correctly");
      return NULL;
    }

    for (std::vector<TVector3D>::iterator it = X0X1X2.begin(); it != X0X1X2.end(); ++it) {
      it->RotateSelfXYZ(Rotations);
      *it += Translation;
    }

    // UPDATE: Check for orthogonality
    Surface.Init((int) NX1, (int) NX2, X0X1X2[0], X0X1X2[1], X0X1X2[2], NormalDirection);
  }


  // Check number of particles
  if (NParticles < 0) {
    PyErr_SetString(PyExc_ValueError, "'nparticles' must be >= 1 (sort of)");
    return NULL;
  }


  // Check GPU parameter
  if (GPU != 0 && GPU != 1) {
    PyErr_SetString(PyExc_ValueError, "'gpu' must be 0 or 1");
    return NULL;
  }

  // Check NThreads parameter
  if (NThreads < 0) {
    PyErr_SetString(PyExc_ValueError, "'nthreads' must be > 0");
    return NULL;
  }

  // Check you are not trying to use threads and GPU
  if (NThreads > 0 && GPU == 1) {
    PyErr_SetString(PyExc_ValueError, "gpu is 1 and nthreads > 0.  Both are not currently allowed.");
    return NULL;
  }
    





  // Container for Point plus scalar
  T3DScalarContainer PowerDensityContainer;


  // Actually calculate the spectrum
  bool const Directional = NormalDirection == 0 ? false : true;
  try {
    self->obj->CalculatePowerDensity(Surface, PowerDensityContainer, Dim, Directional, NParticles, OutFileName, NThreads, GPU);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return NULL;
  } catch (std::out_of_range e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return NULL;
  }



  // Build the output list of: [[[x, y, z], PowerDensity], [...]]
  // Create a python list
  PyObject *PList = PyList_New(0);

  size_t const NPoints = PowerDensityContainer.GetNPoints();

  for (size_t i = 0; i != NPoints; ++i) {
    T3DScalar P = PowerDensityContainer.GetPoint(i);

    // Inner list for each point
    PyObject *PList2 = PyList_New(0);


    // Add position and value to list
    PyList_Append(PList2, OSCARS_TVector3DAsList(P.GetX()));
    PyList_Append(PList2, Py_BuildValue("f", P.GetV()));
    PyList_Append(PList, PList2);

  }

  return PList;
}





















static PyObject* OSCARS_CalculateFlux (OSCARSObject* self, PyObject* args, PyObject *keywds)
{
  // Calculate the flux on a surface given an energy and list of points in 3D

  double      Energy_eV = 0;
  PyObject*   List_Translation = PyList_New(0);
  PyObject*   List_Rotations   = PyList_New(0);
  PyObject*   List_Points      = PyList_New(0);
  int         NormalDirection = 0;
  int         Dim = 3;
  int         NParticles = 0;
  int         GPU = 0;
  int         NThreads = 0;
  char const* OutFileName = "";


  static char *kwlist[] = {"energy_eV", "points", "normal", "rotations", "translation", "nparticles", "nthreads", "gpu", "ofile", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "dO|iOOiiis", kwlist,
                                                              &Energy_eV,
                                                              &List_Points,
                                                              &NormalDirection,
                                                              &List_Rotations,
                                                              &List_Translation,
                                                              &NParticles,
                                                              &NThreads,
                                                              &GPU,
                                                              &OutFileName)) {
    return NULL;
  }

  // Check if a beam is at least defined
  if (self->obj->GetNParticleBeams() < 1) {
    PyErr_SetString(PyExc_ValueError, "No particle beam defined");
    return NULL;
  }


  // Check requested dimension
  if (Dim != 2 & Dim != 3) {
    PyErr_SetString(PyExc_ValueError, "'dim' must be 2 or 3");
    return NULL;
  }


  // Vectors for rotations and translations.  Default to 0
  TVector3D Rotations(0, 0, 0);
  TVector3D Translation(0, 0, 0);


  // Check for Rotations in the input
  if (PyList_Size(List_Rotations) != 0) {
    try {
      Rotations = OSCARS_ListAsTVector3D(List_Rotations);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'rotations'");
      return NULL;
    }
  }


  // Check for Translation in the input
  if (PyList_Size(List_Translation) != 0) {
    try {
      Translation = OSCARS_ListAsTVector3D(List_Translation);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'translation'");
      return NULL;
    }
  }

  // Look for arbitrary shape 3D points
  TSurfacePoints_3D Surface;
  for (size_t i = 0; i < PyList_Size(List_Points); ++i) {
    PyObject* LXN = PyList_GetItem(List_Points, i);
    TVector3D X;
    TVector3D N;
    if (PyList_Size(LXN) == 2) {

      try {
        X = OSCARS_ListAsTVector3D(PyList_GetItem(LXN, 0));
        N = OSCARS_ListAsTVector3D(PyList_GetItem(LXN, 1));
      } catch (std::length_error e) {
        PyErr_SetString(PyExc_ValueError, "Incorrect format in 'points': Point or Normal does not have 3 elements");
        return NULL;
      }

      // Rotate point and normal
      X.RotateSelfXYZ(Rotations);
      N.RotateSelfXYZ(Rotations);

      // Translate point, normal does not get translated
      X += Translation;

      Surface.AddPoint(X, N);
    } else {
      // input format error
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'points'");
      return NULL;
    }
  }

  // Check number of particles
  if (NParticles < 0) {
    PyErr_SetString(PyExc_ValueError, "'nparticles' must be >= 1 (sort of)");
    return NULL;
  }

  // Check GPU parameter
  if (GPU != 0 && GPU != 1) {
    PyErr_SetString(PyExc_ValueError, "'gpu' must be 0 or 1");
    return NULL;
  }

  // Check NThreads parameter
  if (NThreads < 0) {
    PyErr_SetString(PyExc_ValueError, "'nthreads' must be > 0");
    return NULL;
  }

  // Check you are not trying to use threads and GPU
  if (NThreads > 0 && GPU == 1) {
    PyErr_SetString(PyExc_ValueError, "gpu is 1 and nthreads > 0.  Both are not currently allowed.");
    return NULL;
  }
    




  // Container for Point plus scalar
  T3DScalarContainer FluxContainer;

  try {
    self->obj->CalculateFlux(Surface, Energy_eV, FluxContainer, NParticles, NThreads, GPU, Dim, OutFileName);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return NULL;
  } catch (std::out_of_range e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return NULL;
  }

  // Build the output list of: [[[x, y, z], Flux], [...]]
  // Create a python list
  PyObject *PList = PyList_New(0);

  size_t const NPoints = FluxContainer.GetNPoints();

  for (size_t i = 0; i != NPoints; ++i) {
    T3DScalar P = FluxContainer.GetPoint(i);

    // Inner list for each point
    PyObject *PList2 = PyList_New(0);


    // Add position and value to list
    PyList_Append(PList2, OSCARS_TVector3DAsList(P.GetX()));
    PyList_Append(PList2, Py_BuildValue("f", P.GetV()));
    PyList_Append(PList, PList2);

  }

  return PList;
}












static PyObject* OSCARS_CalculateFluxRectangle (OSCARSObject* self, PyObject* args, PyObject *keywds)
{
  // Calculate the spectrum given an observation point, and energy range

  char const* SurfacePlane = "";
  size_t      NX1 = 0;
  size_t      NX2 = 0;
  double      Width_X1 = 0;
  double      Width_X2 = 0;
  PyObject*   List_NPoints= PyList_New(0);
  PyObject*   List_Width= PyList_New(0);
  PyObject*   List_Translation = PyList_New(0);
  PyObject*   List_Rotations = PyList_New(0);
  PyObject*   List_X0X1X2 = PyList_New(0);
  int         NormalDirection = 0;
  int         Dim = 2;
  double      Energy_eV = 0;
  char const* Polarization = "";
  int         NParticles = 0;
  int         NThreads = 0;
  int         GPU = 0;
  char const* OutFileName = "";


  static char *kwlist[] = {"energy_eV", "npoints", "plane", "normal", "dim", "width", "rotations", "translation", "x0x1x2", "nparticles", "polarization", "nthreads", "gpu", "ofile", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "dO|siiOOOOisiis", kwlist,
                                                                   &Energy_eV,
                                                                   &List_NPoints,
                                                                   &SurfacePlane,
                                                                   &NormalDirection,
                                                                   &Dim,
                                                                   &List_Width,
                                                                   &List_Rotations,
                                                                   &List_Translation,
                                                                   &List_X0X1X2,
                                                                   &NParticles,
                                                                   &Polarization,
                                                                   &NThreads,
                                                                   &GPU,
                                                                   &OutFileName)) {
    return NULL;
  }

  // Check if a beam is at least defined
  if (self->obj->GetNParticleBeams() < 1) {
    PyErr_SetString(PyExc_ValueError, "No particle beam defined");
    return NULL;
  }


  // Check requested dimension
  if (Dim != 2 & Dim != 3) {
    PyErr_SetString(PyExc_ValueError, "'dim' must be 2 or 3");
    return NULL;
  }


  // The rectangular surface object we'll use
  TSurfacePoints_Rectangle Surface;

  if (PyList_Size(List_NPoints) == 2) {
    // NPoints in [m]
    NX1 = PyInt_AsSsize_t(PyList_GetItem(List_NPoints, 0));
    NX2 = PyInt_AsSsize_t(PyList_GetItem(List_NPoints, 1));
  } else {
    PyErr_SetString(PyExc_ValueError, "'npoints' must be [int, int]");
    return NULL;
  }



  if (NX1 <= 0 || NX2 <= 0) {
    PyErr_SetString(PyExc_ValueError, "an entry in 'npoints' is <= 0");
    return NULL;
  }

  // Vectors for rotations and translations.  Default to 0
  TVector3D Rotations(0, 0, 0);
  TVector3D Translation(0, 0, 0);

  // Check for Rotations in the input
  if (PyList_Size(List_Rotations) != 0) {
    try {
      Rotations = OSCARS_ListAsTVector3D(List_Rotations);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'rotations'");
      return NULL;
    }
  }


  // Check for Translation in the input
  if (PyList_Size(List_Translation) != 0) {
    try {
      Translation = OSCARS_ListAsTVector3D(List_Translation);
    } catch (std::length_error e) {
      PyErr_SetString(PyExc_ValueError, "Incorrect format in 'translation'");
      return NULL;
    }
  }

  if (PyList_Size(List_Width) == 2) {
    // Width in [m]
    Width_X1 = PyFloat_AsDouble(PyList_GetItem(List_Width, 0));
    Width_X2 = PyFloat_AsDouble(PyList_GetItem(List_Width, 1));
  }



  // If you are requesting a simple surface plane, check that you have widths
  if (std::strlen(SurfacePlane) != 0 && Width_X1 > 0 && Width_X2 > 0) {
    try {
      Surface.Init(SurfacePlane, (int) NX1, (int) NX2, Width_X1, Width_X2, Rotations, Translation, NormalDirection);
    } catch (std::invalid_argument e) {
      PyErr_SetString(PyExc_ValueError, e.what());
      return NULL;
    }
  }



  // If X0X1X2 defined
  std::vector<TVector3D> X0X1X2;

  if (PyList_Size(List_X0X1X2) != 0) {
    if (PyList_Size(List_X0X1X2) == 3) {
      for (int i = 0; i != 3; ++i) {
        PyObject* List_X = PyList_GetItem(List_X0X1X2, i);

        try {
          X0X1X2.push_back(OSCARS_ListAsTVector3D(List_X));
        } catch (std::length_error e) {
          PyErr_SetString(PyExc_ValueError, "Incorrect format in 'x0x1x2'");
          return NULL;
        }
      }
    } else {
      PyErr_SetString(PyExc_ValueError, "'x0x1x2' must have 3 XYZ points defined correctly");
      return NULL;
    }

    for (std::vector<TVector3D>::iterator it = X0X1X2.begin(); it != X0X1X2.end(); ++it) {
      it->RotateSelfXYZ(Rotations);
      *it += Translation;
    }

    // UPDATE: Check for orthogonality
    Surface.Init((int) NX1, (int) NX2, X0X1X2[0], X0X1X2[1], X0X1X2[2], NormalDirection);
  }


  // Check number of particles
  if (NParticles < 0) {
    PyErr_SetString(PyExc_ValueError, "'nparticles' must be >= 1 (sort of)");
    return NULL;
  }


  // Check GPU parameter
  if (GPU != 0 && GPU != 1) {
    PyErr_SetString(PyExc_ValueError, "'gpu' must be 0 or 1");
    return NULL;
  }

  // Check NThreads parameter
  if (NThreads < 0) {
    PyErr_SetString(PyExc_ValueError, "'nthreads' must be > 0");
    return NULL;
  }

  // Check you are not trying to use threads and GPU
  if (NThreads > 0 && GPU == 1) {
    PyErr_SetString(PyExc_ValueError, "gpu is 1 and nthreads > 0.  Both are not currently allowed.");
    return NULL;
  }
    



  // Container for Point plus scalar
  T3DScalarContainer FluxContainer;


  // Actually calculate the spectrum
  // UPDATE: Needed, directional?
  //bool const Directional = NormalDirection == 0 ? false : true;

  try {
    self->obj->CalculateFlux(Surface, Energy_eV, FluxContainer, NParticles, NThreads, GPU, Dim, OutFileName);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return NULL;
  } catch (std::out_of_range e) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return NULL;
  }



  // Build the output list of: [[[x, y, z], Flux], [...]]
  // Create a python list
  PyObject *PList = PyList_New(0);

  size_t const NPoints = FluxContainer.GetNPoints();

  for (size_t i = 0; i != NPoints; ++i) {
    T3DScalar P = FluxContainer.GetPoint(i);

    // Inner list for each point
    PyObject *PList2 = PyList_New(0);


    // Add position and value to list
    PyList_Append(PList2, OSCARS_TVector3DAsList(P.GetX()));
    PyList_Append(PList2, Py_BuildValue("f", P.GetV()));
    PyList_Append(PList, PList2);

  }

  return PList;
}


















static PyObject* OSCARS_AverageSpectra (OSCARSObject* self, PyObject* args, PyObject *keywds)
{
  // Calculate the flux on a surface given an energy and list of points in 3D

  PyObject*   List_InFileNamesText = PyList_New(0);
  PyObject*   List_InFileNamesBinary = PyList_New(0);
  char const* OutFileNameText = "";
  char const* OutFileNameBinary = "";


  static char *kwlist[] = {"ifiles", "bifiles", "ofile", "bofile", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "|OOss", kwlist,
                                                          &List_InFileNamesText,
                                                          &List_InFileNamesBinary,
                                                          &OutFileNameText,
                                                          &OutFileNameBinary)) {
    return NULL;
  }

  // Grab the number of input files for both text and binary lists
  size_t const NFilesText = PyList_Size(List_InFileNamesText);
  size_t const NFilesBinary = PyList_Size(List_InFileNamesBinary);

  // Doesn't allow for both binary and text input at the same time
  if (NFilesText != 0 && NFilesBinary !=0) {
    PyErr_SetString(PyExc_ValueError, "either text or binary files may be added, but not both.");
    return NULL;
  }

  // Check that there is at least one file
  if (NFilesText + NFilesBinary < 1) {
    PyErr_SetString(PyExc_ValueError, "No files given.  You need at least one file as input in a list.");
    return NULL;
  }

  // Add file names to vector
  std::vector<std::string> FileNames;
  for (size_t i = 0; i != NFilesText; ++i) {
    FileNames.push_back( PyString_AsString(PyList_GetItem(List_InFileNamesText, i)) );
  }
  for (size_t i = 0; i != NFilesBinary; ++i) {
    FileNames.push_back( PyString_AsString(PyList_GetItem(List_InFileNamesBinary, i)) );
  }

  // Container for flux average
  TSpectrumContainer Container;

  // Either they are text files or binary files
  if (NFilesText > 0) {
    Container.AverageFromFilesText(FileNames);
  } else {
    //Container.AverageFromFilesBinary(FileNames);
  }

  // Text output
  if (std::string(OutFileNameText) != "") {
    Container.WriteToFileText(OutFileNameText);
  }

  // Binary output
  if (std::string(OutFileNameBinary) != "") {
    Container.WriteToFileBinary(OutFileNameBinary);
  }


  return OSCARS_GetSpectrumAsList(self, Container);
}









static PyObject* OSCARS_AddToSpectrum (OSCARSObject* self, PyObject* args, PyObject *keywds)
{
  // Calculate the flux on a surface given an energy and list of points in 3D

  PyObject*   List_Spectrum = PyList_New(0);
  double Weight = 1;


  static char *kwlist[] = {"spectrum", "weight", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|d", kwlist,
                                                        &List_Spectrum,
                                                        &Weight)) {
    return NULL;
  }


  // Check if there is an input spectrum

  if (PyList_Size(List_Spectrum) < 1) {
    PyErr_SetString(PyExc_ValueError, "No points in spectrum.");
    return NULL;
  }
  TSpectrumContainer S = OSCARS_GetSpectrumFromList(List_Spectrum);

  self->obj->AddToSpectrum(S, Weight);

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}




static PyObject* OSCARS_GetSpectrum (OSCARSObject* self)
{
  // Calculate the flux on a surface given an energy and list of points in 3D

  return OSCARS_GetSpectrumAsList(self, self->obj->GetSpectrum());
}






static PyObject* OSCARS_AddToFlux (OSCARSObject* self, PyObject* args, PyObject *keywds)
{
  // Calculate the flux on a surface given an energy and list of points in 3D

  PyObject*   List_Flux = PyList_New(0);
  double Weight = 1;


  static char *kwlist[] = {"flux", "weight", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|d", kwlist,
                                                        &List_Flux,
                                                        &Weight)) {
    return NULL;
  }


  // Check if there is an input spectrum

  if (PyList_Size(List_Flux) < 1) {
    PyErr_SetString(PyExc_ValueError, "No points in flux.");
    return NULL;
  }
  T3DScalarContainer F = OSCARS_GetT3DScalarContainerFromList(List_Flux);

  self->obj->AddToFlux(F, Weight);

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}



static PyObject* OSCARS_GetFlux (OSCARSObject* self)
{
  // Return flux list

  return OSCARS_GetT3DScalarAsList(self, self->obj->GetFlux());
}






static PyObject* OSCARS_AddToPowerDensity (OSCARSObject* self, PyObject* args, PyObject *keywds)
{
  // Calculate the flux on a surface given an energy and list of points in 3D

  PyObject*   List_PowerDensity = PyList_New(0);
  double Weight = 1;


  static char *kwlist[] = {"flux", "weight", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|d", kwlist,
                                                        &List_PowerDensity,
                                                        &Weight)) {
    return NULL;
  }


  // Check if there is an input spectrum

  if (PyList_Size(List_PowerDensity) < 1) {
    PyErr_SetString(PyExc_ValueError, "No points in flux.");
    return NULL;
  }
  T3DScalarContainer F = OSCARS_GetT3DScalarContainerFromList(List_PowerDensity);

  self->obj->AddToPowerDensity(F, Weight);

  // Must return python object None in a special way
  Py_INCREF(Py_None);
  return Py_None;
}



static PyObject* OSCARS_GetPowerDensity (OSCARSObject* self)
{
  // Return flux list

  return OSCARS_GetT3DScalarAsList(self, self->obj->GetPowerDensity());
}










static PyObject* OSCARS_AverageT3DScalars (OSCARSObject* self, PyObject* args, PyObject *keywds)
{
  // Calculate the flux on a surface given an energy and list of points in 3D

  PyObject*   List_InFileNamesText = PyList_New(0);
  PyObject*   List_InFileNamesBinary = PyList_New(0);
  int         Dim = 2;
  char const* OutFileNameText = "";
  char const* OutFileNameBinary = "";


  static char *kwlist[] = {"ifiles", "bifiles", "ofile", "bofile", "dim", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "|OOssi", kwlist,
                                                         &List_InFileNamesText,
                                                         &List_InFileNamesBinary,
                                                         &OutFileNameText,
                                                         &OutFileNameBinary,
                                                         &Dim)) {
    return NULL;
  }

  // Grab the number of input files for both text and binary lists
  size_t const NFilesText = PyList_Size(List_InFileNamesText);
  size_t const NFilesBinary = PyList_Size(List_InFileNamesBinary);

  // Doesn't allow for both binary and text input at the same time
  if (NFilesText != 0 && NFilesBinary !=0) {
    PyErr_SetString(PyExc_ValueError, "either text or binary files may be added, but not both.");
    return NULL;
  }

  // Check that there is at least one file
  if (NFilesText + NFilesBinary < 1) {
    PyErr_SetString(PyExc_ValueError, "No files given.  You need at least one file as input in a list.");
    return NULL;
  }

  // Add file names to vector
  std::vector<std::string> FileNames;
  for (size_t i = 0; i != NFilesText; ++i) {
    FileNames.push_back( PyString_AsString(PyList_GetItem(List_InFileNamesText, i)) );
  }
  for (size_t i = 0; i != NFilesBinary; ++i) {
    FileNames.push_back( PyString_AsString(PyList_GetItem(List_InFileNamesBinary, i)) );
  }

  // Container for flux average
  T3DScalarContainer Container;

  // Either they are text files or binary files
  if (NFilesText > 0) {
    Container.AverageFromFilesText(FileNames, Dim);
  } else {
    Container.AverageFromFilesBinary(FileNames, Dim);
  }

  // Build the output list of: [[[x, y, z], Value], [...]]
  // Create a python list
  PyObject *PList = PyList_New(0);

  // Number of points in container
  size_t const NPoints = Container.GetNPoints();

  for (size_t i = 0; i != NPoints; ++i) {

    // This point in container
    T3DScalar P = Container.GetPoint(i);

    // Inner list for each point
    PyObject *PList2 = PyList_New(0);


    // Add position and value to list
    PyList_Append(PList2, OSCARS_TVector3DAsList(P.GetX()));
    PyList_Append(PList2, Py_BuildValue("f", P.GetV()));
    PyList_Append(PList, PList2);

  }

  // Text output
  if (std::string(OutFileNameText) != "") {
    Container.WriteToFileText(OutFileNameText, Dim);
  }

  // Binary output
  if (std::string(OutFileNameBinary) != "") {
    Container.WriteToFileBinary(OutFileNameBinary, Dim);
  }

  return PList;
}









static PyObject* OSCARS_CalculateElectricFieldTimeDomain (OSCARSObject* self, PyObject* args, PyObject *keywds)
{
  // Calculate the electric field in the proper time domain.
  // The warning for using this function is that it returns unevenly spaced
  // time steps in the lab frame.  The calculation is based on the time stamps/steps
  // in the Trajectory object.  ie don't blindly employ a DFT or FFT.

  PyObject*   List_Obs = PyList_New(0);
  char const* OutFileName = "";


  static char *kwlist[] = {"obs", "ofile", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, keywds, "O|O", kwlist,
                                                        &List_Obs,
                                                        &OutFileName)) {
    return NULL;
  }

  // Check if a beam is at least defined
  if (self->obj->GetNParticleBeams() < 1) {
    PyErr_SetString(PyExc_ValueError, "No particle beam defined");
    return NULL;
  }


  // Observation point
  TVector3D Obs(0, 0, 0);
  try {
      Obs = OSCARS_ListAsTVector3D(List_Obs);
  } catch (std::length_error e) {
    PyErr_SetString(PyExc_ValueError, "Incorrect format in 'obs'");
    return NULL;
  }

  T3DScalarContainer XYZT;
  self->obj->CalculateElectricFieldTimeDomain(Obs, XYZT);

  // UPDATE: Format is not great for XYZT output
  if (std::string(OutFileName) != "") {
    XYZT.WriteToFileText(OutFileName, 3);
  }

  // Build the output list of: [[[x, y, z], Flux], [...]]
  // Create a python list
  PyObject *PList = PyList_New(0);

  size_t const NPoints = XYZT.GetNPoints();

  for (size_t i = 0; i != NPoints; ++i) {
    T3DScalar P = XYZT.GetPoint(i);

    // Inner list for each point
    PyObject *PList2 = PyList_New(0);


    // Add position and value to list
    PyList_Append(PList2, Py_BuildValue("f", P.GetV()));
    PyList_Append(PList2, OSCARS_TVector3DAsList(P.GetX()));
    PyList_Append(PList, PList2);

  }

  return PList;
}






























static PyGetSetDef OSCARS_getseters[] = {
  {"npoints_trajectory",  (getter)OSCARS_GetNPointsTrajectory,  (setter)OSCARS_SetNPointsTrajectory,  "Total number of points for trajectory", NULL},
  {NULL}  /* Sentinel */
};




static PyMethodDef OSCARS_methods[] = {
  // We must tell python about the function we allow access as well as give them nice
  // python names, and tell python the method of input parameters.

  {"pi",                                (PyCFunction) OSCARS_Pi,                              METH_NOARGS,                  "do you want some pi?"},
  {"qe",                                (PyCFunction) OSCARS_Qe,                              METH_NOARGS,                  "elementary charge in [C]"},
  {"me",                                (PyCFunction) OSCARS_Me,                              METH_NOARGS,                  "electron mass in [kg]"},
  {"rand",                              (PyCFunction) OSCARS_Random,                          METH_NOARGS,                  "uniformally distributed random number [0, 1)"},
  {"norm",                              (PyCFunction) OSCARS_RandomNormal,                    METH_NOARGS,                  "Normally distributed random number"},
  {"set_seed",                          (PyCFunction) OSCARS_SetSeed,                         METH_O,                       "Set the random seed"},
  {"set_gpu_global",                    (PyCFunction) OSCARS_SetGPUGlobal,                    METH_O,                       "Set global use GPU"},
  {"check_gpu",                         (PyCFunction) OSCARS_CheckGPU,                        METH_NOARGS,                  "returns the number of GPU units available, negative for errors"},
  {"set_nthreads_global",               (PyCFunction) OSCARS_SetNThreadsGlobal,               METH_O,                       "Set global number of threads to use"},
                                                                                                                            
  {"get_ctstart",                       (PyCFunction) OSCARS_GetCTStart,                      METH_NOARGS,                  "get the start time in [m]"},
  {"get_ctstop",                        (PyCFunction) OSCARS_GetCTStop,                       METH_NOARGS,                  "get the stop time in [m]"},
  {"set_ctstartstop",                   (PyCFunction) OSCARS_SetCTStartStop,                  METH_VARARGS,                 "set the start and stop time in [m]"},
  {"set_npoints_trajectory",            (PyCFunction) OSCARS_SetNPointsTrajectory,            METH_O,                       "set the total number of points for the trajectory"},
  {"get_npoints_trajectory",            (PyCFunction) OSCARS_GetNPointsTrajectory,            METH_NOARGS,                  "get the total number of points for the trajectory"},
                                                                                          
  {"add_bfield_file",                   (PyCFunction) OSCARS_AddMagneticField,                METH_VARARGS | METH_KEYWORDS, "add a magnetic field from a file"},
  {"add_bfield_function",               (PyCFunction) OSCARS_AddMagneticFieldFunction,        METH_VARARGS,                 "add a magnetic field in form of python function"},
  {"add_bfield_gaussian",               (PyCFunction) OSCARS_AddMagneticFieldGaussian,        METH_VARARGS | METH_KEYWORDS, "add a magnetic field in form of 3D gaussian"},
  {"add_bfield_uniform",                (PyCFunction) OSCARS_AddMagneticFieldUniform,         METH_VARARGS | METH_KEYWORDS, "add a uniform magnetic field in 3D"},
  {"add_bfield_undulator",              (PyCFunction) OSCARS_AddMagneticFieldIdealUndulator,  METH_VARARGS | METH_KEYWORDS, "add magnetic field from ideal undulator in 3D"},
  {"get_bfield",                        (PyCFunction) OSCARS_GetBField,                       METH_VARARGS,                 "get the magnetic field at a given position in space (and someday time?)"},
  {"clear_bfields",                     (PyCFunction) OSCARS_ClearMagneticFields,             METH_NOARGS,                  "clear all internal magnetic fields"},

  {"add_efield_file",                   (PyCFunction) OSCARS_AddElectricField,                METH_VARARGS | METH_KEYWORDS, "add an electric field from a file"},
  {"add_efield_function",               (PyCFunction) OSCARS_AddElectricFieldFunction,        METH_VARARGS,                 "add an electric field in form of python function"},
  {"add_efield_gaussian",               (PyCFunction) OSCARS_AddElectricFieldGaussian,        METH_VARARGS | METH_KEYWORDS, "add an electric field in form of 3D gaussian"},
  {"add_efield_uniform",                (PyCFunction) OSCARS_AddElectricFieldUniform,         METH_VARARGS | METH_KEYWORDS, "add a uniform electric field in 3D"},
  {"add_efield_undulator",              (PyCFunction) OSCARS_AddElectricFieldIdealUndulator,  METH_VARARGS | METH_KEYWORDS, "add magnetic field from ideal undulator in 3D"},
  {"get_efield",                        (PyCFunction) OSCARS_GetEField,                       METH_VARARGS,                 "get the electric field at a given position in space (and someday time?)"},
  {"clear_efields",                     (PyCFunction) OSCARS_ClearElectricFields,             METH_NOARGS,                  "clear all internal electric fields"},
 
  {"add_field_gaussian",                (PyCFunction) OSCARS_AddFieldGaussian,                METH_VARARGS | METH_KEYWORDS, "add a magnetic or electric field in form of 3D gaussian"},

  {"write_bfield",                      (PyCFunction) OSCARS_WriteMagneticField,              METH_VARARGS | METH_KEYWORDS, "write the magnetic field to a file"},
  {"write_efield",                      (PyCFunction) OSCARS_WriteElectricField,              METH_VARARGS | METH_KEYWORDS, "write the magnetic field to a file"},

                                                                                          
  {"set_particle_beam",                 (PyCFunction) OSCARS_SetParticleBeam,                 METH_VARARGS | METH_KEYWORDS, "add a particle beam"},
  {"add_particle_beam",                 (PyCFunction) OSCARS_AddParticleBeam,                 METH_VARARGS | METH_KEYWORDS, "add a particle beam"},
  {"clear_particle_beams",              (PyCFunction) OSCARS_ClearParticleBeams,              METH_NOARGS,                  "Clear all existing particle beams from OSCARS"},
                                                                                          
  {"set_new_particle",                  (PyCFunction) OSCARS_SetNewParticle,                  METH_VARARGS | METH_KEYWORDS, "Set the internal particle to a new random particle"},
  {"get_particle_x0",                   (PyCFunction) OSCARS_GetParticleX0,                   METH_NOARGS,                  "Get the position at t0"},
  {"get_particle_beta0",                (PyCFunction) OSCARS_GetParticleBeta0,                METH_NOARGS,                  "Get the beta at t0"},
  {"get_particle_e0",                   (PyCFunction) OSCARS_GetParticleE0,                   METH_NOARGS,                  "Get the Energy at t0"},
                                                                                          
  {"calculate_trajectory",              (PyCFunction) OSCARS_CalculateTrajectory,             METH_NOARGS,                  "Calclate the trajectory for the current particle"},
  {"get_trajectory",                    (PyCFunction) OSCARS_GetTrajectory,                   METH_NOARGS,                  "Get the trajectory for the current particle"},

  {"calculate_spectrum",                (PyCFunction) OSCARS_CalculateSpectrum,               METH_VARARGS | METH_KEYWORDS, "calculate the spectrum at an observation point"},

  {"calculate_total_power",             (PyCFunction) OSCARS_CalculateTotalPower,             METH_NOARGS,                  "calculate total power radiated"},
  {"calculate_power_density_rectangle", (PyCFunction) OSCARS_CalculatePowerDensityRectangle,  METH_VARARGS | METH_KEYWORDS, "calculate the power density given a surface"},
  {"calculate_power_density",           (PyCFunction) OSCARS_CalculatePowerDensity,           METH_VARARGS | METH_KEYWORDS, "calculate the power density given a surface"},
  {"calculate_flux",                    (PyCFunction) OSCARS_CalculateFlux,                   METH_VARARGS | METH_KEYWORDS, "calculate the flux given a surface"},
  {"calculate_flux_rectangle",          (PyCFunction) OSCARS_CalculateFluxRectangle,          METH_VARARGS | METH_KEYWORDS, "calculate the flux given a surface"},

  {"average_spectra",                   (PyCFunction) OSCARS_AverageSpectra,                  METH_VARARGS | METH_KEYWORDS, "average spectra"},
  {"average_flux",                      (PyCFunction) OSCARS_AverageT3DScalars,               METH_VARARGS | METH_KEYWORDS, "average fluxes"},
  {"average_power_density",             (PyCFunction) OSCARS_AverageT3DScalars,               METH_VARARGS | METH_KEYWORDS, "average power densities"},

  {"add_to_spectrum",                   (PyCFunction) OSCARS_AddToSpectrum,                   METH_VARARGS | METH_KEYWORDS, "add to the running average of a spectrum"},
  {"get_spectrum",                      (PyCFunction) OSCARS_GetSpectrum,                     METH_VARARGS | METH_KEYWORDS, "get the internal to OSCARS spectrum"},
  {"add_to_flux",                       (PyCFunction) OSCARS_AddToFlux,                       METH_VARARGS | METH_KEYWORDS, "add to the running average of a flux"},
  {"get_flux",                          (PyCFunction) OSCARS_GetFlux,                         METH_VARARGS | METH_KEYWORDS, "get the internal to OSCARS flux"},
  {"add_to_power_density",              (PyCFunction) OSCARS_AddToPowerDensity,               METH_VARARGS | METH_KEYWORDS, "add to the running average of a flux"},
  {"get_power_density",                 (PyCFunction) OSCARS_GetPowerDensity,                 METH_VARARGS | METH_KEYWORDS, "get the internal to OSCARS flux"},

  {"calculate_efield_vs_time",          (PyCFunction) OSCARS_CalculateElectricFieldTimeDomain,METH_VARARGS | METH_KEYWORDS, "calculate the electric field in the time domain"},

  {NULL}  /* Sentinel */
};




static PyTypeObject OSCARSType = {
  // The python object.  Fully defined elsewhere.  only put here what you need,
  // otherwise default values

  PyObject_HEAD_INIT(NULL)
  0,                                        /* ob_size */
  "mod.OSCARS",                                 /* tp_name */
  sizeof(OSCARSObject),                         /* tp_basicsize */
  0,                                        /* tp_itemsize */
  (destructor) OSCARS_dealloc,                 /* tp_dealloc */
  0,                                        /* tp_print */
  0,                                        /* tp_getattr */
  0,                                        /* tp_setattr */
  0,                                        /* tp_compare */
  0,                                        /* tp_repr */
  0,                                        /* tp_as_number */
  0,                                        /* tp_as_sequence */
  0,                                        /* tp_as_mapping */
  0,                                        /* tp_hash */
  0,                                        /* tp_call */
  0,                                        /* tp_str */
  0,                                        /* tp_getattro */
  0,                                        /* tp_setattro */
  0,                                        /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
  "OSCARS class",                              /* tp_doc */
  0,                                        /* tp_traverse */
  0,                                        /* tp_clear */
  0,                                        /* tp_richcompare */
  0,                                        /* tp_weaklistoffset */
  0,                                        /* tp_iter */
  0,                                        /* tp_iternext */
  OSCARS_methods,                             /* tp_methods */
  0,                                        /* tp_members */
  OSCARS_getseters,                           /* tp_getset */
  0,                                        /* tp_base */
  0,                                        /* tp_dict */
  0,                                        /* tp_descr_get */
  0,                                        /* tp_descr_set */
  0,                                        /* tp_dictoffset */
  0,                                        /* tp_init */
  0,                                        /* tp_alloc */
  OSCARS_new,                                  /* tp_new */
};




static PyMethodDef module_methods[] = {
  // I do not need
  {NULL}  /* Sentinel */
};




PyMODINIT_FUNC initOSCARS ()
{
  // Initialization of the module.


  PyObject* m;

  if (PyType_Ready(&OSCARSType) < 0)
    return;

  m = Py_InitModule3("OSCARS", module_methods, "OSCARS Extension module for the OSCARS Python API.");
  if (m == NULL)
    return;

  Py_INCREF(&OSCARSType);
  PyModule_AddObject(m, "OSCARS", (PyObject *)&OSCARSType);


  // Print copyright notice
  PyObject* sys = PyImport_ImportModule( "sys");
  PyObject* s_out = PyObject_GetAttrString(sys, "stdout");

  std::string Message = "OSCARS v" + OSCARS::GetVersionString() + " - Open Source Code for Advanced Radiation Simulation\nBrookhaven National Laboratory, Upton NY, USA\nhttp://oscars.bnl.gov\noscars@bnl.gov\n";

  PyObject_CallMethod(s_out, "write", "s", Message.c_str());


}

