/* Siconos-Kernel version 1.1.4, Copyright INRIA 2005-2006.
 * Siconos is a program dedicated to modeling, simulation and control
 * of non smooth dynamical systems.
 * Siconos is a free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Siconos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Siconos; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact: Vincent ACARY vincent.acary@inrialpes.fr
*/
#include "DynamicalSystem.h"

// includes to be deleted thanks to factories
#include "LinearBC.h"
#include "NLinearBC.h"
#include "PeriodicBC.h"
#include "LinearDSIO.h"
#include "LagrangianDSIO.h"
#include "LagrangianLinearDSIO.h"

using namespace std;

// ===== CONSTRUCTORS =====

// Default constructor (private)
DynamicalSystem::DynamicalSystem():
  DSType(NLDS), nsds(NULL), number(0), id("none"), n(0), x0(NULL), x(NULL), xMemory(NULL),
  xDot(NULL), xDotMemory(NULL), xFree(NULL), r(NULL), rMemory(NULL), jacobianX(NULL),
  uSize(0), u(NULL), T(NULL), stepsInMemory(1), BC(NULL), dsxml(NULL),
  vectorFieldFunctionName("none"), computeJacobianXFunctionName("none"), computeUFunctionName("none"),
  computeTFunctionName("none"), vectorFieldPtr(NULL), computeJacobianXPtr(NULL),
  computeUPtr(NULL), computeTPtr(NULL), isBCAllocatedIn(false)
{
  // --- plugins (not u and T because they are optional) -> connected to  "false" plugin
  isXAllocatedIn.resize(7, false);
  isRAllocatedIn.resize(2, false);
  isControlAllocatedIn.resize(2, false);

  // plug-in parameters initialization -> dim. 1 simple vectors. with v(0) = 0.
  parametersList0.reserve(4);
  for (unsigned int i = 0; i < 4; ++i)
    parametersList0.push_back(new SimpleVector(1));
  isParametersList0AllocatedIn.resize(4, true);
  vector<SimpleVector*>::iterator iter;
  for (iter = parametersList0.begin(); iter != parametersList0.end(); ++iter)
    (*iter)->zero();
}

// From XML file (warning: newNsds is optional, default = NULL)
DynamicalSystem::DynamicalSystem(DynamicalSystemXML * dsXML, NonSmoothDynamicalSystem* newNsds):
  DSType(NLDS), nsds(newNsds), number(0), id("none"), n(0), x0(NULL), x(NULL), xMemory(NULL),
  xDot(NULL), xDotMemory(NULL), xFree(NULL), r(NULL), rMemory(NULL), jacobianX(NULL),
  uSize(0), u(NULL), T(NULL), stepsInMemory(1), BC(NULL), dsxml(dsXML),
  vectorFieldFunctionName("none"), computeJacobianXFunctionName("none"), computeUFunctionName("none"),
  computeTFunctionName("none"), vectorFieldPtr(NULL), computeJacobianXPtr(NULL),
  computeUPtr(NULL), computeTPtr(NULL), isBCAllocatedIn(false)
{
  // --- get values in xml file ---
  if (dsXML != NULL)
  {
    number = dsxml->getNumber();
    if (dsxml->hasId() == true) id = dsxml->getId();
    // n is required only for first order systems (not for Lagrangian ones, replaced by ndof)
    if (dsxml->hasN() == true) n = dsxml->getN();
    else if (DSType == NLDS || DSType == LDS)
      RuntimeException::selfThrow("DynamicalSystem:: xml constructor, n (problem size) is a required input");

    // --- Memory allocation for vector and matrix members ---

    // x and related vectors
    if (dsxml->hasX0() == true)
      x0 = new SimpleVector(dsxml->getX0());
    else
      RuntimeException::selfThrow("DynamicalSystem:: xml constructor, x0 is a required input");
    if (x0->size() != n)
      RuntimeException::selfThrow("DynamicalSystem:: xml constructor, x size differs from n!");

    if (dsxml->hasX() == true) x = new SimpleVector(dsxml->getX());
    else x = new SimpleVector(*x0);
    if (x->size() != n)
      RuntimeException::selfThrow("DynamicalSystem:: xml constructor, x size differs from n!");

    xFree = new SimpleVector(n);

    // r
    r = new SimpleVector(n);

    string plugin;
    // vectorField
    if (dsxml->hasXDot() && dsxml->hasVectorFieldPlugin())
      RuntimeException::selfThrow("DynamicalSystem:: xml constructor, you can not give xDot and vectorField plug-in, only one is required.");

    if (dsxml->hasXDot())
    {
      xDot = new SimpleVector(dsxml->getXDot());
      if (xDot->size() != n)
        RuntimeException::selfThrow("DynamicalSystem:: xml constructor, xDot size differs from n!");
      setVectorFieldFunction("DefaultPlugin.so", "vectorField");
    }
    else if (dsxml->hasVectorFieldPlugin())
    {
      xDot = new SimpleVector(n);
      plugin = dsxml->getVectorFieldPlugin();
      setVectorFieldFunction(cShared.getPluginName(plugin), cShared.getPluginFunctionName(plugin));
    }
    else
    {
      xDot = new SimpleVector(n); // for derived classes
      setVectorFieldFunction("DefaultPlugin.so", "vectorField");
    }

    // jacobianX
    jacobianX = new SimpleMatrix(n, n);
    if (dsxml->hasComputeJacobianXPlugin())
    {
      plugin = dsxml->getComputeJacobianXPlugin();
      setComputeJacobianXFunction(cShared.getPluginName(plugin), cShared.getPluginFunctionName(plugin));
    }
    else
      setComputeJacobianXFunction("DefaultPlugin.so", "computeJacobianX");

    isXAllocatedIn.resize(7, true);

    // plug-in parameters initialization -> dim. 1 simple vectors. with v(0) = 0.
    parametersList0.reserve(4);
    for (unsigned int i = 0; i < 4; ++i)
      parametersList0.push_back(new SimpleVector(1));
    isParametersList0AllocatedIn.resize(4, true);
    vector<SimpleVector*>::iterator iter;
    for (iter = parametersList0.begin(); iter != parametersList0.end(); ++iter)
      (*iter)->zero();

    // Memory
    isXAllocatedIn[2] = false ; //xMemory
    isXAllocatedIn[4] = false ; // xDotMemory
    isRAllocatedIn.resize(2, true);
    isRAllocatedIn[1] = false ; // rMemory
    if (dsxml->hasStepsInMemory() == true) stepsInMemory = dsxml->getStepsInMemory();

    if (dsxml->hasXMemory() == true)
    {
      xMemory = new SiconosMemory(dsxml->getXMemoryXML());
      isXAllocatedIn[2] = true;
    }

    if (dsxml->hasXDotMemory() == true)
    {
      xDotMemory = new SiconosMemory(dsxml->getXDotMemoryXML());
      isXAllocatedIn[4] = true;
    }

    // --- u and T xml loading (optional) ---
    isControlAllocatedIn.resize(2, false);
    isPlugin.resize(2, false);
    if (dsxml->hasT())
    {
      // uSize is required
      if (dsxml->hasUSize())
        uSize = dsxml -> getUSize();
      else
        RuntimeException::selfThrow("DynamicalSystem:: xml constructor, uSize is a required input");

      T = new SimpleMatrix(n, uSize);
      u = new SimpleVector(uSize);
      isControlAllocatedIn[0] = true;
      isControlAllocatedIn[1] = true;

      // if T plugin, set compute function, else load matrix from xml
      if (dsxml->isTPlugin())
      {
        plugin = dsxml->getTPlugin();
        setComputeTFunction(cShared.getPluginName(plugin), cShared.getPluginFunctionName(plugin));
        isPlugin[0] = true;
      }
      else
        *T = dsxml->getTMatrix();
    }
    // optional xml loading for u
    if (dsxml->hasU())
    {
      // check if uSize has already been loaded
      if (uSize == 0)
      {
        if (dsxml->hasUSize())
          uSize = dsxml -> getUSize();
        else
          RuntimeException::selfThrow("DynamicalSystem:: xml constructor, uSize is a required input");
      }

      // check if u is allocated
      if (u == NULL)
      {
        u = new SimpleVector(uSize);
        isControlAllocatedIn[0] = true;
      }

      if (dsxml->isUPlugin())
      {
        plugin = dsxml->getUPlugin();
        setComputeUFunction(cShared.getPluginName(plugin), cShared.getPluginFunctionName(plugin));
        isPlugin[1] = true;
      }
      else if (dsxml->getUVector().size() == uSize)
        *u = dsxml->getUVector();
      else
        RuntimeException::selfThrow("DynamicalSystem:: xml constructor, inconsistent size between uSize, u and T");
    }

    // --- Boundary conditions ---
    fillBoundaryConditionsFromXml();

    // --- DS input-output ---
    fillDsioFromXml();
  }
  else
    RuntimeException::selfThrow("DynamicalSystem::DynamicalSystem - DynamicalSystemXML paramater must not be NULL");
}

// From a minimum set of data
DynamicalSystem::DynamicalSystem(const int& newNumber, const unsigned int& newN, const SiconosVector& newX0,
                                 const string& vectorFieldPlugin, const string& jacobianXPlugin):
  DSType(NLDS), nsds(NULL), number(newNumber), id("none"), n(newN), x0(NULL), x(NULL), xMemory(NULL),
  xDot(NULL), xDotMemory(NULL), xFree(NULL), r(NULL), rMemory(NULL), jacobianX(NULL), uSize(0), u(NULL),
  T(NULL), stepsInMemory(1), BC(NULL), dsxml(NULL),
  vectorFieldFunctionName("none"), computeJacobianXFunctionName("none"), computeUFunctionName("none"),
  computeTFunctionName("none"), vectorFieldPtr(NULL), computeJacobianXPtr(NULL),
  computeUPtr(NULL), computeTPtr(NULL), isBCAllocatedIn(false)
{
  // --- Memory allocation ---
  if (n != newX0.size())
    RuntimeException::selfThrow("DynamicalSystem::constructor from data, inconsistent sizes between problem size and x0.");

  // x and related vectors
  x0 = new SimpleVector(newX0);
  x = new SimpleVector(*x0); // x is initialized with x0.
  xFree = new SimpleVector(n);

  // r
  r = new SimpleVector(n);

  // vectorField
  xDot = new SimpleVector(n);
  setVectorFieldFunction(cShared.getPluginName(vectorFieldPlugin), cShared.getPluginFunctionName(vectorFieldPlugin));

  // jacobianX
  jacobianX = new SimpleMatrix(n, n);
  setComputeJacobianXFunction(cShared.getPluginName(jacobianXPlugin), cShared.getPluginFunctionName(jacobianXPlugin));

  isXAllocatedIn.resize(7, true);

  // Memory
  isXAllocatedIn[2] = false ; //xMemory
  isXAllocatedIn[4] = false ; // xDotMemory
  isRAllocatedIn.resize(2, true);
  isRAllocatedIn[1] = false ; // rMemory

  // plug-in parameters initialization -> dim. 1 simple vectors. with v(0) = 0.
  parametersList0.reserve(4);
  for (unsigned int i = 0; i < 4; ++i)
    parametersList0.push_back(new SimpleVector(1));
  isParametersList0AllocatedIn.resize(4, true);
  vector<SimpleVector*>::iterator iter;
  for (iter = parametersList0.begin(); iter != parametersList0.end(); ++iter)
    (*iter)->zero();

  // u and T are optional
  isPlugin.resize(2, false);
  isControlAllocatedIn.resize(2, false);
}

// copy constructor
DynamicalSystem::DynamicalSystem(const DynamicalSystem& newDS):
  DSType(NLDS), nsds(newDS.getNSDSPtr()), number(-2), id("copy"), n(newDS.getN()),
  x0(NULL), x(NULL), xMemory(NULL), xDot(NULL), xDotMemory(NULL),
  xFree(NULL), r(NULL), rMemory(NULL), jacobianX(NULL), uSize(newDS.getUSize()), u(NULL), T(NULL),
  stepsInMemory(newDS.getStepsInMemory()), BC(NULL), dsxml(NULL),
  vectorFieldFunctionName(newDS.getVectorFieldFunctionName()), computeJacobianXFunctionName(newDS. getVectorFieldFunctionName()),
  computeUFunctionName(newDS.getComputeUFunctionName()), computeTFunctionName(newDS.getComputeTFunctionName()),
  vectorFieldPtr(NULL), computeJacobianXPtr(NULL),
  computeUPtr(NULL), computeTPtr(NULL), isBCAllocatedIn(false)
{

  cout << "!!! Warning: Dynamical System copy, do not forget to set id and number for the new system !!! " << endl;

  if (newDS.getX0Ptr()->isBlock())
  {
    x0 = new BlockVector(newDS.getX0());
    x = new BlockVector(newDS.getX());
    xDot = new BlockVector(newDS.getXDot());
    xFree = new BlockVector(newDS.getXFree());
  }
  else
  {
    x0 = new SimpleVector(newDS.getX0());
    x = new SimpleVector(newDS.getX());
    xDot = new SimpleVector(newDS.getXDot());
    xFree = new SimpleVector(newDS.getXFree());
  }
  isXAllocatedIn.resize(7, true);
  isXAllocatedIn[2] = false ; //xMemory
  isXAllocatedIn[4] = false ; // xDotMemory
  isXAllocatedIn[6] = false ; // jacobianX
  isRAllocatedIn.resize(2, false);
  isRAllocatedIn[1] = false ; // rMemory

  if (newDS.getRPtr() != NULL)
  {
    r = new SimpleVector(newDS.getR());
    isRAllocatedIn[0] = true ;
  }

  if (newDS.getJacobianXPtr() != NULL)
  {
    jacobianX = new SimpleMatrix(newDS.getJacobianX());
    isXAllocatedIn[6] = true ;
  }

  if (newDS.getXMemoryPtr() != NULL)
  {
    xMemory = new SiconosMemory(newDS.getXMemory());
    isXAllocatedIn[2] = true;
  }

  if (newDS.getXDotMemoryPtr() != NULL)
  {
    xDotMemory = new SiconosMemory(newDS.getXDotMemory());
    isXAllocatedIn[4] = true;
  }

  if (newDS.getRMemoryPtr() != NULL)
  {
    rMemory = new SiconosMemory(newDS.getRMemory());
    isRAllocatedIn[1] = true;
  }

  isControlAllocatedIn.resize(2, false);
  if (newDS.getUPtr() != NULL)
  {
    if (newDS.getUPtr()->isBlock())
      u = new BlockVector(newDS.getU());
    else
      u = new SimpleVector(newDS.getU());
    isControlAllocatedIn[0] = true;
  }

  if (newDS.getTPtr() != NULL)
  {
    T = new SimpleMatrix(newDS.getT());
    isControlAllocatedIn[1] = true ;
  }

  // plug-in parameters initialization -> dim. 1 simple vectors. with v(0) = 0.
  setParametersListVector(newDS.getParametersListVector());

  string pluginPath, functionName;

  functionName = cShared.getPluginFunctionName(vectorFieldFunctionName);
  pluginPath  = cShared.getPluginName(vectorFieldFunctionName);
  setVectorFieldFunction(pluginPath, functionName);

  functionName = cShared.getPluginFunctionName(computeJacobianXFunctionName);
  pluginPath  = cShared.getPluginName(computeJacobianXFunctionName);
  setComputeJacobianXFunction(pluginPath, functionName);

  isPlugin = newDS.getIsPlugin();

  // plugin for u
  if (isPlugin[0])
  {
    functionName = cShared.getPluginFunctionName(computeUFunctionName);
    pluginPath  = cShared.getPluginName(computeUFunctionName);
    setVectorFieldFunction(pluginPath, functionName);
  }
  // plugin for T
  if (isPlugin[1])
  {
    functionName = cShared.getPluginFunctionName(computeTFunctionName);
    pluginPath  = cShared.getPluginName(computeTFunctionName);
    setVectorFieldFunction(pluginPath, functionName);
  }

  // \todo: manage copy of dsio and boundary conditions when these classes will be well implemented.

  // xml link is not copied.
}

// --- Destructor ---
DynamicalSystem::~DynamicalSystem()
{
  if (isXAllocatedIn[0])delete x0;
  x0 = NULL ;
  if (isXAllocatedIn[1]) delete x;
  x = NULL;
  if (isXAllocatedIn[2]) delete xMemory;
  xMemory = NULL;
  if (isXAllocatedIn[3]) delete xDot;
  xDot = NULL;
  if (isXAllocatedIn[4]) delete xDotMemory;
  xDotMemory = NULL;
  if (isXAllocatedIn[5]) delete xFree;
  xFree = NULL;
  if (isRAllocatedIn[0]) delete r;
  r = NULL;
  if (isRAllocatedIn[1]) delete rMemory;
  rMemory = NULL;
  if (isXAllocatedIn[6]) delete jacobianX;
  jacobianX = NULL;
  if (isControlAllocatedIn[0]) delete u;
  u = NULL;
  if (isControlAllocatedIn[1]) delete T;
  T = NULL;

  for (unsigned int i = 0; i < parametersList0.size(); ++i)
  {
    if (isParametersList0AllocatedIn[i]) delete parametersList0[i];
    parametersList0[i] = NULL;
  }
  isParametersList0AllocatedIn.resize(4, false);

  for (unsigned int i = 0; i < dsioVector.size(); i++)
  {
    if (isDsioAllocatedIn[i]) delete dsioVector[i];
    dsioVector[i] = NULL;
  }
  if (isBCAllocatedIn) delete BC;
  BC = NULL;
}

// Boundary conditions built-in (called from constructors)
void DynamicalSystem::fillBoundaryConditionsFromXml()
{
  if (dsxml->getBoundaryConditionXML() != 0)
  {
    if (dsxml->getBoundaryConditionXML()->getType() == LINEARBC_TAG)
    {
      //  Linear BC
      BC = new LinearBC();
      static_cast<LinearBC*>(BC)->createBoundaryCondition(dsxml->getBoundaryConditionXML());
      isBCAllocatedIn = true;
    }
    else if (dsxml->getBoundaryConditionXML()->getType() == NON_LINEARBC_TAG)
    {
      // Non linear BC
      BC = new NLinearBC();
      static_cast<NLinearBC*>(BC)->createBoundaryCondition(dsxml->getBoundaryConditionXML());
      isBCAllocatedIn = true;
    }

    else if (dsxml->getBoundaryConditionXML()->getType() == PERIODICBC_TAG)
    {
      // Periodic BC
      BC = new PeriodicBC();
      static_cast<PeriodicBC*>(BC)->createBoundaryCondition(dsxml->getBoundaryConditionXML());
      isBCAllocatedIn = true;
    }
    else RuntimeException::selfThrow("DynamicalSystem::linkDynamicalSystemXML - bad kind of BoundaryCondition : " + dsxml->getBoundaryConditionXML()->getType());
  }
}

// DSIO built-in (called from constructors)
void DynamicalSystem::fillDsioFromXml()
{
  DSInputOutput *dsio;
  // get the numbers of DSIO
  vector<int> nbDSIOtab = dsxml->getDSInputOutputNumbers();
  unsigned int sizeTab = nbDSIOtab.size();
  dsioVector.resize(sizeTab, NULL);
  isDsioAllocatedIn.resize(sizeTab, false);
  for (unsigned int i = 0; i < sizeTab; i++)
  {
    if (dsxml->getDSInputOutputXML(nbDSIOtab[i])->getType() == LINEAR_DSIO_TAG)
    {
      // Linear DSIO
      dsioVector[i] = new LinearDSIO();
      isDsioAllocatedIn[i] = true;
      static_cast<LinearDSIO*>(dsio)->createDSInputOutput(dsxml->getDSInputOutputXML(nbDSIOtab[i]));
    }
    else if (dsxml->getDSInputOutputXML(nbDSIOtab[i])->getType() == NON_LINEAR_DSIO_TAG)
    {
      // Non linear DSIO
      dsioVector[i] = new DSInputOutput();
      isDsioAllocatedIn[i] = true;
      static_cast<DSInputOutput*>(dsio)->createDSInputOutput(dsxml->getDSInputOutputXML(nbDSIOtab[i]));
    }
    else if (dsxml->getDSInputOutputXML(nbDSIOtab[i])->getType() == LAGRANGIAN_DSIO_TAG)
    {
      // Lagrangian DSIO
      dsioVector[i] =  new LagrangianDSIO();
      isDsioAllocatedIn[i] = true;
      static_cast<LagrangianDSIO*>(dsio)->createDSInputOutput(dsxml->getDSInputOutputXML(nbDSIOtab[i]));
    }
    else if (dsxml->getDSInputOutputXML(nbDSIOtab[i])->getType() == LAGRANGIAN_LINEAR_DSIO_TAG)
    {
      // Linear lagrangian DSIO
      dsioVector[i] =  new LagrangianDSIO();
      isDsioAllocatedIn[i] = true;
      static_cast<LagrangianLinearDSIO*>(dsio)->createDSInputOutput(dsxml->getDSInputOutputXML(nbDSIOtab[i]));
    }
    else RuntimeException::selfThrow("DynamicalSystem::linkDynamicalSystemXML - bad kind of DSInputOutput: " + dsxml->getDSInputOutputXML(nbDSIOtab[i])->getType());
  }
}

// Setters

void DynamicalSystem::setX0(const SiconosVector& newValue)
{
  // check dimensions ...
  if (newValue.size() != n)
    RuntimeException::selfThrow("DynamicalSystem::setX0 - inconsistent sizes between x0 input and n - Maybe you forget to set n?");

  if (x0 != NULL)
    *x0 = newValue;

  else
  {
    if (newValue.isBlock())
      x0 = new BlockVector(newValue);
    else
      x0 = new SimpleVector(newValue);
    isXAllocatedIn[0] = true;
  }
}

void DynamicalSystem::setX0Ptr(SiconosVector* newPtr)
{
  // check dimensions ...
  if (newPtr->size() != n)
    RuntimeException::selfThrow("DynamicalSystem::setX0Ptr - inconsistent sizes between x0 input and n - Maybe you forget to set n?");

  if (isXAllocatedIn[0]) delete x0;
  x0 = newPtr;
  isXAllocatedIn[0] = false;
}

void DynamicalSystem::setX(const SiconosVector& newValue)
{
  // check dimensions ...
  if (newValue.size() != n)
    RuntimeException::selfThrow("DynamicalSystem::setX - inconsistent sizes between x0 input and n - Maybe you forget to set n?");

  if (x != NULL)
    *x = newValue;

  else
  {
    if (newValue.isBlock())
      x = new BlockVector(newValue);
    else
      x = new SimpleVector(newValue);
    isXAllocatedIn[1] = true;
  }
}

void DynamicalSystem::setXPtr(SiconosVector* newPtr)
{
  // check dimensions ...
  if (newPtr->size() != n)
    RuntimeException::selfThrow("DynamicalSystem::setXPtr - inconsistent sizes between x0 input and n - Maybe you forget to set n?");

  if (isXAllocatedIn[1]) delete x;
  x = newPtr;
  isXAllocatedIn[1] = false;
}

void DynamicalSystem::setXMemory(const SiconosMemory& newValue)
{
  if (xMemory != NULL)
  {
    if (newValue.getMemorySize() != xMemory->getMemorySize())
      RuntimeException::selfThrow("DynamicalSystem::setXMemory - inconsistent sizes between xMemory input and existing memorySize");
    else
      *xMemory = newValue;
  }
  else
  {
    xMemory = new SiconosMemory(newValue);
    isXAllocatedIn[2] = true;
  }
}

void DynamicalSystem::setXMemoryPtr(SiconosMemory * newPtr)
{
  if (isXAllocatedIn[2]) delete xMemory;
  xMemory = newPtr;
  isXAllocatedIn[2] = false;
}

void DynamicalSystem::setXDot(const SiconosVector& newValue)
{
  // check dimensions ...
  if (newValue.size() != n)
    RuntimeException::selfThrow("DynamicalSystem::setXDot - inconsistent sizes between x0 input and n - Maybe you forget to set n?");

  if (xDot != NULL)
    *xDot = newValue;

  else
  {
    if (newValue.isBlock())
      xDot = new BlockVector(newValue);
    else
      xDot = new SimpleVector(newValue);
    isXAllocatedIn[3] = true;
  }
}

void DynamicalSystem::setXDotPtr(SiconosVector* newPtr)
{
  // check dimensions ...
  if (newPtr->size() != n)
    RuntimeException::selfThrow("DynamicalSystem::setXDotPtr - inconsistent sizes between x0 input and n - Maybe you forget to set n?");

  if (isXAllocatedIn[3]) delete xDot;
  xDot = newPtr;
  isXAllocatedIn[3] = false;
}

void DynamicalSystem::setXDotMemory(const SiconosMemory& newValue)
{
  if (xDotMemory != NULL)
  {
    if (newValue.getMemorySize() != xDotMemory->getMemorySize())
      RuntimeException::selfThrow("DynamicalSystem::setXDotMemory - inconsistent sizes between xDotMemory input and existing memorySize");
    else
      *xDotMemory = newValue;
  }
  else
  {
    xDotMemory = new SiconosMemory(newValue);
    isXAllocatedIn[4] = true;
  }
}

void DynamicalSystem::setXDotMemoryPtr(SiconosMemory * newPtr)
{
  if (isXAllocatedIn[4]) delete xDotMemory;
  xDotMemory = newPtr;
  isXAllocatedIn[4] = false;
}

void DynamicalSystem::setXFree(const SiconosVector& newValue)
{
  // check dimensions ...
  if (newValue.size() != n)
    RuntimeException::selfThrow("DynamicalSystem::setXFree - inconsistent sizes between x0 input and n - Maybe you forget to set n?");

  if (xFree != NULL)
    *xFree = newValue;

  else
  {
    if (newValue.isBlock())
      xFree = new BlockVector(newValue);
    else
      xFree = new SimpleVector(newValue);
    isXAllocatedIn[5] = true;
  }
}

void DynamicalSystem::setXFreePtr(SiconosVector* newPtr)
{
  // check dimensions ...
  if (newPtr->size() != n)
    RuntimeException::selfThrow("DynamicalSystem::setXFreePtr - inconsistent sizes between x0 input and n - Maybe you forget to set n?");

  if (isXAllocatedIn[5]) delete xFree;
  xFree = newPtr;
  isXAllocatedIn[5] = false;
}

void DynamicalSystem::setR(const SiconosVector& newValue)
{
  // check dimensions ...
  if (newValue.size() != n)
    RuntimeException::selfThrow("DynamicalSystem::setR - inconsistent sizes between x0 input and n - Maybe you forget to set n?");

  if (r != NULL)
    *r = newValue;

  else
  {
    r = new SimpleVector(newValue);
    isRAllocatedIn[0] = true;
  }
}

void DynamicalSystem::setRPtr(SiconosVector *newPtr)
{
  // check dimensions ...
  if (newPtr->size() != n)
    RuntimeException::selfThrow("DynamicalSystem::setRPtr - inconsistent sizes between x0 input and n - Maybe you forget to set n?");

  if (isRAllocatedIn[0]) delete r;
  r = newPtr;
  isRAllocatedIn[0] = false;
}

void DynamicalSystem::setRMemory(const SiconosMemory& newValue)
{
  if (rMemory != NULL)
  {
    if (newValue.getMemorySize() != rMemory->getMemorySize())
      RuntimeException::selfThrow("DynamicalSystem::setRMemory - inconsistent sizes between rMemory input and existing memorySize");
    else
      *rMemory = newValue;
  }
  else
  {
    rMemory = new SiconosMemory(newValue);
    isRAllocatedIn[1] = true;
  }
}

void DynamicalSystem::setRMemoryPtr(SiconosMemory * newPtr)
{
  if (isRAllocatedIn[1]) delete rMemory;
  rMemory = newPtr;
  isRAllocatedIn[1] = false;
}

void DynamicalSystem::setJacobianX(const SiconosMatrix& newValue)
{
  // check dimensions ...
  if (newValue.size(0) != n || newValue.size(1) != n)
    RuntimeException::selfThrow("DynamicalSystem::setJacobianX - inconsistent sizes between jacobianX input and n - Maybe you forget to set n?");

  if (jacobianX != NULL)
    *jacobianX = newValue;

  else
  {
    jacobianX = new SimpleMatrix(newValue);
    isXAllocatedIn[6] = true;
  }
}

void DynamicalSystem::setJacobianXPtr(SiconosMatrix *newPtr)
{
  // check dimensions ...
  if (newPtr->size(0) != n || newPtr->size(1) != n)
    RuntimeException::selfThrow("DynamicalSystem::setJacobianXPtr - inconsistent sizes between jacobianX input and n - Maybe you forget to set n?");

  if (isXAllocatedIn[6]) delete jacobianX;
  jacobianX = newPtr;
  isXAllocatedIn[6] = false;
}

void  DynamicalSystem::setUSize(const unsigned int& newUSize)
{
  if (isControlAllocatedIn[0]) delete u;
  uSize = newUSize;
  u = new SimpleVector(uSize);
  isControlAllocatedIn[0] = true;
  u->zero();
}

// Three steps to set u:
//  - Check if uSize has been given (default value=0 in all constructors)
//  - Allocate memory for u, if necessary
//  - Set value for u
void DynamicalSystem::setU(const SiconosVector& newValue)
{
  if (uSize == 0 || newValue.size() != uSize)
    RuntimeException::selfThrow("DynamicalSystem::setU - inconsistent sizes between u input and uSize - Maybe you forget to set uSize?");
  if (u != NULL)
    *u = newValue;

  else
  {
    if (newValue.isBlock())
      u = new BlockVector(newValue);
    else
      u = new SimpleVector(newValue);
    isControlAllocatedIn[0] = true;
  }
}

void DynamicalSystem::setUPtr(SiconosVector* newPtr)
{
  if (uSize == 0 || newPtr->size() != uSize)
    RuntimeException::selfThrow("DynamicalSystem::setUPtr - inconsistent sizes between u input and uSize - Maybe you forget to set uSize?");
  // check dimensions ...

  if (isControlAllocatedIn[0]) delete u;
  u = newPtr;
  isControlAllocatedIn[0] = false;
}

void DynamicalSystem::setT(const SiconosMatrix& newValue)
{
  // check dimensions ...
  if (uSize == 0 || newValue.size(1) != uSize || newValue.size(0) != n)
    RuntimeException::selfThrow("DynamicalSystem::setT - inconsistent sizes between T input, uSize and/or n - Maybe you forget to set n or uSize?");

  if (T != NULL)
    *T = newValue;
  else
  {
    T = new SimpleMatrix(newValue);
    isControlAllocatedIn[1] = true;
  }
}

void DynamicalSystem::setTPtr(SiconosMatrix *newPtr)
{
  // check dimensions ...
  if (uSize == 0 || newPtr->size(1) != uSize || newPtr->size(0) != n)
    RuntimeException::selfThrow("DynamicalSystem::setTPtr - inconsistent sizes between T input, uSize and/or n - Maybe you forget to set n or uSize?");

  if (isControlAllocatedIn[1]) delete T;
  T = newPtr;
  isControlAllocatedIn[1] = false;
}

DSInputOutput* DynamicalSystem::getDSInputOutput(const unsigned int& i)
{
  if (i >= dsioVector.size())
    RuntimeException::selfThrow("DS - getDSInputOutput : \'i\' is out of range");
  return dsioVector[i];
}

void DynamicalSystem::setBoundaryConditionPtr(BoundaryCondition *newBC)
{
  if (isBCAllocatedIn) delete BC;
  BC = newBC;
  isBCAllocatedIn = false;
}

void DynamicalSystem::initialize(const double& time, const unsigned int& sizeOfMemory)
{

  // reset x to x0, xFree and r to zero.
  *x = *x0;
  xFree->zero();
  r->zero();

  // Initialize memory vectors
  initMemory(sizeOfMemory);

  // compute initial values for vectorField and jacobianX
  if (vectorFieldPtr == NULL)
    RuntimeException::selfThrow("vectorField() is not linked to a plugin function");
  if (computeJacobianXPtr == NULL)
    RuntimeException::selfThrow("computeJacobianX() is not linked to a plugin function");

  SimpleVector* param = parametersList0[0];
  vectorFieldPtr(n, &time, &(*x)(0) , &(*xDot)(0), &(*param)(0));

  param = parametersList0[1];
  computeJacobianXPtr(n, &time, &(*x)(0), &(*jacobianX)(0, 0), &(*param)(0));

  // If they are plugged, initialize u and T
  if (u != NULL && computeUPtr != NULL)
  {
    param = parametersList0[2];
    computeUPtr(uSize, n, &time, &(*x)(0), &(*u)(0), &(*param)(0));
  }

  if (T != NULL && computeTPtr != NULL)
  {
    param = parametersList0[3];
    computeTPtr(uSize, n, &(*x)(0), &(*T)(0, 0), &(*param)(0));
  }
}

// ===== MEMORY MANAGEMENT FUNCTIONS =====

void DynamicalSystem::initMemory(const unsigned int& steps)
{
  if (steps == 0)
    cout << "Warning : DynamicalSystem::initMemory with size equal to zero" << endl;
  else
  {
    stepsInMemory = steps;
    if (isXAllocatedIn[2]) delete xMemory;
    xMemory = new SiconosMemory(steps);
    isXAllocatedIn[2] = true;

    if (isXAllocatedIn[4]) delete xDotMemory;
    xDotMemory = new SiconosMemory(steps);
    isXAllocatedIn[4] = true;

    if (isRAllocatedIn[1]) delete rMemory;
    rMemory = new SiconosMemory(steps);
    isRAllocatedIn[1] = true;
  }
}

void DynamicalSystem::swapInMemory()
{
  xMemory->swap(*x);
  xDotMemory->swap(*xDot);
  rMemory->swap(*r);
}

// ===== COMPUTE PLUGINS FUNCTIONS =====

void DynamicalSystem::setVectorFieldFunction(const string& pluginPath, const string& functionName)
{
  if (xDot == NULL)
  {
    xDot = new SimpleVector(n);
    isXAllocatedIn[3] = true ;
  }

  vectorFieldPtr = NULL;
  cShared.setFunction(&vectorFieldPtr, pluginPath, functionName);
  string plugin;
  plugin = pluginPath.substr(0, pluginPath.length() - 3);
  vectorFieldFunctionName = plugin + ":" + functionName;
}

void DynamicalSystem::setComputeJacobianXFunction(const string& pluginPath, const string& functionName)
{
  if (jacobianX == NULL)
  {
    jacobianX = new SimpleMatrix(n, n);
    isXAllocatedIn[6] = true ;
  }

  computeJacobianXPtr = NULL;
  cShared.setFunction(&computeJacobianXPtr, pluginPath, functionName);

  string plugin;
  plugin = pluginPath.substr(0, pluginPath.length() - 3);
  computeJacobianXFunctionName = plugin + ":" + functionName;
}

void DynamicalSystem::setComputeUFunction(const string& pluginPath, const string& functionName)
{
  // since u is not allocated by default, memory must be reserved for it
  if (uSize == 0)
    RuntimeException::selfThrow("DynamicalSystem::setComputeUFunction - uSize is equal to 0 - Maybe you forget to set it?");

  if (u == NULL)
  {
    u = new SimpleVector(uSize);
    isControlAllocatedIn[0] = true;
    u->zero();
  }

  computeUPtr = NULL;
  cShared.setFunction(&computeUPtr, pluginPath, functionName);

  string plugin;
  plugin = pluginPath.substr(0, pluginPath.length() - 3);
  computeUFunctionName = plugin + ":" + functionName;
}

void DynamicalSystem::setComputeTFunction(const string& pluginPath, const string& functionName)
{
  // since T is not allocated by default, memory must be reserved for it
  if (uSize == 0)
    RuntimeException::selfThrow("DynamicalSystem::setComputeUFunction - uSize is equal to 0 - Maybe you forget to set it?");

  if (T == NULL)
  {
    T = new SimpleMatrix(n, uSize);
    isControlAllocatedIn[1] = true;
    T->zero();
  }

  computeTPtr = NULL;
  cShared.setFunction(&computeTPtr, pluginPath, functionName);

  string plugin;
  plugin = pluginPath.substr(0, pluginPath.length() - 3);
  computeTFunctionName = plugin + ":" + functionName;
}

void DynamicalSystem::setParametersListVector(const std::vector<SimpleVector*>& newVector)
{
  // copy!!
  for (unsigned int i = 0; i < parametersList0.size(); ++i)
  {
    if (isParametersList0AllocatedIn[i]) delete parametersList0[i];
    *(parametersList0[i]) = *(newVector[i]);
    isParametersList0AllocatedIn[i] = true;
  }
}

void DynamicalSystem::setParametersList(const SimpleVector& newValue, const unsigned int & index)
{
  if (isParametersList0AllocatedIn[index]) delete parametersList0[index];
  parametersList0[index] = new SimpleVector(newValue);
  isParametersList0AllocatedIn[index] = true;
}

void DynamicalSystem::setParametersListPtr(SimpleVector *newPtr, const unsigned int & index)
{
  if (isParametersList0AllocatedIn[index]) delete parametersList0[index];
  parametersList0[index] = newPtr;
  isParametersList0AllocatedIn[index] = false;
}

void DynamicalSystem::computeVectorField(const double& time)
{
  if (vectorFieldPtr == NULL)
    RuntimeException::selfThrow("vectorField() is not linked to a plugin function");

  unsigned int size = x->size();
  SimpleVector* param = parametersList0[0];
  vectorFieldPtr(size, &time, &(*x)(0) , &(*xDot)(0), &(*param)(0));
}

void DynamicalSystem::computeJacobianX(const double& time)
{
  if (computeJacobianXPtr == NULL)
    RuntimeException::selfThrow("computeJacobianX() is not linked to a plugin function");

  unsigned int size = x->size();
  SimpleVector* param = parametersList0[1];
  computeJacobianXPtr(size, &time, &(*x)(0), &(*jacobianX)(0, 0), &(*param)(0));
}

void DynamicalSystem::computeU(const double& time)
{
  if (computeUPtr == NULL)
    RuntimeException::selfThrow("computeU() is not linked to a plugin function");
  if (u == NULL)
    RuntimeException::selfThrow("computeU(), warning: u = NULL");


  unsigned int sizeX = x->size();
  SimpleVector* param = parametersList0[2];
  computeUPtr(uSize, sizeX, &time, &(*x)(0), &(*u)(0), &(*param)(0));
}

void DynamicalSystem::computeU(const double& time, SiconosVector* xx)
{
  if (computeUPtr == NULL)
    RuntimeException::selfThrow("computeU() is not linked to a plugin function");
  if (u == NULL)
    RuntimeException::selfThrow("computeU(), warning: u = NULL");

  unsigned int sizeX = xx->size();
  SimpleVector* param = parametersList0[2];
  computeUPtr(uSize, sizeX, &time, &(*xx)(0), &(*u)(0), &(*param)(0));
}

void DynamicalSystem::computeT()
{
  if (computeTPtr == NULL)
    RuntimeException::selfThrow("computeT() is not linked to a plugin function");
  if (T == NULL)
    RuntimeException::selfThrow("computeT(), warning: T = NULL");

  unsigned int sizeX = x->size();
  SimpleVector* param = parametersList0[3];
  computeTPtr(uSize, sizeX, &(*x)(0), &(*T)(0, 0), &(*param)(0));
}

// ===== XML MANAGEMENT FUNCTIONS =====

void DynamicalSystem::saveDSToXML()
{
  // --- general DS data ---
  saveDSDataToXML();

  // --- other data ---
  if (dsxml != NULL)
  {
    dsxml->setN(n);
    dsxml->setVectorFieldPlugin(vectorFieldFunctionName);
    if (computeJacobianXFunctionName != "")
      dsxml->setComputeJacobianXPlugin(computeJacobianXFunctionName);
    else
      dsxml->setComputeJacobianXPlugin("DefaultPlugin:computeJacobianX");
  }
  else RuntimeException::selfThrow("DynamicalSystem::saveDSToXML - The DynamicalSystemXML object doesn't exists");
}

// Save data common to each system into the xml file
void DynamicalSystem::saveDSDataToXML()
{
  //--- Boundary conditions ---
  saveBCToXML();
  // --- DS input-output ---
  saveDSIOToXML();
  if (dsxml != NULL)
  {
    dsxml->setId(id);
    dsxml->setX0(x0);
    dsxml->setX(x);
    dsxml->setXMemory(xMemory);
    dsxml->setXDot(xDot);
    dsxml->setXDotMemory(xDotMemory);
    dsxml->setStepsInMemory(stepsInMemory);
    dsxml->setR(r);
  }
  else RuntimeException::selfThrow("DynamicalSystem::saveDSToXML - The DynamicalSystemXML object doesn't exists");
}

// Save boundary conditions to xml file
void DynamicalSystem::saveBCToXML()
{
  if (BC != NULL)
  {
    if (BC->getType() == LINEARBC)
      (static_cast<LinearBC*>(BC))->saveBCToXML();
    else if (BC->getType() == NLINEARBC)
      (static_cast<NLinearBC*>(BC))->saveBCToXML();
    else if (BC->getType() == PERIODICBC)
      (static_cast<PeriodicBC*>(BC))->saveBCToXML();
    else RuntimeException::selfThrow("DynamicalSystem::saveDSToXML - bad kind of BoundaryCondition");
  }
}

// Save DS Input-Output to xml file
void DynamicalSystem::saveDSIOToXML()
{
  if (dsioVector.size() != 0)
  {
    for (unsigned int i = 0; i < dsioVector.size(); i++)
    {
      if (dsioVector[i]->getType() == LINEARDSIO)
        (static_cast<LinearDSIO*>(dsioVector[i]))->saveDSInputOutputToXML();
      else if (dsioVector[i]->getType() == NLINEARDSIO)
        (static_cast<DSInputOutput*>(dsioVector[i]))->saveDSInputOutputToXML();
      else if (dsioVector[i]->getType() == LAGRANGIANDSIO)
        (static_cast<LagrangianDSIO*>(dsioVector[i]))->saveDSInputOutputToXML();
      else RuntimeException::selfThrow("DynamicalSystem::saveDSToXML - bad kind of DSInputOuput");
    }
  }
}

// ===== MISCELLANEOUS ====

void DynamicalSystem::display() const
{
  cout << " ===== General dynamical system display =====" << endl;
  cout << "- number : " << number << endl;
  cout << "- id : " << id << endl;
  cout << "- n (size) : " << n << endl;
  cout << "- x " << endl;
  if (x != NULL) x->display();
  else cout << "-> NULL" << endl;
  cout << "- x0 " << endl;
  if (x0 != NULL) x0->display();
  else cout << "-> NULL" << endl;
  cout << "- xFree " << endl;
  if (xFree != NULL) xFree->display();
  else cout << "-> NULL" << endl;
  cout << "- xDot " << endl;
  if (xDot != NULL) xDot->display();
  else cout << "-> NULL" << endl;
  cout << "- stepsInMemory : " << stepsInMemory << endl;
  cout << "- r " << endl;
  if (r != NULL) r->display();
  else cout << "-> NULL" << endl;
  cout << "- u " << endl;
  if (u != NULL) u->display();
  else cout << "-> NULL" << endl;
  cout << "- T " << endl;
  if (T != NULL) T->display();
  else cout << "-> NULL" << endl;
  //  cout<<"- VectorField plugin: "<<vectorFieldFunctionName <<endl;
  //  cout<<"- JacobianX plugin: "<<computeJacobianXFunctionName <<endl;
  cout << " ============================================" << endl;
}

double DynamicalSystem::dsConvergenceIndicator()
{
  RuntimeException::selfThrow("DynamicalSystem:dsConvergenceIndicator - not yet implemented for Dynamical system type :" + DSType);
  return 0;
}

