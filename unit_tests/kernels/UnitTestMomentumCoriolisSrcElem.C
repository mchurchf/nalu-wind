/*------------------------------------------------------------------------*/
/*  Copyright 2014 National Renewable Energy Laboratory.                  */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level Nalu          */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/

#include "kernels/UnitTestKernelUtils.h"
#include "UnitTestUtils.h"

#include "CoriolisSrc.h"
#include "MomentumCoriolisSrcElemKernel.h"
#include "BucketLoop.h"

#include <random>

TEST_F(MomentumKernelHex8Mesh, coriolis)
{
  fill_mesh_and_init_fields();

  // simplify ICs
  stk::mesh::field_fill(1.0, *velocity_);
  stk::mesh::field_fill(1.0, *density_);

  // Setup solution options for default kernel
  solnOpts_.meshMotion_ = false;
  solnOpts_.meshDeformation_ = false;
  solnOpts_.externalMeshDeformation_ = false;

  // Setup Coriolis specific options.  Mimics the Ekman spiral test
  solnOpts_.earthAngularVelocity_ = 7.2921159e-5;
  solnOpts_.latitude_ = 30.0;
  solnOpts_.eastVector_ = { 1.0, 0.0, 0.0 };
  solnOpts_.northVector_ = { 0.0, 1.0, 0.0 };

  sierra::nalu::CoriolisSrc cor(solnOpts_);
  EXPECT_NEAR(cor.upVector_[0], 0.0, tol);
  EXPECT_NEAR(cor.upVector_[1], 0.0, tol);
  EXPECT_NEAR(cor.upVector_[2], 1.0, tol);

  // Initialize the kernel driver
  unit_test_kernel_utils::TestKernelDriver assembleKernels(
    bulk_, partVec_, coordinates_, spatialDim_, stk::topology::HEX_8);

  // Initialize the kernel
  std::unique_ptr<sierra::nalu::Kernel> kernel(
    new sierra::nalu::MomentumCoriolisSrcElemKernel<sierra::nalu::AlgTraitsHex8>(
      bulk_, solnOpts_, velocity_, assembleKernels.dataNeededByKernels_, false)
  );

  // Add to kernels to be tested
  assembleKernels.activeKernels_.push_back(kernel.get());

  assembleKernels.execute();

  EXPECT_EQ(assembleKernels.lhs_.dimension(0), 24u);
  EXPECT_EQ(assembleKernels.lhs_.dimension(1), 24u);
  EXPECT_EQ(assembleKernels.rhs_.dimension(0), 24u);


  // TEST: checks consistency with the Jacobian in CoriolisSrc

  // Exact solution
  std::vector<double> rhsExact(24,0.0);
  for (int n = 0; n < 8; ++n) {
    int nnDim = n * 3;
    rhsExact[nnDim + 0] = 0.125 * (+cor.Jxy_  + cor.Jxz_);
    rhsExact[nnDim + 1] = 0.125 * (-cor.Jxy_  + cor.Jyz_);
    rhsExact[nnDim + 2] = 0.125 * (-cor.Jxz_  - cor.Jyz_);
  }
  unit_test_kernel_utils::expect_all_near(assembleKernels.rhs_, rhsExact.data());
}