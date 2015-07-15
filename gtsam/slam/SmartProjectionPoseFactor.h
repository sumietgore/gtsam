/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation,
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file   SmartProjectionPoseFactor.h
 * @brief  Smart factor on poses, assuming camera calibration is fixed
 * @author Luca Carlone
 * @author Chris Beall
 * @author Zsolt Kira
 */

#pragma once

#include <gtsam/slam/SmartProjectionFactor.h>

namespace gtsam {
/**
 *
 * @addtogroup SLAM
 *
 * If you are using the factor, please cite:
 * L. Carlone, Z. Kira, C. Beall, V. Indelman, F. Dellaert, Eliminating conditionally
 * independent sets in factor graphs: a unifying perspective based on smart factors,
 * Int. Conf. on Robotics and Automation (ICRA), 2014.
 *
 */

/**
 * This factor assumes that camera calibration is fixed, and that
 * the calibration is the same for all cameras involved in this factor.
 * The factor only constrains poses (variable dimension is 6).
 * This factor requires that values contains the involved poses (Pose3).
 * If the calibration should be optimized, as well, use SmartProjectionFactor instead!
 * @addtogroup SLAM
 */
template<class CALIBRATION>
class SmartProjectionPoseFactor: public SmartProjectionFactor<
    PinholePose<CALIBRATION> > {

private:
  typedef PinholePose<CALIBRATION> Camera;
  typedef SmartProjectionFactor<Camera> Base;
  typedef SmartProjectionPoseFactor<CALIBRATION> This;

protected:

  boost::shared_ptr<CALIBRATION> K_; ///< calibration object (one for all cameras)

public:

  /// shorthand for a smart pointer to a factor
  typedef boost::shared_ptr<This> shared_ptr;

  /**
   * Constructor
   * @param K (fixed) calibration, assumed to be the same for all cameras
   * @param body_P_sensor pose of the camera in the body frame
   * @param params internal parameters of the smart factors
   */
  SmartProjectionPoseFactor(const boost::shared_ptr<CALIBRATION> K,
      const boost::optional<Pose3> body_P_sensor = boost::none,
      const SmartProjectionParams& params = SmartProjectionParams()) :
      Base(body_P_sensor, params), K_(K) {
  }

  /** Virtual destructor */
  virtual ~SmartProjectionPoseFactor() {
  }

  /**
   * print
   * @param s optional string naming the factor
   * @param keyFormatter optional formatter useful for printing Symbols
   */
  void print(const std::string& s = "", const KeyFormatter& keyFormatter =
      DefaultKeyFormatter) const {
    std::cout << s << "SmartProjectionPoseFactor, z = \n ";
    Base::print("", keyFormatter);
  }

  /// equals
  virtual bool equals(const NonlinearFactor& p, double tol = 1e-9) const {
    const This *e = dynamic_cast<const This*>(&p);
    return e && Base::equals(p, tol);
  }

  /**
   * error calculates the error of the factor.
   */
  virtual double error(const Values& values) const {
    if (this->active(values)) {
      return this->totalReprojectionError(cameras(values));
    } else { // else of active flag
      return 0.0;
    }
  }

  /** return calibration shared pointers */
  inline const boost::shared_ptr<CALIBRATION> calibration() const {
    return K_;
  }

  /**
   * Collect all cameras involved in this factor
   * @param values Values structure which must contain camera poses corresponding
   * to keys involved in this factor
   * @return vector of Values
   */
  typename Base::Cameras cameras(const Values& values) const {
    typename Base::Cameras cameras;
    BOOST_FOREACH(const Key& k, this->keys_) {
      Pose3 pose = values.at<Pose3>(k);
      if (Base::body_P_sensor_)
        pose = pose.compose(*(Base::body_P_sensor_));

      Camera camera(pose, K_);
      cameras.push_back(camera);
    }
    return cameras;
  }

private:

  /// Serialization function
  friend class boost::serialization::access;
  template<class ARCHIVE>
  void serialize(ARCHIVE & ar, const unsigned int /*version*/) {
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Base);
    ar & BOOST_SERIALIZATION_NVP(K_);
  }

};
// end of class declaration

/// traits
template<class CALIBRATION>
struct traits<SmartProjectionPoseFactor<CALIBRATION> > : public Testable<
    SmartProjectionPoseFactor<CALIBRATION> > {
};

} // \ namespace gtsam
