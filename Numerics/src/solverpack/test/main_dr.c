///////////////////////////////////////////////////////////////////////////
// This main file allows the dual resolution of relay problems:
//  try (z,w) such that:
//
//
//
//                    M z  +q = w                      (1)
//                    -z in diff(PSI) (w)              (2)
//                               [-b,a]
//
//
//  here M is an n by n  matrix, q an n-dimensional vector, z,w,b,a an n-dimensional vectors.
//
//  This system of equations and inequalities is solved thanks to rp_subroutine:
//        dr_gsnl  ( M, q, n, a, b, itermax, tol, chat, z, w, it_end, res, info)
//        dr_latin ( M, q, n, k_latin,  a, b, itermax, tol, chat, z, w, it_end, res, info)
//
//
//  where _ itermax      is the maximum iterations required, it's an integer
//        _ res          is the residue, it's a float (positive float)
//        _ it_end       is the number of iterations carried out, it's an integer
//        _ tol          is the tolerance value desired, it's a positive float (if you make it non positive then you force the convergence)
//        _ chat         is the output log identifiant
//        _ k_latin      is the parameter research of the latin, it's a float (strictly positive)
//        _ a            is the upper bound, it's a vector of floats
//        _ b            is the down bound, it's a vector of floats
//        _ z and w      are the solutions of the problem
//        _ info         shows the termination reason,0 is successful (otherwise see the termination reason of the solver used), it's an integer.
//
//
//    For more information about the methods see the chapter 4 of the Siconos manual theory.
//
//
//
//  The subroutine's call is due to the function solve_rd:
//
//  int solve_dr ( double *M, double *q, int n, methode *pt, double *z, double *w )
//
//  where M       is an n by n matrix,
//        q       an n-dimensional vector,
//        n       is the row dimension of M,
//        pt      a pointer other a union ( methode see "solverpack.h").
//        z and w are n-dimensional  vectors solution.
//
//        methode is a variable with a union type; in this union you find the structure (method_rd) that gives to the function
//  solve_dr, the name and the parameters (itermax, tol, k_latin, a, b, chat) of the method we want to use.
//  This function return an interger:  0 successful .
//
//
///////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "solverpack.h"

#define CHAT


void test_mmc(void)
{
  FILE *f1, *f2, *f3, *f4;
  int i, j, nl, nc, nll, n = 40;
  int info1 = -1, info2 = -1;
  int nonsymmetric;

  double *q, *z1, *w1, *z2, *w2, *vec, *a, *b;
  double qi, Mij;


  char val[50], vall[50];
  method meth_dr1, meth_dr2;


  printf("\n* *** ******************** *** * \n");
  printf("* ***        TEST MMC      *** * \n");
  printf("* *** ******************** *** * \n");



  /*           Allocations                     */

  q   = (double*)malloc(n * sizeof(double));
  z1  = (double*)malloc(n * sizeof(double));
  w1  = (double*)malloc(n * sizeof(double));
  z2  = (double*)malloc(n * sizeof(double));
  w2  = (double*)malloc(n * sizeof(double));
  a   = (double*) malloc(n * sizeof(double));
  b   = (double*)malloc(n * sizeof(double));
  vec = (double*) malloc(n * n * sizeof(double));


  /*     Data loading of M , q , a and b           */

  if ((f1 = fopen("DATA/M_relay_rd2.dat", "r")) == NULL)
  {
    perror("fopen 1");
    exit(1);
  }


  if ((f2 = fopen("DATA/q_relay_rd2.dat", "r")) == NULL)
  {
    perror("fopen 2");
    exit(2);
  }


  if ((f3 = fopen("DATA/a_relay_rd2.dat", "r")) == NULL)
  {
    perror("fopen 5");
    exit(5);
  }


  if ((f4 = fopen("DATA/b_relay_rd2.dat", "r")) == NULL)
  {
    perror("fopen 6");
    exit(6);
  }



  while (!feof(f1))
  {
    fscanf(f1, "%d", &nl);
    fscanf(f1, "%d", &nc);
    fscanf(f1, "%s", val);
    Mij = atof(val);


    vec[(nc - 1)*n + nl - 1 ] = Mij;

  }




  while (!feof(f2))
  {
    fscanf(f2, "%d", &nll);
    fscanf(f2, "%s", vall);
    qi = atof(vall);
    q[nll - 1] = -qi;

  }


  while (!feof(f3))
  {
    fscanf(f3, "%d", &nll);
    fscanf(f3, "%s", vall);
    qi = atof(vall);
    a[nll - 1] = qi;
  }


  while (!feof(f4))
  {
    fscanf(f4, "%d", &nll);
    fscanf(f4, "%s", vall);
    qi = atof(vall);
    b[nll - 1] = qi;
  }


  fclose(f2);
  fclose(f4);
  fclose(f1);
  fclose(f3);



  /*           End od the data loading        */


  nonsymmetric = 0;


  /*           Is M symmetric ?                */

  for (i = 0 ; i < n ; ++i)
  {
    for (j = 0 ; j < i ; ++j)
    {
      if (abs(vec[i * n + j] - vec[j * n + i]) > 1e-16)
      {
        nonsymmetric = 1;
        break;
      }
    }
  }



#ifdef CHAT
  if (nonsymmetric) printf("\n !! WARNING !!\n M is a non symmetric matrix \n \n");
  else printf(" \n M is a symmetric matrix \n \n");
#endif





  /*           Initialization                 */

  strcpy(meth_dr1.dr.name, "NLGS");
  meth_dr1.dr.itermax  =  1000;
  meth_dr1.dr.tol      =  0.000001;
  meth_dr1.dr.chat  =  0;


  strcpy(meth_dr2.dr.name, "Latin");
  meth_dr2.dr.itermax  =  5000;
  meth_dr2.dr.tol      =  0.0000000000001;
  meth_dr2.dr.k_latin  =  0.09;
  meth_dr2.dr.chat  =  0;




  meth_dr1.dr.a = (double*)malloc(n * sizeof(double));
  meth_dr1.dr.b = (double*)malloc(n * sizeof(double));
  meth_dr2.dr.a = (double*)malloc(n * sizeof(double));
  meth_dr2.dr.b = (double*)malloc(n * sizeof(double));

  for (i = 0; i <= n - 1; i++)
  {

    meth_dr1.dr.a[i] =  a[i];
    meth_dr1.dr.b[i] = -b[i];
    meth_dr2.dr.a[i] =  a[i];
    meth_dr2.dr.b[i] = -b[i];


  }




#ifdef CHAT
  printf("**** NLGS TEST ****\n \n");
#endif

  info1 = dr_solver(vec, q, &n, &meth_dr1, z1, w1);

#ifdef CHAT
  printf("\n**** LATIN TEST ***\n \n");
#endif

  info2 = dr_solver(vec, q, &n, &meth_dr2, z2, w2);


#ifdef CHAT
  printf(" *** ************************************** ***\n");
  for (i = 0 ; i < n ; ++i)
    printf(" NLGS RESULT : %14.7e %14.7e   LATIN RESULT : % 14.7e  %14.7e \n" , z1[i] , w1 [i], z2[i], w2[i]);

  printf("\n -- SOLVEUR ------ ITER ----- ERR\n");
  printf(" \n NLGS RESULT : %d   % 14.7e \n" , meth_dr1.dr.iter , meth_dr1.dr.err);
  printf("   LATIN RESULT : %d   % 14.7e \n" , meth_dr2.dr.iter , meth_dr2.dr.err);





  printf(" *** ************************************** ***\n");
#endif


  free(vec);
  free(q);
  free(z1);
  free(w1);
  free(z2);
  free(w2);
  free(a);
  free(b);
  free(meth_dr1.dr.a);
  free(meth_dr1.dr.b);
  free(meth_dr2.dr.a);
  free(meth_dr2.dr.b);
}


int main(void)
{
  test_mmc();
  return 1;
}


