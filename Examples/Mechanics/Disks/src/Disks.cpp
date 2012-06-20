/* Siconos-sample , Copyright INRIA 2005-2011.
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

/*!\file Disks.cpp

  Some Disks (2D), friction, and walls.
  Direct description of the model without XML input.  Simulation with
  a Time-Stepping scheme.
*/

// Siconos
#include <SiconosKernel.hpp>
#include <SiconosPointers.hpp>

//#include <Siconos/IO/SiconosRestart.hpp>

#include "Disks.hpp"

using namespace std;

/* do nothing if solver does not converge */
void localCheckSolverOuput(int, Simulation*)
{};

double A(double t)
{
  return 0. ;
}
double B(double t)
{
  return 1. ;
}
double C(double t)
{
  return 0.0;//1.1*cos(32.*M_PI*t) ;
}
double DA(double t)
{
  return 0. ;
}
double DB(double t)
{
  return 0. ;
}
double DC(double t)
{
  return 0.0;//-1.1*32.*M_PI*sin(32.*M_PI*t) ;
}


// ================= Creation of the model =======================
void Disks::init()
{

  SP::TimeDiscretisation timedisc_;
  SP::TimeStepping simulation_;
  SP::FrictionContact osnspb_;

  // User-defined main parameters

  double t0 = 0;                   // initial computation time

  double T = 0.02;

  double h = 0.01;                // time step
  double g = 9.81;

  double theta = 0.5;              // theta for Moreau integrator

  std::string solverName = "NSGS";

  // -----------------------------------------
  // --- Dynamical systems && interactions ---
  // -----------------------------------------

  double R;
  double m;

  try
  {

    // ------------
    // --- Init ---
    // ------------

    std::cout << "====> Model loading ..." << std::endl << std::endl;

    _plans.reset(new SimpleMatrix("plans.dat", true));
    if (_plans->size(0) == 0)
    {
      /* default plans */
      double A1 = P1A;
      double B1 = P1B;
      double C1 = P1C;

      double A2 = P2A;
      double B2 = P2B;
      double C2 = P2C;

      _plans.reset(new SimpleMatrix(6, 6));
      _plans->zero();
      (*_plans)(0, 0) = 0;
      (*_plans)(0, 1) = 1;
      (*_plans)(0, 2) = -GROUND;

      (*_plans)(1, 0) = 1;
      (*_plans)(1, 1) = 0;
      (*_plans)(1, 2) = WALL;

      (*_plans)(2, 0) = 1;
      (*_plans)(2, 1) = 0;
      (*_plans)(2, 2) = -WALL;

      (*_plans)(3, 0) = 0;
      (*_plans)(3, 1) = 1;
      (*_plans)(3, 2) = -TOP;

      (*_plans)(4, 0) = A1;
      (*_plans)(4, 1) = B1;
      (*_plans)(4, 2) = C1;

      (*_plans)(5, 0) = A2;
      (*_plans)(5, 1) = B2;
      (*_plans)(5, 2) = C2;

    }

    /* set center positions */
    for (unsigned int i = 0 ; i < _plans->size(0); ++i)
    {
      SP::DiskPlanR tmpr;
      tmpr.reset(new DiskPlanR(1, (*_plans)(i, 0), (*_plans)(i, 1), (*_plans)(i, 2),
                               (*_plans)(i, 3), (*_plans)(i, 4), (*_plans)(i, 5)));
      (*_plans)(i, 3) = tmpr->getXCenter();
      (*_plans)(i, 4) = tmpr->getYCenter();
    }

    /*    _moving_plans.reset(new FMatrix(1,6));
        (*_moving_plans)(0,0) = &A;
        (*_moving_plans)(0,1) = &B;
        (*_moving_plans)(0,2) = &C;
        (*_moving_plans)(0,3) = &DA;
        (*_moving_plans)(0,4) = &DB;
        (*_moving_plans)(0,5) = &DC;*/



    SP::SiconosMatrix Disks;
    Disks.reset(new SimpleMatrix("disks.dat", true));

    // -- OneStepIntegrators --
    SP::OneStepIntegrator osi;
    osi.reset(new Moreau(theta));

    // -- Model --
    _model.reset(new Model(t0, T));

    for (unsigned int i = 0; i < Disks->size(0); i++)
    {
      R = Disks->getValue(i, 2);
      m = Disks->getValue(i, 3);

      SP::SiconosVector qTmp;
      SP::SiconosVector vTmp;

      qTmp.reset(new SiconosVector(NDOF));
      vTmp.reset(new SiconosVector(NDOF));
      vTmp->zero();
      (*qTmp)(0) = (*Disks)(i, 0);
      (*qTmp)(1) = (*Disks)(i, 1);

      SP::LagrangianDS body;
      if (R > 0)
        body.reset(new Disk(R, m, qTmp, vTmp));
      else
        body.reset(new Circle(-R, m, qTmp, vTmp));

      // -- Set external forces (weight) --
      SP::SiconosVector FExt;
      FExt.reset(new SiconosVector(NDOF));
      FExt->zero();
      FExt->setValue(1, -m * g);
      body->setFExtPtr(FExt);

      // add the dynamical system to the one step integrator
      osi->insertDynamicalSystem(body);

      // add the dynamical system in the non smooth dynamical system
      _model->nonSmoothDynamicalSystem()->insertDynamicalSystem(body);

    }


    _model->nonSmoothDynamicalSystem()->setSymmetric(true);


    // ------------------
    // --- Simulation ---
    // ------------------

    // -- Time discretisation --
    timedisc_.reset(new TimeDiscretisation(t0, h));

    // -- OneStepNsProblem --
    osnspb_.reset(new FrictionContact(2));

    osnspb_->numericsSolverOptions()->iparam[0] = 100; // Max number of
    // iterations
    osnspb_->numericsSolverOptions()->iparam[1] = 20; // compute error
    // iterations
    osnspb_->numericsSolverOptions()->dparam[0] = 1e-3; // Tolerance


    osnspb_->setMaxSize(6 * ((3 * Ll * Ll + 3 * Ll) / 2 - Ll));
    osnspb_->setMStorageType(1);            // Sparse storage
    osnspb_->setNumericsVerboseMode(0);

    osnspb_->setKeepLambdaAndYState(true);  // inject previous solution

    // -- Simulation --
    simulation_.reset(new TimeStepping(timedisc_));

    boost::static_pointer_cast<TimeStepping>(simulation_)->setNewtonMaxIteration(3);

    simulation_->insertIntegrator(osi);
    simulation_->insertNonSmoothProblem(osnspb_);

    simulation_->setCheckSolverFunction(localCheckSolverOuput);

    // --- Simulation initialization ---

    std::cout << "====> Simulation initialisation ..." << std::endl << std::endl;

    SP::NonSmoothLaw nslaw(new NewtonImpactFrictionNSL(0, 0, 0.3, 2));

    _playground.reset(new SpaceFilter(3, 6, _model, nslaw, _plans, _moving_plans));

    _model->initialize(simulation_);

  }

  catch (SiconosException e)
  {
    std::cout << e.report() << std::endl;
    exit(1);
  }
  catch (...)
  {
    std::cout << "Exception caught in Disks::init()" << std::endl;
    exit(1);
  }
}





// =========================== End of model definition ===========================

// ================================= Computation =================================

void Disks::compute()
{
  try
  {

    _playground->buildInteractions(_model->currentTime());

    _model->simulation()->advanceToEvent();

    _model->simulation()->processEvents();

    //    Siconos::save(_model, "Disks.bin");

  }

  catch (SiconosException e)
  {
    std::cout << e.report() << std::endl;
  }
  catch (...)
  {
    std::cout << "Exception caught in SiconosBodies::compute()" << std::endl;
  }
}
