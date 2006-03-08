/* Siconos-Kernel version 1.1.2, Copyright INRIA 2005-2006.
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
#include "FrictionContact2D.h"

// includes to be deleted thanks to factories?
#include "Moreau.h"
#include "LagrangianLinearR.h"
#include "NewtonImpactFrictionNSL.h"

using namespace std;

// Default constructor
FrictionContact2D::FrictionContact2D(): FrictionContact()
{
  nspbType = "FrictionContact2D";
}

// xml constructor
FrictionContact2D::FrictionContact2D(OneStepNSProblemXML* osNsPbXml, Strategy* newStrat):
  FrictionContact(osNsPbXml, newStrat)
{
  nspbType = "FrictionContact2D";
  // === read solver related data ===
  if (onestepnspbxml->hasSolver())
    solver = new Solver(onestepnspbxml->getSolverXMLPtr(), nspbType);
  else // solver = default one
  {
    solver = new Solver(nspbType, DEFAULT_SOLVER, DEFAULT_ITER, DEFAULT_TOL, DEFAULT_NORMTYPE, DEFAULT_SEARCHDIR);
    cout << " Warning, no solver defined in non-smooth problem - Default one is used" << endl;
    solver->display();
  }
  isSolverAllocatedIn = true;
}

// Constructor from a set of data
FrictionContact2D::FrictionContact2D(Strategy* newStrat, const string& solverName,
                                     const unsigned int& MaxIter, const double & Tolerance, const string & NormType,
                                     const double & SearchDirection): FrictionContact(newStrat)
{
  nspbType = "FrictionContact2D";
  // set solver:
  solver = new Solver(nspbType, solverName, MaxIter, Tolerance, NormType, SearchDirection);
  isSolverAllocatedIn = true;
}

// Constructor from a set of data
FrictionContact2D::FrictionContact2D(Strategy* newStrat, Solver*  newSolver):
  FrictionContact(newStrat, newSolver)
{
  nspbType = "FrictionContact2D";
}

// destructor
FrictionContact2D::~FrictionContact2D()
{}

void FrictionContact2D::computeQ(const double& time)
{
  IN("FrictionContact2D::computeQ(void)\n");
  if (q == NULL)
  {
    q = new SimpleVector(dim);
    isQAllocatedIn = true;
  }
  else if (q->size() != dim)
  {
    // reset q if it has a wrong size
    if (isQAllocatedIn) delete q;
    q = new SimpleVector(dim);
    isQAllocatedIn = true;
  }

  q->zero();

  // --- get topology ---
  Topology * topology = strategy->getModelPtr()->getNonSmoothDynamicalSystemPtr()->getTopologyPtr();
  // --- get interactions list ---
  vector<Interaction*> listInteractions = strategy->getModelPtr()->getNonSmoothDynamicalSystemPtr()->getInteractions();
  vector<Interaction*>::iterator iter;
  // get Interaction position map
  map< Interaction* , SiconosMatrix*>::iterator itDiago;
  map< Interaction*, unsigned int>  interactionEffectivePositionMap =  topology->getInteractionEffectivePositionMap();
  unsigned int pos;
  Relation *R;
  NonSmoothLaw *nslaw;
  vector<unsigned int> index ;
  unsigned int effectiveSize ;
  Interaction *currentInteraction ;
  SimpleVector * yFree;
  // --- loop over all the interactions ---
  for (iter = listInteractions.begin(); iter != listInteractions.end(); ++iter)
  {
    // get current interaction, its relation and its nslaw
    currentInteraction = *iter;
    unsigned int numberOfRelations = currentInteraction->getNInteraction();
    R = currentInteraction->getRelationPtr();
    string relationType = R->getType();
    nslaw = currentInteraction->getNonSmoothLawPtr();
    // position of current yFree in global (including all interactions) y vector
    pos = interactionEffectivePositionMap[currentInteraction];
    index = topology->getEffectiveIndexes(currentInteraction);

    if (relationType == LAGRANGIANLINEARRELATION || relationType == LAGRANGIANRELATION)
    {
      LagrangianLinearR *LLR = static_cast<LagrangianLinearR*>(R);
      if (nslaw->getType() == NEWTONIMPACTFRICTIONNSLAW)
      {
        vector<unsigned int> indexMax = topology->getIndexMax(currentInteraction);

        NewtonImpactFrictionNSL * newton = static_cast<NewtonImpactFrictionNSL*>(nslaw);
        double e = newton->getEn();
        //double mu= newton->getMu();
        LLR->computeFreeOutput(time);
        if (topology->isTimeInvariant())
        {
          yFree = new SimpleVector(currentInteraction -> getY(1)); // copy, no pointer equality
          // Only normal part has to be multiplied by e.
          // even indexes corresponds to normal components of y.

          SimpleVector * yDotOld = currentInteraction->getYOldPtr(1);
          for (unsigned int i = 0; i < numberOfRelations / 2; i++)
            (*yFree)(2 * i) += e * (*yDotOld)(2 * i);

          for (unsigned int i = 0; i < numberOfRelations; i++)
            (*q)(i + pos) = (*yFree)(i);
        }
        else
        {
          // compute list of effective relations
          unsigned int k = 0;
          for (unsigned int j = 0; j < numberOfRelations; j++)
          {
            for (unsigned int i = 1; i < indexMax[j] + 1; i++)
            {
              index[k] = j;
              k++;
            }
          }

          effectiveSize = index.size();
          yFree = new SimpleVector(effectiveSize); // we get only the "effective" relations
          (currentInteraction->getY(1)).getBlock(index, *yFree); // copy, no pointer equality
          SimpleVector * tmp =  new SimpleVector(effectiveSize);
          (currentInteraction -> getYOld(1)).getBlock(index, *tmp);

          for (unsigned int i = 0; i < numberOfRelations / 2; i++)
            (*yFree)(2 * i) += e * (*tmp)(2 * i);

          delete tmp;
          for (unsigned int i = 0; i < effectiveSize; i++)
            (*q)(i + pos) = (*yFree)(i);
        }
      }
      else
        RuntimeException::selfThrow("FrictionContact2D::computeQ not yet implemented for NSlaw of type " + nslaw->getType() + "and for relation of type " + R->getType());
    }
    else
      RuntimeException::selfThrow("FrictionContact2D::computeQ not yet implemented for relation of type " + R->getType());

    delete yFree;
  }
  OUT("FrictionContact2D::computeQ(void)\n");
}

FrictionContact2D* FrictionContact2D::convert(OneStepNSProblem* osnsp)
{
  FrictionContact2D* fc2d = dynamic_cast<FrictionContact2D*>(osnsp);
  return fc2d;
}


