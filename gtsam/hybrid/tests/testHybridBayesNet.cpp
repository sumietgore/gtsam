/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file    testHybridBayesNet.cpp
 * @brief   Unit tests for HybridBayesNet
 * @author  Varun Agrawal
 * @author  Fan Jiang
 * @author  Frank Dellaert
 * @date    December 2021
 */

#include <gtsam/base/serializationTestHelpers.h>
#include <gtsam/hybrid/HybridBayesNet.h>
#include <gtsam/hybrid/HybridBayesTree.h>
#include <gtsam/nonlinear/NonlinearFactorGraph.h>

#include "Switching.h"

// Include for test suite
#include <CppUnitLite/TestHarness.h>

using namespace std;
using namespace gtsam;
using namespace gtsam::serializationTestHelpers;

using noiseModel::Isotropic;
using symbol_shorthand::M;
using symbol_shorthand::X;

static const DiscreteKey Asia(0, 2);

/* ****************************************************************************/
// Test creation
TEST(HybridBayesNet, Creation) {
  HybridBayesNet bayesNet;

  bayesNet.add(Asia, "99/1");

  DiscreteConditional expected(Asia, "99/1");

  CHECK(bayesNet.atDiscrete(0));
  auto& df = *bayesNet.atDiscrete(0);
  EXPECT(df.equals(expected));
}

/* ****************************************************************************/
// Test adding a bayes net to another one.
TEST(HybridBayesNet, Add) {
  HybridBayesNet bayesNet;

  bayesNet.add(Asia, "99/1");

  DiscreteConditional expected(Asia, "99/1");

  HybridBayesNet other;
  other.push_back(bayesNet);
  EXPECT(bayesNet.equals(other));
}

/* ****************************************************************************/
// Test choosing an assignment of conditionals
TEST(HybridBayesNet, Choose) {
  Switching s(4);

  Ordering ordering;
  for (auto&& kvp : s.linearizationPoint) {
    ordering += kvp.key;
  }

  HybridBayesNet::shared_ptr hybridBayesNet;
  HybridGaussianFactorGraph::shared_ptr remainingFactorGraph;
  std::tie(hybridBayesNet, remainingFactorGraph) =
      s.linearizedFactorGraph.eliminatePartialSequential(ordering);

  DiscreteValues assignment;
  assignment[M(1)] = 1;
  assignment[M(2)] = 1;
  assignment[M(3)] = 0;

  GaussianBayesNet gbn = hybridBayesNet->choose(assignment);

  EXPECT_LONGS_EQUAL(4, gbn.size());

  EXPECT(assert_equal(*(*boost::dynamic_pointer_cast<GaussianMixture>(
                          hybridBayesNet->atMixture(0)))(assignment),
                      *gbn.at(0)));
  EXPECT(assert_equal(*(*boost::dynamic_pointer_cast<GaussianMixture>(
                          hybridBayesNet->atMixture(1)))(assignment),
                      *gbn.at(1)));
  EXPECT(assert_equal(*(*boost::dynamic_pointer_cast<GaussianMixture>(
                          hybridBayesNet->atMixture(2)))(assignment),
                      *gbn.at(2)));
  EXPECT(assert_equal(*(*boost::dynamic_pointer_cast<GaussianMixture>(
                          hybridBayesNet->atMixture(3)))(assignment),
                      *gbn.at(3)));
}

/* ****************************************************************************/
// Test bayes net optimize
TEST(HybridBayesNet, OptimizeAssignment) {
  Switching s(4);

  Ordering ordering;
  for (auto&& kvp : s.linearizationPoint) {
    ordering += kvp.key;
  }

  HybridBayesNet::shared_ptr hybridBayesNet;
  HybridGaussianFactorGraph::shared_ptr remainingFactorGraph;
  std::tie(hybridBayesNet, remainingFactorGraph) =
      s.linearizedFactorGraph.eliminatePartialSequential(ordering);

  DiscreteValues assignment;
  assignment[M(1)] = 1;
  assignment[M(2)] = 1;
  assignment[M(3)] = 1;

  VectorValues delta = hybridBayesNet->optimize(assignment);

  // The linearization point has the same value as the key index,
  // e.g. X(1) = 1, X(2) = 2,
  // but the factors specify X(k) = k-1, so delta should be -1.
  VectorValues expected_delta;
  expected_delta.insert(make_pair(X(1), -Vector1::Ones()));
  expected_delta.insert(make_pair(X(2), -Vector1::Ones()));
  expected_delta.insert(make_pair(X(3), -Vector1::Ones()));
  expected_delta.insert(make_pair(X(4), -Vector1::Ones()));

  EXPECT(assert_equal(expected_delta, delta));
}

/* ****************************************************************************/
// Test bayes net optimize
TEST(HybridBayesNet, Optimize) {
  Switching s(4);

  Ordering hybridOrdering = s.linearizedFactorGraph.getHybridOrdering();
  HybridBayesNet::shared_ptr hybridBayesNet =
      s.linearizedFactorGraph.eliminateSequential(hybridOrdering);

  HybridValues delta = hybridBayesNet->optimize();

  DiscreteValues expectedAssignment;
  expectedAssignment[M(1)] = 1;
  expectedAssignment[M(2)] = 0;
  expectedAssignment[M(3)] = 1;
  EXPECT(assert_equal(expectedAssignment, delta.discrete()));

  VectorValues expectedValues;
  expectedValues.insert(X(1), -0.999904 * Vector1::Ones());
  expectedValues.insert(X(2), -0.99029 * Vector1::Ones());
  expectedValues.insert(X(3), -1.00971 * Vector1::Ones());
  expectedValues.insert(X(4), -1.0001 * Vector1::Ones());

  EXPECT(assert_equal(expectedValues, delta.continuous(), 1e-5));
}

/* ****************************************************************************/
// Test bayes net multifrontal optimize
TEST(HybridBayesNet, OptimizeMultifrontal) {
  Switching s(4);

  Ordering hybridOrdering = s.linearizedFactorGraph.getHybridOrdering();
  HybridBayesTree::shared_ptr hybridBayesTree =
      s.linearizedFactorGraph.eliminateMultifrontal(hybridOrdering);
  HybridValues delta = hybridBayesTree->optimize();

  VectorValues expectedValues;
  expectedValues.insert(X(1), -0.999904 * Vector1::Ones());
  expectedValues.insert(X(2), -0.99029 * Vector1::Ones());
  expectedValues.insert(X(3), -1.00971 * Vector1::Ones());
  expectedValues.insert(X(4), -1.0001 * Vector1::Ones());

  EXPECT(assert_equal(expectedValues, delta.continuous(), 1e-5));
}

/* ****************************************************************************/
// Test bayes net error
TEST(HybridBayesNet, Error) {
  Switching s(3);

  Ordering hybridOrdering = s.linearizedFactorGraph.getHybridOrdering();
  HybridBayesNet::shared_ptr hybridBayesNet =
      s.linearizedFactorGraph.eliminateSequential(hybridOrdering);

  HybridValues delta = hybridBayesNet->optimize();
  auto error_tree = hybridBayesNet->error(delta.continuous());

  std::vector<DiscreteKey> discrete_keys = {{M(1), 2}, {M(2), 2}};
  std::vector<double> leaves = {0.0097568009, 3.3973404e-31, 0.029126214,
                                0.0097568009};
  AlgebraicDecisionTree<Key> expected_error(discrete_keys, leaves);

  // regression
  EXPECT(assert_equal(expected_error, error_tree, 1e-9));

  // Error on pruned bayes net
  auto prunedBayesNet = hybridBayesNet->prune(2);
  auto pruned_error_tree = prunedBayesNet.error(delta.continuous());

  std::vector<double> pruned_leaves = {2e50, 3.3973404e-31, 2e50, 0.0097568009};
  AlgebraicDecisionTree<Key> expected_pruned_error(discrete_keys,
                                                   pruned_leaves);

  // regression
  EXPECT(assert_equal(expected_pruned_error, pruned_error_tree, 1e-9));

  // Verify error computation and check for specific error value
  DiscreteValues discrete_values;
  discrete_values[M(1)] = 1;
  discrete_values[M(2)] = 1;

  double total_error = 0;
  for (size_t idx = 0; idx < hybridBayesNet->size(); idx++) {
    if (hybridBayesNet->at(idx)->isHybrid()) {
      double error = hybridBayesNet->atMixture(idx)->error(delta.continuous(),
                                                           discrete_values);
      total_error += error;
    } else if (hybridBayesNet->at(idx)->isContinuous()) {
      double error = hybridBayesNet->atGaussian(idx)->error(delta.continuous());
      total_error += error;
    }
  }

  EXPECT_DOUBLES_EQUAL(
      total_error, hybridBayesNet->error(delta.continuous(), discrete_values),
      1e-9);
  EXPECT_DOUBLES_EQUAL(total_error, error_tree(discrete_values), 1e-9);
  EXPECT_DOUBLES_EQUAL(total_error, pruned_error_tree(discrete_values), 1e-9);
}

/* ****************************************************************************/
// Test bayes net pruning
TEST(HybridBayesNet, Prune) {
  Switching s(4);

  Ordering hybridOrdering = s.linearizedFactorGraph.getHybridOrdering();
  HybridBayesNet::shared_ptr hybridBayesNet =
      s.linearizedFactorGraph.eliminateSequential(hybridOrdering);

  HybridValues delta = hybridBayesNet->optimize();

  auto prunedBayesNet = hybridBayesNet->prune(2);
  HybridValues pruned_delta = prunedBayesNet.optimize();

  EXPECT(assert_equal(delta.discrete(), pruned_delta.discrete()));
  EXPECT(assert_equal(delta.continuous(), pruned_delta.continuous()));
}

/* ****************************************************************************/
// Test HybridBayesNet serialization.
TEST(HybridBayesNet, Serialization) {
  Switching s(4);
  Ordering ordering = s.linearizedFactorGraph.getHybridOrdering();
  HybridBayesNet hbn = *(s.linearizedFactorGraph.eliminateSequential(ordering));

  EXPECT(equalsObj<HybridBayesNet>(hbn));
  EXPECT(equalsXML<HybridBayesNet>(hbn));
  EXPECT(equalsBinary<HybridBayesNet>(hbn));
}

/* ************************************************************************* */
int main() {
  TestResult tr;
  return TestRegistry::runAllTests(tr);
}
/* ************************************************************************* */
