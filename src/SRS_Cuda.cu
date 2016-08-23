////////////////////////////////////////////////////////////////////
//
// Dean Andrew Hidas <dhidas@bnl.gov>
//
// Created on: Tue May 24 11:33:19 EDT 2016
//
////////////////////////////////////////////////////////////////////

#include <cuComplex.h>

#include "SRS_Cuda.h"

#include "SRS.h"

#include <cmath>
#include <fstream>
#include <sstream>

#include "TVector3DC.h"
#include "TBField1DZRegularized.h"
#include "TBField3DZRegularized.h"
#include "TSpectrumContainer.h"





__device__ __forceinline__ cuComplex my_cexpf (cuComplex& z)
{

  cuComplex res;
  float t = exp(z.x);
  sincos (z.y, &res.y, &res.x);
  res.x *= t;
  res.y *= t;

  return res;
}



__global__ void ComplexTest()
{
  double cr = 1;
  double ci = 2;
  double r = 3;
  cuDoubleComplex c = make_cuDoubleComplex(cr, ci);
  cuDoubleComplex result = cuCmul(c, make_cuDoubleComplex(r, 0));

  result = cuCadd(c, make_cuDoubleComplex(r, 0));
  result = cuCsub(c, make_cuDoubleComplex(r, 0));
  result = cuCdiv(c, make_cuDoubleComplex(r, 0));
}


__global__ void Orthogonal(double *a, double *b)
{
  // Return a vector which is orthogonal vector a
  double xx = a[0] < 0.0 ? -a[0] : a[0];
  double yy = a[1] < 0.0 ? -a[1] : a[1];
  double zz = a[2] < 0.0 ? -a[2] : a[2];
  if (xx < yy) {
    if (xx < zz) {
      b[0] = 0;
      b[1] = a[2];
      b[2] = -a[1];
    } else {
      b[0] = a[1];
      b[1] = -a[0];
      b[2] = 0;
    }
  } else {
    if (yy < zz) {
      b[0] = -a[2];
      b[1] = 0;
      b[2] = a[0];
    } else {
      b[0] = a[1];
      b[1] = -a[0];
      b[2] = 0;
    }
  }
}



__host__ __device__ static __inline__ cuDoubleComplex cuCexp(cuDoubleComplex x)
{
  double factor = exp(x.x);
  return make_cuDoubleComplex(factor * cos(x.y), factor * sin(x.y));
}




__global__ void SRS_Cuda_SpectrumGPU (double *x, double *y, double *z, double *bx, double *by, double *bz, double *ox, double *oy, double *oz, double *dt, int *nt, int *ns, double *C0, double *C2, double *EvToOmega, double *C, double *se, double *sf)
{
  // Check that this is within the number of spectrum points requested
  int is = threadIdx.x + blockIdx.x * blockDim.x;
  if (is >= *ns) {
    return;
  }

  // Complex i
  cuDoubleComplex I = make_cuDoubleComplex(0, 1);

  double const Omega = *EvToOmega * se[is];
  cuDoubleComplex ICoverOmega = make_cuDoubleComplex(0, (*C) / Omega);


  // E-field omponents sum
  cuDoubleComplex SumEX = make_cuDoubleComplex(0, 0);
  cuDoubleComplex SumEY = make_cuDoubleComplex(0, 0);
  cuDoubleComplex SumEZ = make_cuDoubleComplex(0, 0);


  // Loop over all points in trajectory
  for (int i = 0; i < *nt; ++i) {

    // Distance to observer
    double const D = sqrt( pow(ox[i] - x[i], 2) + pow(oy[i] - y[i], 2) + pow(oz[i] - z[i], 2) );

    // Normal in direction of observer
    double const NX = (ox[i] - x[i]) / D;
    double const NY = (oy[i] - y[i]) / D;
    double const NZ = (oz[i] - z[i]) / D;

    // Exponent for fourier transformed field
    cuDoubleComplex Exponent = make_cuDoubleComplex(0, Omega * ((*dt) * i + D / (*C)));


    cuDoubleComplex BXMinusNXTimesICOverOmegaOverDOverD = make_cuDoubleComplex(bx[i] - NX / D, -NX * Omega * ((*dt) * i + D / (*C)) / D);
    cuDoubleComplex BYMinusNYTimesICOverOmegaOverDOverD = make_cuDoubleComplex(by[i] - NY / D, -NY * Omega * ((*dt) * i + D / (*C)) / D);
    cuDoubleComplex BZMinusNZTimesICOverOmegaOverDOverD = make_cuDoubleComplex(bz[i] - NZ / D, -NZ * Omega * ((*dt) * i + D / (*C)) / D);
    

    cuDoubleComplex MyEXP = cuCexp(Exponent);

    cuDoubleComplex X = cuCmul(BXMinusNXTimesICOverOmegaOverDOverD, MyEXP);
    cuDoubleComplex Y = cuCmul(BYMinusNYTimesICOverOmegaOverDOverD, MyEXP);
    cuDoubleComplex Z = cuCmul(BZMinusNZTimesICOverOmegaOverDOverD, MyEXP);


    SumEX = cuCadd(SumEX, X);
    SumEY = cuCadd(SumEY, Y);
    SumEZ = cuCadd(SumEZ, Z);

    // Sum in fourier transformed field (integral)
    //SumEX += (TVector3DC(B) - (N *     (One + (ICoverOmega / (D)))     )) / D * std::exp(Exponent);
  }

  SumEX = cuCmul(make_cuDoubleComplex(0, (*C0) * Omega * (*dt)), SumEX);
  SumEY = cuCmul(make_cuDoubleComplex(0, (*C0) * Omega * (*dt)), SumEY);
  SumEZ = cuCmul(make_cuDoubleComplex(0, (*C0) * Omega * (*dt)), SumEZ);


  double const EX = (*C2) * (SumEX.x * SumEX.x + SumEX.y * SumEX.y);
  double const EY = (*C2) * (SumEY.x * SumEY.x + SumEY.y * SumEY.y);
  double const EZ = (*C2) * (SumEZ.x * SumEZ.x + SumEZ.y * SumEZ.y);

  // Multiply field by Constant C1 and time step
  //SumE *= C1 * DeltaT;

  // Set the flux for this frequency / energy point
  //Spectrum.AddToFlux(i, C2 *  SumE.Dot( SumE.CC() ).real() * Weight);

  sf[is] = EX + EY + EZ;

}








void SRS_Cuda_CalculateSpectrumGPU (TParticleA& Particle, TVector3D const& ObservationPoint, TSpectrumContainer& Spectrum, double const Weight)
{

  int ngpu = 0;
  cudaGetDeviceCount(&ngpu);
  if (ngpu == 0) {
    throw std::invalid_argument("No GPU found");
  }

  std::cout << "ngpu " << ngpu << std::endl;




  // Grab the Trajectory
  TParticleTrajectoryPoints& T = Particle.GetTrajectory();

  // Number of points in Trajectory
  int const NTPoints = (int) T.GetNPoints();

  // Timestep from trajectory
  double const DeltaT = T.GetDeltaT();

  double *x     = new double[NTPoints];
  double *y     = new double[NTPoints];
  double *z     = new double[NTPoints];
  double *bx    = new double[NTPoints];
  double *by    = new double[NTPoints];
  double *bz    = new double[NTPoints];

  int const NSPoints = (int) Spectrum.GetNPoints();

  // Observer
  double ox = ObservationPoint.GetX();
  double oy = ObservationPoint.GetY();
  double oz = ObservationPoint.GetZ();

  // Constants
  double const C = TSRS::C();
  double const EvToOmega = TSRS::EvToAngularFrequency(1);

  // Spectrum energy and flux
  double *se     = new double[NSPoints];
  double *sf     = new double[NSPoints];


  // Set trajectory
  for (size_t i = 0; i < NTPoints; ++i) {
    x[i] = T.GetX(i).GetX();
    y[i] = T.GetX(i).GetY();
    z[i] = T.GetX(i).GetZ();

    bx[i] = T.GetB(i).GetX();
    by[i] = T.GetB(i).GetY();
    bz[i] = T.GetB(i).GetZ();
  }



  // Set energy to value
  for (size_t i = 0; i < NSPoints; ++i) {
    se[i] = Spectrum.GetEnergy(i);
  }



  double *d_x, *d_y, *d_z;
  double *d_bx, *d_by, *d_bz;
  double *d_ox, *d_oy, *d_oz;
  double *d_se, *d_sf;
  double *d_dt;
  int    *d_nt, *d_ns;

  int const size_x = NTPoints * sizeof(double);
  int const size_s = NSPoints * sizeof(double);

  cudaMalloc((void **) &d_x, size_x);
  cudaMalloc((void **) &d_y, size_x);
  cudaMalloc((void **) &d_z, size_x);

  cudaMalloc((void **) &d_bx, size_x);
  cudaMalloc((void **) &d_by, size_x);
  cudaMalloc((void **) &d_bz, size_x);

  cudaMalloc((void **) &d_ox, sizeof(double));
  cudaMalloc((void **) &d_oy, sizeof(double));
  cudaMalloc((void **) &d_oz, sizeof(double));

  cudaMalloc((void **) &d_dt, sizeof(double));
  cudaMalloc((void **) &d_nt, sizeof(int));
  cudaMalloc((void **) &d_ns, sizeof(int));

  cudaMalloc((void **) &d_se, sizeof(double));
  cudaMalloc((void **) &d_sf, sizeof(double));


  cudaMemcpy(d_x, x, size_x, cudaMemcpyHostToDevice);
  cudaMemcpy(d_y, y, size_x, cudaMemcpyHostToDevice);
  cudaMemcpy(d_z, z, size_x, cudaMemcpyHostToDevice);

  cudaMemcpy(d_bx, bx, size_x, cudaMemcpyHostToDevice);
  cudaMemcpy(d_by, by, size_x, cudaMemcpyHostToDevice);
  cudaMemcpy(d_bz, bz, size_x, cudaMemcpyHostToDevice);


  cudaMemcpy(d_ox, &ox, sizeof(double), cudaMemcpyHostToDevice);
  cudaMemcpy(d_oy, &oy, sizeof(double), cudaMemcpyHostToDevice);
  cudaMemcpy(d_oz, &oz, sizeof(double), cudaMemcpyHostToDevice);

  cudaMemcpy(d_dt, &DeltaT, sizeof(double), cudaMemcpyHostToDevice);
  cudaMemcpy(d_nt, &NTPoints, sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_ns, &NSPoints, sizeof(int), cudaMemcpyHostToDevice);

  cudaMemcpy(d_se, se, size_s, cudaMemcpyHostToDevice);


  // Constant C0 for calculation
  double const C0 = Particle.GetQ() / (TSRS::FourPi() * TSRS::C() * TSRS::Epsilon0() * TSRS::Sqrt2Pi());

  // Constant for flux calculation at the end
  double const C2 = TSRS::FourPi() * Particle.GetCurrent() / (TSRS::H() * fabs(Particle.GetQ()) * TSRS::Mu0() * TSRS::C()) * 1e-6 * 0.001;

  // Constants to send in to GPU
  double *d_C0, *d_C2, *d_EvToOmega, *d_C;

  // Allocate memory for constants
  cudaMalloc((void **) &d_C0,        sizeof(double));
  cudaMalloc((void **) &d_C2,        sizeof(double));
  cudaMalloc((void **) &d_EvToOmega, sizeof(double));
  cudaMalloc((void **) &d_C,         sizeof(double));

  // Copy constants to GPU
  cudaMemcpy(d_C0,        &C0,        sizeof(double), cudaMemcpyHostToDevice);
  cudaMemcpy(d_C2,        &C2,        sizeof(double), cudaMemcpyHostToDevice);
  cudaMemcpy(d_EvToOmega, &EvToOmega, sizeof(double), cudaMemcpyHostToDevice);
  cudaMemcpy(d_C,         &C,         sizeof(double), cudaMemcpyHostToDevice);


  // Send computation to gpu
  int const NThreadsPerBlock = 64;
  int const NBlocks = NSPoints / NThreadsPerBlock + 1;
  SRS_Cuda_SpectrumGPU<<<NBlocks, NThreadsPerBlock>>>(d_x, d_y, d_z, d_bx, d_by, d_bz, d_ox, d_oy, d_oz, d_dt, d_nt, d_ns, d_C0, d_C2, d_EvToOmega, d_C, d_se, d_sf);

  // Copy result back from GPU
  cudaMemcpy(sf, d_sf, size_s, cudaMemcpyDeviceToHost);



  // Add result to power density container
  for (size_t i = 0; i < NSPoints; ++i) {
    Spectrum.SetFlux(i, sf[i] * Weight);
  }


  // Free all gpu memory
  cudaFree(d_x);
  cudaFree(d_y);
  cudaFree(d_z);

  cudaFree(d_bx);
  cudaFree(d_by);
  cudaFree(d_bz);

  cudaFree(d_ox);
  cudaFree(d_oy);
  cudaFree(d_oz);

  cudaFree(d_dt);
  cudaFree(d_nt);
  cudaFree(d_ns);

  cudaFree(d_se);
  cudaFree(d_sf);

  cudaFree(d_C0);
  cudaFree(d_C2);
  cudaFree(d_EvToOmega);
  cudaFree(d_C);


  // Free all heap memory
  delete [] x;
  delete [] y;
  delete [] z;

  delete [] bx;
  delete [] by;
  delete [] bz;


  delete [] se;
  delete [] sf;


  return;
}


















__global__ void SRS_Cuda_PowerDensityGPU (double *x, double *y, double *z, double *bx, double *by, double *bz, double *aocx, double *aocy, double *aocz, double *sx, double *sy, double *sz, double *snx, double *sny, double *snz, double *dt, int *nt, int *ns, double *power_density)
{
  // Get surface id from block and thread number
  int is = threadIdx.x + blockIdx.x * blockDim.x;

  if (is >= *ns) {
    return;
  }




  // If you could copy int ultra-fast memory, something like this:
  //__shared__ double temp[6144];
  //if (threadIdx.x == 0) {
  //  for (int i = 0; i < *nt; ++i) {
  //    if (i <= 6144) {
  //      break;
  //    }
  //    temp[i] = x[i];
  //  }
  //}
  // __syncthreads();



  // Observation point
  double const OX = sx[is];
  double const OY = sy[is];
  double const OZ = sz[is];

  // Normal vector from input
  double const NormalX = snx[is];
  double const NormalY = sny[is];
  double const NormalZ = snz[is];

  double Sum = 0;

  for (int i = 0; i < *nt; ++i) {

    // Normal vector in direction of observation point
    double const R1 = sqrt( pow(OX - x[i], 2) + pow(OY - y[i], 2) + pow(OZ - z[i], 2) );
    double const N1X = (OX - x[i]) / R1;
    double const N1Y = (OY - y[i]) / R1;
    double const N1Z = (OZ - z[i]) / R1;

    // Surface normal dot with vector normal
    double const N1DotNormal = N1X * NormalX + N1Y * NormalY + N1Z * NormalZ;

    // Orthogonal vector 2 & 3
    double N2X;
    double N2Y;
    double N2Z;

    double const xx = N1X < 0.0 ? -N1X : N1X;
    double const yy = N1Y < 0.0 ? -N1Y : N1Y;
    double const zz = N1Z < 0.0 ? -N1Z : N1Z;
    if (xx < yy) {
      if (xx < zz) {
        N2X = 0;
        N2Y = N1Z;
        N2Z = -N1Y;
      } else {
        N2X = N1Y;
        N2Y = -N1X;
        N2Z = 0;
      }
    } else {
      if (yy < zz) {
        N2X = -N1Z;
        N2Y = 0;
        N2Z = N1X;
      } else {
        N2X = N1Y;
        N2Y = -N1X;
        N2Z = 0;
      }
    }
    double const R2 = sqrt(N2X * N2X + N2Y * N2Y + N2Z * N2Z);
    N2X /= R2;
    N2Y /= R2;
    N2Z /= R2;

    // Ortohgonal vector N3
    double const N3X = N1Y * N2Z - N1Z * N2Y;
    double const N3Y = N1Z * N2X - N1X * N2Z;
    double const N3Z = N1X * N2Y - N1Y * N2X;





    double const x1 = N1X - bx[i];
    double const y1 = N1Y - by[i];
    double const z1 = N1Z - bz[i];

    double const x2 = y1 * aocz[i] - z1 * aocy[i];
    double const y2 = z1 * aocx[i] - x1 * aocz[i];
    double const z2 = x1 * aocy[i] - y1 * aocx[i];

    // Numerator = N1.Cross( ( (N1 - B).Cross((AoverC)) ) );
    double const x3 = N1Y * z2 - N1Z * y2;
    double const y3 = N1Z * x2 - N1X * z2;
    double const z3 = N1X * y2 - N1Y * x2;

    double const BdotN1 = bx[i] * N1X + by[i] * N1Y + bz[i] * N1Z;
    double const Denominator = pow(1. - BdotN1, 5);

    Sum += pow( x3 * N2X + y3 * N2Y + z3 * N2Z, 2) / Denominator / (R1 * R1) * N1DotNormal;
    Sum += pow( x3 * N3X + y3 * N3Y + z3 * N3Z, 2) / Denominator / (R1 * R1) * N1DotNormal;
  }

  power_density[is] = Sum * (*dt);

  return;
}



void SRS_Cuda_CalculatePowerDensityGPU (TParticleA& Particle, TSurfacePoints const& Surface, T3DScalarContainer& PowerDensityContainer, int const Dimension, bool const Directional, std::string const& OutFileName)
{

  int ngpu = 0;
  cudaGetDeviceCount(&ngpu);
  if (ngpu == 0) {
    throw std::invalid_argument("No GPU found");
  }

  std::cout << "ngpu " << ngpu << std::endl;




  // Grab the Trajectory
  TParticleTrajectoryPoints& T = Particle.GetTrajectory();

  // Number of points in Trajectory
  int const NTPoints = (int) T.GetNPoints();

  // Timestep from trajectory
  double const DeltaT = T.GetDeltaT();

  double *x     = new double[NTPoints];
  double *y     = new double[NTPoints];
  double *z     = new double[NTPoints];
  double *bx    = new double[NTPoints];
  double *by    = new double[NTPoints];
  double *bz    = new double[NTPoints];
  double *aocx  = new double[NTPoints];
  double *aocy  = new double[NTPoints];
  double *aocz  = new double[NTPoints];

  int const NSPoints = (int) Surface.GetNPoints();

  double *sx     = new double[NSPoints];
  double *sy     = new double[NSPoints];
  double *sz     = new double[NSPoints];

  double *snx    = new double[NSPoints];
  double *sny    = new double[NSPoints];
  double *snz    = new double[NSPoints];

  double *power_density = new double[NSPoints];


  for (size_t i = 0; i < NTPoints; ++i) {
    x[i] = T.GetX(i).GetX();
    y[i] = T.GetX(i).GetY();
    z[i] = T.GetX(i).GetZ();

    bx[i] = T.GetB(i).GetX();
    by[i] = T.GetB(i).GetY();
    bz[i] = T.GetB(i).GetZ();

    aocx[i] = T.GetAoverC(i).GetX();
    aocy[i] = T.GetAoverC(i).GetY();
    aocz[i] = T.GetAoverC(i).GetZ();
  }



  for (size_t i = 0; i < NSPoints; ++i) {
    sx[i] = Surface.GetPoint(i).GetX();
    sy[i] = Surface.GetPoint(i).GetY();
    sz[i] = Surface.GetPoint(i).GetZ();

    snx[i] = Surface.GetPoint(i).GetNormalX();
    sny[i] = Surface.GetPoint(i).GetNormalY();
    snz[i] = Surface.GetPoint(i).GetNormalZ();
  }



  double *d_x, *d_y, *d_z;
  double *d_bx, *d_by, *d_bz;
  double *d_aocx, *d_aocy, *d_aocz;
  double *d_sx, *d_sy, *d_sz;
  double *d_snx, *d_sny, *d_snz;
  double *d_power_density;
  double *d_dt;
  int    *d_nt, *d_ns;

  int const size_x = NTPoints * sizeof(double);
  int const size_s = NSPoints * sizeof(double);

  cudaMalloc((void **) &d_x, size_x);
  cudaMalloc((void **) &d_y, size_x);
  cudaMalloc((void **) &d_z, size_x);

  cudaMalloc((void **) &d_bx, size_x);
  cudaMalloc((void **) &d_by, size_x);
  cudaMalloc((void **) &d_bz, size_x);

  cudaMalloc((void **) &d_aocx, size_x);
  cudaMalloc((void **) &d_aocy, size_x);
  cudaMalloc((void **) &d_aocz, size_x);

  cudaMalloc((void **) &d_sx, size_s);
  cudaMalloc((void **) &d_sy, size_s);
  cudaMalloc((void **) &d_sz, size_s);

  cudaMalloc((void **) &d_snx, size_s);
  cudaMalloc((void **) &d_sny, size_s);
  cudaMalloc((void **) &d_snz, size_s);

  cudaMalloc((void **) &d_power_density, size_s);

  cudaMalloc((void **) &d_dt, sizeof(double));

  cudaMalloc((void **) &d_nt, sizeof(int));
  cudaMalloc((void **) &d_ns, sizeof(int));


  cudaMemcpy(d_x, x, size_x, cudaMemcpyHostToDevice);
  cudaMemcpy(d_y, y, size_x, cudaMemcpyHostToDevice);
  cudaMemcpy(d_z, z, size_x, cudaMemcpyHostToDevice);

  cudaMemcpy(d_bx, bx, size_x, cudaMemcpyHostToDevice);
  cudaMemcpy(d_by, by, size_x, cudaMemcpyHostToDevice);
  cudaMemcpy(d_bz, bz, size_x, cudaMemcpyHostToDevice);

  cudaMemcpy(d_aocx, aocx, size_x, cudaMemcpyHostToDevice);
  cudaMemcpy(d_aocy, aocy, size_x, cudaMemcpyHostToDevice);
  cudaMemcpy(d_aocz, aocz, size_x, cudaMemcpyHostToDevice);

  cudaMemcpy(d_sx, sx, size_s, cudaMemcpyHostToDevice);
  cudaMemcpy(d_sy, sy, size_s, cudaMemcpyHostToDevice);
  cudaMemcpy(d_sz, sz, size_s, cudaMemcpyHostToDevice);

  cudaMemcpy(d_snx, snx, size_s, cudaMemcpyHostToDevice);
  cudaMemcpy(d_sny, sny, size_s, cudaMemcpyHostToDevice);
  cudaMemcpy(d_snz, snz, size_s, cudaMemcpyHostToDevice);

  cudaMemcpy(d_dt, &DeltaT, sizeof(double), cudaMemcpyHostToDevice);

  cudaMemcpy(d_nt, &NTPoints, sizeof(int), cudaMemcpyHostToDevice);
  cudaMemcpy(d_ns, &NSPoints, sizeof(int), cudaMemcpyHostToDevice);


  // Send computation to gpu
  int const NThreadsPerBlock = 64;
  int const NBlocks = NSPoints / NThreadsPerBlock + 1;
  SRS_Cuda_PowerDensityGPU<<<NBlocks, NThreadsPerBlock>>>(d_x, d_y, d_z, d_bx, d_by, d_bz, d_aocx, d_aocy, d_aocz, d_sx, d_sy, d_sz, d_snx, d_sny, d_snz, d_dt, d_nt, d_ns, d_power_density);

  // Copy result back from GPU
  cudaMemcpy(power_density, d_power_density, size_s, cudaMemcpyDeviceToHost);



  // Add result to power density container
  for (size_t i = 0; i < NSPoints; ++i) {
    PowerDensityContainer.AddPoint(TVector3D(sx[i], sy[i], sz[i]), power_density[i] * fabs(Particle.GetQ() * Particle.GetCurrent()) / (16 * TSRS::Pi2() * TSRS::Epsilon0() * TSRS::C()) / 1e6);
  }


  // Free all gpu memory
  cudaFree(d_x);
  cudaFree(d_y);
  cudaFree(d_z);

  cudaFree(d_bx);
  cudaFree(d_by);
  cudaFree(d_bz);

  cudaFree(d_aocx);
  cudaFree(d_aocy);
  cudaFree(d_aocz);

  cudaFree(d_sx);
  cudaFree(d_sy);
  cudaFree(d_sz);

  cudaFree(d_snx);
  cudaFree(d_sny);
  cudaFree(d_snz);

  cudaFree(d_dt);
  cudaFree(d_nt);
  cudaFree(d_ns);

  cudaFree(d_power_density);





  // Free all heap memory
  delete [] x;
  delete [] y;
  delete [] z;

  delete [] bx;
  delete [] by;
  delete [] bz;

  delete [] aocx;
  delete [] aocy;
  delete [] aocz;

  delete [] sx;
  delete [] sy;
  delete [] sz;

  delete [] snx;
  delete [] sny;
  delete [] snz;

  delete [] power_density;

  return;
}





