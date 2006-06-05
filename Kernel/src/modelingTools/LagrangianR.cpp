/* Siconos-Kernel version 1.2.0, Copyright INRIA 2005-2006.
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

// \todo : create a work vector for all tmp vectors used in computeG, computeH ...

#include "LagrangianR.h"
using namespace std;

// Default constructor with optional interaction parameter
LagrangianR::LagrangianR(Interaction* inter):
  Relation(inter), LagrangianRelationType("default"), isHPlugged(false), hFunctionName("none"),
  h0Ptr(NULL), G0Ptr(NULL), h1Ptr(NULL), G10Ptr(NULL), G11Ptr(NULL), h2Ptr(NULL), G20Ptr(NULL), G21Ptr(NULL)
{
  isOutputPlugged = false;
  isInputPlugged  = false;
  relationType = LAGRANGIANRELATION;
  isHPlugged = false ;
}

// xml constructor
LagrangianR::LagrangianR(RelationXML* relxml, Interaction* inter):
  Relation(relxml, inter), LagrangianRelationType("none"), isHPlugged(true), hFunctionName("none"),
  h0Ptr(NULL), G0Ptr(NULL), h1Ptr(NULL), G10Ptr(NULL), G11Ptr(NULL), h2Ptr(NULL), G20Ptr(NULL), G21Ptr(NULL)
{

  if (isOutputPlugged || isInputPlugged)
    RuntimeException::selfThrow("LagrangianR:: xml constructor, input and output plug-in must not be set for this kind of relation");

  relationType = LAGRANGIANRELATION;
  LagrangianRXML * LRxml = static_cast<LagrangianRXML *>(relationxml);
  string plugin;
  // problem type
  LagrangianRelationType = LRxml->getLagrangianType();

  // h plug-in
  if (LRxml->hasH())
  {
    plugin = LRxml->getHPlugin();
    setComputeHFunction(cShared.getPluginName(plugin), cShared.getPluginFunctionName(plugin));
  }
  else
  {
    // This case corresponds to LagrangianLinearR, else h is required !!
    setComputeHFunction("DefaultPlugin.so", "h0");
    isHPlugged = false ;
  }

  if (LRxml->hasG())
  {
    if (LagrangianRelationType == "scleronomic")
    {
      isGPlugged.push_back(true);
      G.reserve(1);
      G.push_back(NULL);
      isAllocatedIn["G0"] = false ;
      GFunctionName.reserve(1);
      GFunctionName.push_back("none");

      if (LRxml->isGPlugin())
      {
        plugin = LRxml->getGPlugin();
        setComputeGFunction(cShared.getPluginName(plugin), cShared.getPluginFunctionName(plugin));
      }
      else
      {
        G[0] = new SimpleMatrix(LRxml->getGMatrix());
        isAllocatedIn["G0"] = true   ;
        isGPlugged[0] = false;
      }
    }
    else if (LagrangianRelationType == "rhenomorous" || LagrangianRelationType == "scleronomic+lambda")
    {
      isGPlugged.resize(2, true);
      G.reserve(2);
      G.push_back(NULL);
      G.push_back(NULL);
      isAllocatedIn["G0"] = false   ;
      isAllocatedIn["G1"] = false   ;
      GFunctionName.reserve(2);
      GFunctionName.push_back("none");
      GFunctionName.push_back("none");
      if (LRxml->isGPlugin())
      {
        plugin = LRxml->getGPlugin(0);
        setComputeGFunction(cShared.getPluginName(plugin), cShared.getPluginFunctionName(plugin), 0);
        plugin = LRxml->getGPlugin(1);
        setComputeGFunction(cShared.getPluginName(plugin), cShared.getPluginFunctionName(plugin), 1);
      }
      else
      {
        G[0] = new SimpleMatrix(LRxml->getGMatrix(0));
        G[1] = new SimpleMatrix(LRxml->getGMatrix(1));
        isAllocatedIn["G0"] = true   ;
        isAllocatedIn["G1"] = true   ;
        isGPlugged[0] = false;
        isGPlugged[1] = false;
      }
    }
    else
      cout << "LagrangianR xml constructor: warning, xml loading not yet implemented for G for this kind of problem.\n You should fix it by yourself using set functions" << endl;
  }
  else
  {
    isGPlugged.push_back(false);
    if (LagrangianRelationType == "scleronomic")
    {
      G.reserve(1);
      G.push_back(NULL);
      isAllocatedIn["G0"] = false   ;
      GFunctionName.reserve(1);
      GFunctionName.push_back("none");
      setComputeGFunction("DefaultPlugin.so", "G0");
      isGPlugged[0] = false;
    }
    else if (LagrangianRelationType == "rhenomorous" || LagrangianRelationType == "scleronomic+lambda")
    {
      G.reserve(2);
      G.push_back(NULL);
      G.push_back(NULL);
      isAllocatedIn["G0"] = false   ;
      isAllocatedIn["G1"] = false   ;
      GFunctionName.reserve(2);
      GFunctionName.push_back("none");
      GFunctionName.push_back("none");
      if (LagrangianRelationType == "rhenomorous")
      {
        setComputeGFunction("DefaultPlugin.so", "G10");
        setComputeGFunction("DefaultPlugin.so", "G11");
      }
      else
      {
        setComputeGFunction("DefaultPlugin.so", "G20");
        setComputeGFunction("DefaultPlugin.so", "G21");
      }
      isGPlugged.push_back(false);
      isGPlugged.push_back(false);
    }
  }

  // check or set sizes for G
  if (inter != NULL) // else this will be done during setInteractionPtr call by user
    manageGMemory();
}

// constructor from a set of data
LagrangianR::LagrangianR(const string& lagRelType, const string& computeH, const vector<string> & computeG, Interaction* inter):
  Relation(inter), LagrangianRelationType(lagRelType), isHPlugged(true), hFunctionName("none"),
  h0Ptr(NULL), G0Ptr(NULL), h1Ptr(NULL), G10Ptr(NULL), G11Ptr(NULL), h2Ptr(NULL), G20Ptr(NULL), G21Ptr(NULL)
{

  relationType = LAGRANGIANRELATION;
  isOutputPlugged = false;
  isInputPlugged  = false;

  if (LagrangianRelationType == "scleronomic")
  {
    isGPlugged.push_back(true);
    G.reserve(1);
    G.push_back(NULL);
    isAllocatedIn["G0"] = false   ;
    GFunctionName.reserve(1);
    GFunctionName.push_back("none");
  }
  else if (LagrangianRelationType == "rhenomorous" || LagrangianRelationType == "scleronomic+lambda")
  {
    isGPlugged.push_back(true);
    isGPlugged.push_back(true);
    G.reserve(2);
    G.push_back(NULL);
    G.push_back(NULL);
    isAllocatedIn["G0"] = false   ;
    isAllocatedIn["G1"] = false   ;
    GFunctionName.reserve(1);
    GFunctionName.push_back("none");
    GFunctionName.push_back("none");
  }

  // === Set plug-in for h and G functions
  // h
  setComputeHFunction(cShared.getPluginName(computeH), cShared.getPluginFunctionName(computeH));
  // G0
  setComputeGFunction(cShared.getPluginName(computeG[0]), cShared.getPluginFunctionName(computeG[0]), 0);
  // G1 if necessary
  if (LagrangianRelationType == "rhenomorous" || LagrangianRelationType == "scleronomic+lambda")
    setComputeGFunction(cShared.getPluginName(computeG[1]), cShared.getPluginFunctionName(computeG[1]), 1);
  // Memory allocation for G
  if (inter != NULL)
    manageGMemory();
}

// copy constructor (inter is optional)
LagrangianR::LagrangianR(const Relation & newLNLR, Interaction* inter):
  Relation(newLNLR, inter), LagrangianRelationType("none"), isHPlugged(true), hFunctionName("none"),
  h0Ptr(NULL), G0Ptr(NULL), h1Ptr(NULL), G10Ptr(NULL), G11Ptr(NULL)
{
  if (relationType !=  LAGRANGIANRELATION && relationType !=  LAGRANGIANLINEARRELATION)
    RuntimeException::selfThrow("LagrangianR:: copy constructor, inconsistent relation types for copy");

  isOutputPlugged = false;
  isInputPlugged  = false;

  const LagrangianR * lnlr = static_cast<const LagrangianR*>(&newLNLR);
  LagrangianRelationType = lnlr->getLagrangianRelationType();
  setParameters(newLNLR.getParameters());   // Copy !!
  string plugin;
  // --- h ---
  string hPluginName = lnlr->getHFunctionName();
  setComputeHFunction(cShared.getPluginName(hPluginName), cShared.getPluginFunctionName(hPluginName));
  if (cShared.getPluginName(hPluginName) == "DefaultPlugin.so")
    isHPlugged = false;


  // --- G ---
  if (LagrangianRelationType == "scleronomic")
  {
    isGPlugged.push_back(true);
    G.reserve(1);
    //      G.push_back(NULL);
    isAllocatedIn["G0"] = false   ;
    GFunctionName.reserve(1);
    GFunctionName.push_back("none");
    if (lnlr->getGPtr() != NULL)
    {
      G.push_back(new SimpleMatrix(lnlr->getG()));
      isAllocatedIn["G0"] = true   ;
    }
    else
      G.push_back(NULL);
    plugin = lnlr->getGFunctionName();
    setComputeGFunction(cShared.getPluginName(plugin), cShared.getPluginFunctionName(plugin), 0);
    if (cShared.getPluginName(plugin) == "DefaultPlugin.so")
      isGPlugged[0] = false;
  }

  else if (LagrangianRelationType == "rhenomorous" || LagrangianRelationType == "scleronomic+lambda")
  {
    isGPlugged.push_back(true);
    isGPlugged.push_back(true);
    G.reserve(2);
    G.push_back(NULL);
    G.push_back(NULL);
    isAllocatedIn["G0"] = false   ;
    isAllocatedIn["G1"] = false   ;
    GFunctionName.reserve(1);
    GFunctionName.push_back("none");
    GFunctionName.push_back("none");
    if (lnlr->getGPtr(0) != NULL)
    {
      G[0] = new SimpleMatrix(lnlr->getG(0));
      isAllocatedIn["G0"] = true   ;
    }
    plugin = lnlr->getGFunctionName(0);
    setComputeGFunction(cShared.getPluginName(plugin), cShared.getPluginFunctionName(plugin), 0);
    if (cShared.getPluginName(plugin) == "DefaultPlugin.so")
      isGPlugged[0] = false;

    if (lnlr->getGPtr(1) != NULL)
    {
      G[1] = new SimpleMatrix(lnlr->getG(1));
      isAllocatedIn["G1"] = true   ;
      isGPlugged[1] = false;
    }
    plugin = lnlr->getGFunctionName(1);
    setComputeGFunction(cShared.getPluginName(plugin), cShared.getPluginFunctionName(plugin), 1);
    if (cShared.getPluginName(plugin) == "DefaultPlugin.so")
      isGPlugged[0] = false;
  }
  else
    RuntimeException::selfThrow("LagrangianR:: copy constructor, not yet implemented for problem of type" + LagrangianRelationType);

  if (interaction != NULL)
    manageGMemory();
}

LagrangianR::~LagrangianR()
{
  stringstream sstr;
  string tmp, alloc;
  for (unsigned int i = 0; i < G.size(); i++)
  {
    sstr << i  ;
    sstr >> tmp;
    alloc = "G" + tmp;
    if (isAllocatedIn[alloc]) delete G[i];
    G[i] = NULL;
  }

  h0Ptr = NULL;
  h1Ptr = NULL;
  G0Ptr = NULL;
  G10Ptr = NULL;
  G11Ptr = NULL;
  h2Ptr = NULL;
  G20Ptr = NULL;
  G21Ptr = NULL;
}

void LagrangianR::manageGMemory()
{
  if (interaction != NULL)
  {
    unsigned int sizeY = interaction->getInteractionSize();
    unsigned int sizeQ = interaction->getSizeOfDS();
    unsigned int size0, size1;

    if (G[0] == NULL)
    {
      G[0] = new SimpleMatrix(sizeY, sizeQ);
      isAllocatedIn["G0"] = true;
    }
    else
    {
      size0 = G[0]->size(0);
      size1 = G[0]->size(1);
      if (size0 != sizeY || size1 != sizeQ)
        RuntimeException::selfThrow("LagrangianR:: setInteractionPtr, inconsistent sizes for G[0] with interaction dimensions.");
    }

    if (LagrangianRelationType == "rhenomorous")
    {
      if (G[1] == NULL)
      {
        G[1] = new SimpleMatrix(sizeY, 1);
        isAllocatedIn["G1"] = true ;
      }
      else
      {
        size0 = G[1]->size(0);
        size1 = G[1]->size(1);
        if (size0 != sizeY || size1 != 1)
          RuntimeException::selfThrow("LagrangianR:: setInteractionPtr, inconsistent sizes for G[1] with interaction dimensions.");
      }
    }
    if (LagrangianRelationType == "scleronomic+lambda")
    {
      if (G[1] == NULL)
      {
        G[1] = new SimpleMatrix(sizeY, sizeY);
        isAllocatedIn["G1"] = true ;
      }
      else
      {
        size0 = G[1]->size(0);
        size1 = G[1]->size(1);
        if (size0 != sizeY || size1 != sizeY)
          RuntimeException::selfThrow("LagrangianR:: setInteractionPtr, inconsistent sizes for G[1] with interaction dimensions.");
      }
    }
  }
  else
    cout << "LagrangianR::manageGMemory, no interaction linked to the current relation, can not set size for G" << endl;
}

void LagrangianR::setInteractionPtr(Interaction* newInter)
{
  interaction = newInter;
  if (interaction != NULL)
    manageGMemory();
  else
    RuntimeException::selfThrow("LagrangianR::setInteractionPtr, input interaction ==NULL ");
}

void LagrangianR::setLagrangianRelationType(const string & type)
{
  // Warning: this function reset G to NULL !!
  stringstream sstr;
  string tmp, alloc;
  for (unsigned int i = 0; i < G.size(); i++)
  {
    sstr << i  ;
    sstr >> tmp;
    alloc = "G" + tmp;
    if (isAllocatedIn[alloc]) delete G[i];
    G[i] = NULL;
  }

  LagrangianRelationType = type;
  if (LagrangianRelationType == "scleronomic")
  {
    isGPlugged.resize(1, false);
    G.resize(1, NULL);
    isAllocatedIn["G0"] = false;
    GFunctionName.resize(1, "none");
    setComputeGFunction("DefaultPlugin.so", "G0");
    isGPlugged[0] = false;
    setComputeHFunction("DefaultPlugin.so", "h0");
    isHPlugged = false;
  }
  else if (LagrangianRelationType == "rhenomorous" || LagrangianRelationType == "scleronomic+lambda")
  {
    isGPlugged.resize(2, false);
    G.resize(2, NULL);
    isAllocatedIn["G0"] = false;
    isAllocatedIn["G1"] = false;
    GFunctionName.resize(2, "none");
  }
  else
    RuntimeException::selfThrow("LagrangianR:: copy constructor, not yet implemented for problem of type" + LagrangianRelationType);

  if (interaction != NULL)
    manageGMemory();
}

void LagrangianR::setGVector(const vector<SiconosMatrix*>& newVector)
{
  if (interaction == NULL)
    RuntimeException::selfThrow("LagrangianR:: setGVector, no interaction linked to the current relation");

  unsigned int sizeY = interaction->getInteractionSize();
  unsigned int sizeQ = interaction->getSizeOfDS();

  stringstream sstr;
  string tmp, alloc;
  for (unsigned int i = 0; i < G.size(); i++)
  {
    sstr << i  ;
    sstr >> tmp;
    alloc = "G" + tmp;
    if (isAllocatedIn[alloc]) delete G[i];
    G[i] = NULL;
  }

  G.resize(newVector.size(), NULL);
  if (LagrangianRelationType == "scleronomic+lambda")
  {
    if (G.size() != 2)
      RuntimeException::selfThrow("LagrangianR - setGVector: wrong size for G vector in scleronomic+lambda case");
    if (newVector[0]->size(0) != sizeQ || newVector[0]->size(1) != sizeY)
      RuntimeException::selfThrow("LagrangianR - setGVector: inconsistent input matrix size ");
    G[0] = new SimpleMatrix(*(newVector[0]));
    isAllocatedIn["G0"] = true;
    if (newVector[1]->size(0) != sizeY || newVector[0]->size(1) != sizeY)
      RuntimeException::selfThrow("LagrangianR - setGVector: inconsistent input matrix size ");
    G[1] = new SimpleMatrix(*(newVector[1]));
    isAllocatedIn["G1"] = true;
  }
  else
  {
    for (unsigned int i = 0; i < G.size(); i++)
    {
      if (newVector[i] != NULL)
      {
        if (newVector[i]->size(0) != sizeQ || newVector[i]->size(1) != sizeY)
          RuntimeException::selfThrow("LagrangianR - setGVector: inconsistent input matrix size ");
        G[i] = new SimpleMatrix(*(newVector[i]));
        sstr << i  ;
        sstr >> tmp;
        alloc = "G" + tmp;
        isAllocatedIn[alloc] = true;
      }
    }
  }
}

void LagrangianR::setG(const SiconosMatrix& newValue, const unsigned int & index)
{
  if (interaction == NULL)
    RuntimeException::selfThrow("LagrangianR:: setG, no interaction linked to the current relation");
  unsigned int sizeY = interaction->getInteractionSize();
  unsigned int sizeQ = interaction->getSizeOfDS();
  if (LagrangianRelationType == "scleronomic+lambda" && index == 1)
  {
    if (newValue.size(1) != sizeY || newValue.size(0) != sizeY)
      RuntimeException::selfThrow("LagrangianR - setG: inconsistent input matrix size ");
  }
  else
  {
    if (newValue.size(1) != sizeQ || newValue.size(0) != sizeY)
      RuntimeException::selfThrow("LagrangianR - setG: inconsistent input matrix size ");
  }

  if (index >= G.size())
    RuntimeException::selfThrow("LagrangianR:: setG(mat,index), index out of range. Use rather setGVector?");

  if (G[index] == NULL)
  {
    stringstream sstr;
    string tmp, alloc;
    G[index] = new SimpleMatrix(newValue);
    sstr << index  ;
    sstr >> tmp;
    alloc = "G" + tmp;
    isAllocatedIn[alloc] = true;
  }
  else
    *(G[index]) = newValue;
  isGPlugged[index] = false;
}

void LagrangianR::setGPtr(SiconosMatrix *newPtr, const unsigned int & index)
{
  if (interaction == NULL)
    RuntimeException::selfThrow("LagrangianR:: setGPtr, no interaction linked to the current relation");
  unsigned int sizeY = interaction->getInteractionSize();
  unsigned int sizeQ = interaction->getSizeOfDS();
  if (LagrangianRelationType == "scleronomic+lambda" && index == 1)
  {
    if (newPtr->size(1) != sizeY || newPtr->size(0) != sizeY)
      RuntimeException::selfThrow("LagrangianR - setGPtr: inconsistent input matrix size ");
  }
  else
  {
    if (newPtr->size(1) != sizeQ || newPtr->size(0) != sizeY)
      RuntimeException::selfThrow("LagrangianR - setGPtr: inconsistent input matrix size ");
  }

  if (index >= G.size())
    RuntimeException::selfThrow("LagrangianR:: setGPtr(mat,index), index out of range. Use rather setGVector?");

  stringstream sstr;
  string tmp, alloc;
  sstr << index  ;
  sstr >> tmp;
  alloc = "G" + tmp;
  if (isAllocatedIn[alloc]) delete G[index];
  G[index] = newPtr;
  isAllocatedIn[alloc] = false;
  isGPlugged[index] = false;
}

void LagrangianR::setComputeHFunction(const string& pluginPath, const string& functionName)
{
  if (LagrangianRelationType == "scleronomic")
    cShared.setFunction(&h0Ptr, pluginPath, functionName);
  else if (LagrangianRelationType == "rhenomorous")
    cShared.setFunction(&h1Ptr, pluginPath, functionName);
  else if (LagrangianRelationType == "scleronomic+lambda")
    cShared.setFunction(&h2Ptr, pluginPath, functionName);
  else
    RuntimeException::selfThrow("LagrangianR:: setComputeHFunction,  not yet implemented for this type of constraints");

  initParameter("h");

  string plugin;
  plugin = pluginPath.substr(0, pluginPath.length() - 3);
  hFunctionName = plugin + ":" + functionName;
  isHPlugged = true;
}

void LagrangianR::setComputeGFunction(const string& pluginPath, const string& functionName, const unsigned int & index)
{
  if (index >= G.size())
    RuntimeException::selfThrow("LagrangianR:: setComputeGFunction, index out of range. Use rather setGVector?");

  if (LagrangianRelationType == "scleronomic")
  {
    cShared.setFunction(&G0Ptr, pluginPath, functionName);
    initParameter("G0");
  }
  else if (LagrangianRelationType == "rhenomorous")
  {
    if (index == 0)
    {
      cShared.setFunction(&G10Ptr, pluginPath, functionName);
      initParameter("G10");
    }
    else if (index == 1)
    {
      cShared.setFunction(&G11Ptr, pluginPath, functionName);
      initParameter("G11");
    }
    else
      RuntimeException::selfThrow("LagrangianR:: setComputeGFunction, index out of range");
  }
  else if (LagrangianRelationType == "scleronomic+lambda")
  {
    if (index == 0)
    {
      cShared.setFunction(&G20Ptr, pluginPath, functionName);
      initParameter("G20");
    }
    else if (index == 1)
    {
      cShared.setFunction(&G21Ptr, pluginPath, functionName);
      initParameter("G21");
    }
    else
      RuntimeException::selfThrow("LagrangianR:: setComputeGFunction, index out of range");
  }
  else
    RuntimeException::selfThrow("LagrangianR:: setComputeGFunction,  not yet implemented for this type of constraints");

  string plugin;
  plugin = pluginPath.substr(0, pluginPath.length() - 3);
  GFunctionName[index] = plugin + ":" + functionName;
  isGPlugged[index] = true;
}

void LagrangianR::computeH(const double& time)
{
  if (interaction == NULL)
    RuntimeException::selfThrow("LagrangianR:computeH(): no interaction linked with relation");

  unsigned int sizeY = interaction->getInteractionSize();
  unsigned int sizeQ = interaction->getSizeOfDS();

  // Get the DS concerned by the interaction of this relation
  DSSet vDS = interaction->getDynamicalSystems();
  DSIterator it;
  string type;
  BlockVector *qTmp = new BlockVector();

  LagrangianDS* lds;
  for (it = vDS.begin(); it != vDS.end(); ++it)
  {
    type = (*it)->getType();
    // check dynamical system type
    if (type != LTIDS && type != LNLDS)
      RuntimeException::selfThrow("LagrangianLinearR::computeOutput not yet implemented for dynamical system of type: " + type);

    // convert vDS systems into LagrangianDS and put them in vLDS
    lds = static_cast<LagrangianDS*>(*it);

    // Put q of each DS into a block
    // Warning: use copy constructors (add function), no link between pointers
    qTmp->add(lds->getQ());
  }

  // get vector y of the current interaction
  SimpleVector *y = interaction->getYPtr(0);
  SimpleVector *lambda = interaction->getLambdaPtr(0);

  SimpleVector* param = parametersList["h"];

  if (LagrangianRelationType == "scleronomic")
  {
    if (h0Ptr == NULL)
      RuntimeException::selfThrow("LagrangianR:computeH() is not linked to a plugin function");
    h0Ptr(sizeQ, &(*qTmp)(0) , sizeY, &(*y)(0), &(*param)(0));
  }
  else if (LagrangianRelationType == "rhenomorous")
  {
    if (h1Ptr == NULL)
      RuntimeException::selfThrow("LagrangianR:computeH() is not linked to a plugin function");
    h1Ptr(sizeQ, &(*qTmp)(0), &time, sizeY,  &(*y)(0), &(*param)(0));
  }
  else if (LagrangianRelationType == "scleronomic+lambda")
  {
    if (h2Ptr == NULL)
      RuntimeException::selfThrow("LagrangianR:computeH() is not linked to a plugin function");
    h2Ptr(sizeQ, &(*qTmp)(0), &(*lambda)(0), sizeY,  &(*y)(0), &(*param)(0));
  }
  else
    RuntimeException::selfThrow("LagrangianR::computeH,  not yet implemented for this type of constraints");
  delete qTmp;
}

void LagrangianR::computeG(const double & time, const unsigned int & index)
{

  if (index >= G.size())
    RuntimeException::selfThrow("LagrangianR:: computeG(), index out of range. Use rather setGVector?");
  if (interaction == NULL)
    RuntimeException::selfThrow("LagrangianR:computeG(): no interaction linked with relation");

  unsigned int sizeY = interaction->getInteractionSize();
  unsigned int sizeQ = interaction->getSizeOfDS();
  // Get the DS concerned by the interaction of this relation
  DSSet vDS = interaction->getDynamicalSystems();
  DSIterator it;
  string type;
  BlockVector *qTmp = new BlockVector();

  LagrangianDS* lds;
  for (it = vDS.begin(); it != vDS.end(); ++it)
  {
    type = (*it)->getType();
    // check dynamical system type
    if (type != LTIDS && type != LNLDS)
      RuntimeException::selfThrow("LagrangianLinearR::computeG not yet implemented for dynamical system of type: " + type);

    // convert vDS systems into LagrangianDS and put them in vLDS
    lds = static_cast<LagrangianDS*>(*it);

    // Put q of each DS into a block
    // Warning: use copy constructors (add function), no link between pointers
    qTmp->add(lds->getQ());
  }

  // get vector lambda of the current interaction
  SimpleVector *lambda = interaction->getLambdaPtr(0);
  SimpleVector* param;

  if (LagrangianRelationType == "scleronomic")
  {
    param = parametersList["G0"];
    SiconosMatrix * Gtmp = G[0];
    if (G0Ptr == NULL)
      RuntimeException::selfThrow("computeG() is not linked to a plugin function");
    G0Ptr(sizeQ, &(*qTmp)(0), sizeY, &(*Gtmp)(0, 0), &(*param)(0));
  }
  else if (LagrangianRelationType == "rhenomorous")
  {
    SiconosMatrix * Gtmp = G[index];
    if (index == 0)
    {
      if (G10Ptr == NULL)
        RuntimeException::selfThrow("computeG() is not linked to a plugin function");
      param = parametersList["G10"];
      G10Ptr(sizeQ, &(*qTmp)(0), &time, sizeY, &(*Gtmp)(0, 0), &(*param)(0));
    }
    else if (index == 1)
    {
      if (G11Ptr == NULL)
        RuntimeException::selfThrow("computeG() is not linked to a plugin function");
      param = parametersList["G11"];
      G11Ptr(sizeQ, &(*qTmp)(0), &time, sizeY, &(*Gtmp)(0, 0), &(*param)(0));
    }
    else
      RuntimeException::selfThrow("LagrangianR::computeG, index out of range");
  }
  else if (LagrangianRelationType == "scleronomic+lambda")
  {
    SiconosMatrix * Gtmp = G[index];
    if (index == 0)
    {
      if (G20Ptr == NULL)
        RuntimeException::selfThrow("computeG() is not linked to a plugin function");
      param = parametersList["G20"];
      G20Ptr(sizeQ, &(*qTmp)(0), &(*lambda)(0), sizeY, &(*Gtmp)(0, 0), &(*param)(0));
    }
    else if (index == 1)
    {
      if (G21Ptr == NULL)
        RuntimeException::selfThrow("computeG() is not linked to a plugin function");
      param = parametersList["G21"];
      G21Ptr(sizeQ, &(*qTmp)(0), &(*lambda)(0), sizeY, &(*Gtmp)(0, 0), &(*param)(0));
    }
    else
      RuntimeException::selfThrow("LagrangianR::computeG, index out of range");
  }
  else
    RuntimeException::selfThrow("LagrangianR::computeG,  not yet implemented for this type of constraints");

  delete qTmp;

}

void LagrangianR::computeOutput(const double& time)
{
  if (interaction == NULL)
    RuntimeException::selfThrow("LagrangianR::computeOutput, no interaction linked with this relation");

  unsigned int sizeY = interaction->getInteractionSize();
  unsigned int sizeQ = interaction->getSizeOfDS();

  // Get the DS concerned by the interaction of this relation
  DSSet vDS = interaction->getDynamicalSystems();
  DSIterator it;
  string type;
  BlockVector *qTmp = new BlockVector();
  BlockVector *velocityTmp = new BlockVector();

  LagrangianDS* lds;
  for (it = vDS.begin(); it != vDS.end(); ++it)
  {
    type = (*it)->getType();
    // check dynamical system type
    if (type != LTIDS && type != LNLDS)
      RuntimeException::selfThrow("LagrangianLinearR::computeOutput not yet implemented for dynamical system of type: " + type);

    // convert vDS systems into LagrangianDS and put them in vLDS
    lds = static_cast<LagrangianDS*>(*it);

    // Put q of each DS into a block
    // Warning: use copy constructors (add function), no link between pointers
    qTmp->add(lds->getQ());
    velocityTmp->add(lds->getVelocity());
  }

  // get y and yDot of the interaction
  SimpleVector *y = interaction->getYPtr(0);
  SimpleVector *yDot = interaction->getYPtr(1);
  SimpleVector *lambda = interaction->getLambdaPtr(0);
  SimpleVector *lambdaDot = interaction->getLambdaPtr(1);
  SimpleVector* param;

  // y = h(...) and yDot = GqDot
  if (LagrangianRelationType == "scleronomic")
  {
    if (h0Ptr == NULL)
      RuntimeException::selfThrow("LagrangianR:computeOutput() h0 is not linked to a plugin function");
    param = parametersList["h"];
    h0Ptr(sizeQ, &(*qTmp)(0) , sizeY, &(*y)(0), &(*param)(0));
    SiconosMatrix * Gtmp = G[0];
    if (G0Ptr == NULL)
      RuntimeException::selfThrow("computeG() is not linked to a plugin function");
    param = parametersList["G0"];
    G0Ptr(sizeQ, &(*qTmp)(0), sizeY, &(*Gtmp)(0, 0), &(*param)(0));
    *yDot = *Gtmp * *velocityTmp;
  }
  // y = h(...) and yDot = G10qDot + G11
  else if (LagrangianRelationType == "rhenomorous")
  {
    if (h1Ptr == NULL)
      RuntimeException::selfThrow("LagrangianR:computeOutput() h1 is not linked to a plugin function");
    param = parametersList["h"];
    h1Ptr(sizeQ, &(*qTmp)(0), &time, sizeY,  &(*y)(0), &(*param)(0));
    SiconosMatrix * G0tmp = G[0];
    SiconosMatrix * G1tmp = G[1];

    if (G10Ptr == NULL)
      RuntimeException::selfThrow("computeG() is not linked to a plugin function");
    param = parametersList["G10"];
    G10Ptr(sizeQ, &(*qTmp)(0), &time, sizeY, &(*G0tmp)(0, 0), &(*param)(0));

    if (G11Ptr == NULL)
      RuntimeException::selfThrow("computeG() is not linked to a plugin function");
    param = parametersList["G11"];
    G11Ptr(sizeQ, &(*qTmp)(0), &time, sizeY, &(*G1tmp)(0, 0), &(*param)(0));

    // warning: G1 is a matrix
    SimpleVector * Id = new SimpleVector(1);
    (*Id)(0) = 1.0;
    *yDot = *G0tmp * *velocityTmp + *G1tmp * *Id;
    delete Id;
  }
  else if (LagrangianRelationType == "scleronomic+lambda")
  {
    if (h1Ptr == NULL)
      RuntimeException::selfThrow("LagrangianR:computeOutput() h2 is not linked to a plugin function");
    param = parametersList["h"];
    h2Ptr(sizeQ, &(*qTmp)(0), &(*lambda)(0), sizeY,  &(*y)(0), &(*param)(0));
    SiconosMatrix * G0tmp = G[0];
    SiconosMatrix * G1tmp = G[1];

    if (G20Ptr == NULL)
      RuntimeException::selfThrow("computeG() is not linked to a plugin function");
    param = parametersList["G20"];
    G20Ptr(sizeQ, &(*qTmp)(0), &(*lambda)(0), sizeY, &(*G0tmp)(0, 0), &(*param)(0));

    if (G21Ptr == NULL)
      RuntimeException::selfThrow("computeG() is not linked to a plugin function");
    param = parametersList["G21"];
    G21Ptr(sizeQ, &(*qTmp)(0), &(*lambda)(0), sizeY, &(*G1tmp)(0, 0), &(*param)(0));

    *yDot = *G0tmp * *velocityTmp + *G1tmp * *lambdaDot;
  }
  else
    RuntimeException::selfThrow("LagrangianR::computeOutput(),  not yet implemented for this type of constraints");

  // free memory
  delete qTmp;
  delete velocityTmp;
}
void LagrangianR::computeFreeOutput(const double& time)
{
  // warning: the only difference with computeOutput is that we get qFREE and velocityFREE instead of q and velocity.
  if (interaction == NULL)
    RuntimeException::selfThrow("LagrangianR::computeOutput, no interaction linked with this relation");

  unsigned int sizeY = interaction->getInteractionSize();
  unsigned int sizeQ = interaction->getSizeOfDS();

  // Get the DS concerned by the interaction of this relation
  DSSet vDS = interaction->getDynamicalSystems();
  DSIterator it;
  string type;
  BlockVector *qTmp = new BlockVector();
  BlockVector *velocityTmp = new BlockVector();

  LagrangianDS* lds;
  for (it = vDS.begin(); it != vDS.end(); ++it)
  {
    type = (*it)->getType();
    // check dynamical system type
    if (type != LTIDS && type != LNLDS)
      RuntimeException::selfThrow("LagrangianLinearR::computeFreeOutput not yet implemented for dynamical system of type: " + type);

    // convert vDS systems into LagrangianDS and put them in vLDS
    lds = static_cast<LagrangianDS*>(*it);

    // Put q of each DS into a block
    // Warning: use copy constructors (add function), no link between pointers
    qTmp->add(lds->getQFree());
    velocityTmp->add(lds->getVelocityFree());
  }

  // get y and yDot of the interaction
  SimpleVector *y = interaction->getYPtr(0);
  SimpleVector *yDot = interaction->getYPtr(1);
  SimpleVector *lambda = interaction->getLambdaPtr(0);
  SimpleVector *lambdaDot = interaction->getLambdaPtr(1);
  SimpleVector* param;

  // y = h(...) and yDot = GqDot
  if (LagrangianRelationType == "scleronomic")
  {
    if (h0Ptr == NULL)
      RuntimeException::selfThrow("LagrangianR:computeOutput() h0 is not linked to a plugin function");
    param = parametersList["h"];
    h0Ptr(sizeQ, &(*qTmp)(0) , sizeY, &(*y)(0), &(*param)(0));
    SiconosMatrix * Gtmp = G[0];
    if (G0Ptr == NULL)
      RuntimeException::selfThrow("computeG() is not linked to a plugin function");
    param = parametersList["G0"];
    G0Ptr(sizeQ, &(*qTmp)(0), sizeY, &(*Gtmp)(0, 0), &(*param)(0));

    *yDot = *Gtmp * *velocityTmp;
  }

  // y = h(...) and yDot = G10qDot + G11
  else if (LagrangianRelationType == "rhenomorous")
  {
    if (h1Ptr == NULL)
      RuntimeException::selfThrow("LagrangianR:computeOutput() h1 is not linked to a plugin function");
    param = parametersList["h"];
    h1Ptr(sizeQ, &(*qTmp)(0), &time, sizeY,  &(*y)(0), &(*param)(0));
    SiconosMatrix * G0tmp = G[0];
    SiconosMatrix * G1tmp = G[0];

    if (G10Ptr == NULL)
      RuntimeException::selfThrow("computeG() is not linked to a plugin function");
    param = parametersList["G10"];
    G10Ptr(sizeQ, &(*qTmp)(0), &time, sizeY, &(*G0tmp)(0, 0), &(*param)(0));

    if (G11Ptr == NULL)
      RuntimeException::selfThrow("computeG() is not linked to a plugin function");
    param = parametersList["G11"];
    G11Ptr(sizeQ, &(*qTmp)(0), &time, sizeY, &(*G1tmp)(0, 0), &(*param)(0));

    // warning: G1 is a matrix
    SimpleVector * Id = new SimpleVector(1);
    (*Id)(0) = 1.0;
    *yDot = *G0tmp * *velocityTmp + *G1tmp * *Id;
  }
  else if (LagrangianRelationType == "scleronomic+lambda")
  {
    if (h1Ptr == NULL)
      RuntimeException::selfThrow("LagrangianR:computeOutput() h2 is not linked to a plugin function");
    param = parametersList["h"];
    h2Ptr(sizeQ, &(*qTmp)(0), &(*lambda)(0), sizeY,  &(*y)(0), &(*param)(0));
    SiconosMatrix * G0tmp = G[0];
    SiconosMatrix * G1tmp = G[1];

    if (G20Ptr == NULL)
      RuntimeException::selfThrow("computeG() is not linked to a plugin function");
    param = parametersList["G20"];
    G20Ptr(sizeQ, &(*qTmp)(0), &(*lambda)(0), sizeY, &(*G0tmp)(0, 0), &(*param)(0));

    if (G21Ptr == NULL)
      RuntimeException::selfThrow("computeG() is not linked to a plugin function");
    param = parametersList["G21"];
    G21Ptr(sizeQ, &(*qTmp)(0), &(*lambda)(0), sizeY, &(*G1tmp)(0, 0), &(*param)(0));

    *yDot = *G0tmp * *velocityTmp + *G1tmp * *lambdaDot;
  }
  else
    RuntimeException::selfThrow("LagrangianR::computeOutput(),  not yet implemented for this type of constraints");


  // free memory
  delete qTmp;
  delete velocityTmp;
}

void LagrangianR::computeInput(const double& time)
{
  if (interaction == NULL)
    RuntimeException::selfThrow("LagrangianLinearR::computeInput, no interaction linked with this relation");

  // Get the DS concerned by the interaction of this relation
  DSSet vDS = interaction->getDynamicalSystems();
  DSIterator it;
  string type;

  BlockVector *p = new BlockVector();
  LagrangianDS* lds;
  for (it = vDS.begin(); it != vDS.end(); ++it)
  {
    type = (*it)->getType();
    // check dynamical system type
    if (type != LTIDS && type != LNLDS)
      RuntimeException::selfThrow("LagrangianLinearR::computeInput not yet implemented for dynamical system of type: " + type);

    // convert vDS systems into LagrangianDS and put them in vLDS
    lds = static_cast<LagrangianDS*>(*it);

    // Put p of each DS into a block
    // Warning: use addPtr -> link between pointers
    p->addPtr(lds->getPPtr());
  }

  // get lambda of the concerned interaction
  SimpleVector *lambda = interaction->getLambdaPtr(1);

  if (LagrangianRelationType == "scleronomic")
  {
    computeG(time, 0);
    *p += matTransVecMult(*(G[0]), *lambda);
  }
  else
    RuntimeException::selfThrow("LagrangianR::computeInput,  not yet implemented for constraints of type" + LagrangianRelationType);
  delete p;
}

void LagrangianR::getGBlockDS(DynamicalSystem * ds, SiconosMatrix& Block, const unsigned & index) const
{
  unsigned int k = 0;
  DSSet vDS = interaction ->getDynamicalSystems();
  DSIterator itDS;
  itDS = vDS.begin();

  // look for ds
  while (*itDS != ds && itDS != vDS.end())
  {
    k += (*itDS)->getN() / 2;
    itDS++;
  }
  // check dimension
  if ((*itDS)->getN() / 2 != Block.size(1))
    RuntimeException::selfThrow("LagrangianR - getGBlockDS: inconsistent sizes between GBlock and DS");

  // get block
  unsigned int l = k + (*itDS)->getN() / 2 - 1;
  vector<unsigned int> index_list(4);
  index_list[0] = 0;
  index_list[1] = G[index]->size(0) - 1;
  index_list[2] = k;
  index_list[3] = l;
  G[index]->getBlock(index_list, Block);
}

void LagrangianR::getGBlockDS(const int& DSNumber, SiconosMatrix& Block, const unsigned & index) const
{
  unsigned int k = 0;

  DSSet vDS = interaction ->getDynamicalSystems();

  DSIterator itDS = vDS.begin();

  // look for DS number DSNumber ...
  while ((*itDS)->getNumber() != DSNumber && itDS != vDS.end())
  {
    k += (*itDS)->getN() / 2;
    itDS++;
  }

  // check dimension
  if ((*itDS)->getN() / 2 != Block.size(1))
    RuntimeException::selfThrow("LagrangianR - getGlockDS: inconsistent sizes between GBlock and DS");

  // get block
  unsigned int l = k + (*itDS)->getN() / 2 - 1;
  vector<unsigned int> index_list(4);
  index_list[0] = 0;
  index_list[1] = G[index]->size(0) - 1;
  index_list[2] = k;
  index_list[3] = l;
  G[index]->getBlock(index_list, Block);
}

void LagrangianR::saveRelationToXML() const
{
  if (relationxml != NULL)
  {
    relationxml->setComputeInputPlugin(computeInputName);
    relationxml->setComputeOutputPlugin(computeOutputName);
  }
  else RuntimeException::selfThrow("LagrangianR::saveRelationToXML - object RelationXML does not exist");
}

LagrangianR* LagrangianR::convert(Relation *r)
{
  LagrangianR* lnlr = dynamic_cast<LagrangianR*>(r);
  return lnlr;
}

void LagrangianR::display() const
{
  cout << "===== Lagrangian Relation display ===== " << endl;
  cout << "- Relation type: " << relationType << endl;
  if (interaction != NULL) cout << "- Interaction id" << interaction->getId() << endl;
  else cout << "- Linked interaction -> NULL" << endl;
  if (isHPlugged) cout << "h is plugged on function: " << hFunctionName << endl;
  for (unsigned int i = 0; i < G.size(); i++)
  {
    if (isGPlugged[i]) cout << "G" << i << " is plugged on function: " << GFunctionName[i] << endl;
    cout << " G" << i << ": " << endl;
    if (G[i] != NULL)
      G[i] -> display();
    else
      cout << " -> NULL " << endl;
  }
  cout << "===================================== " << endl;
}
