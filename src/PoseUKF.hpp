#ifndef _UWV_KALMAN_FILTERS_POSE_UKF_HPP_
#define _UWV_KALMAN_FILTERS_POSE_UKF_HPP_

#include <base/Time.hpp>
#include <pose_estimation/UnscentedKalmanFilter.hpp>
#include <pose_estimation/Measurement.hpp>
#include <uwv_dynamic_model/DataTypes.hpp>
#include "PoseState.hpp"
#include "PoseUKFConfig.hpp"
#include <dynamic_model_svr/SVRThreeDOFModel.hpp>

namespace uwv_dynamic_model
{
    class DynamicModel;
}

namespace pose_estimation
{
    class GeographicProjection;
}

namespace uwv_kalman_filters
{

/**
 * This implements a full model aided inertial localization solution for autonomous underwater vehicles.
 *
 * As minimal input the filter relays on rotation rates and accelerations from an IMU and velocities from a DVL.
 * Given force and torque measurements an AUV motion model aids the velocity estimate during DVL drop outs.
 * ADCP measurements further aid the estimation in cases of DVL bottom-lock loss.
 * Given gyroscopes capable of sensing the rotation of the earth (e.g. a fiber optic gyro)
 * this filter is able to estimate it's true heading.
 *
 * NOTE: In this filter the IMU frame is, in order to keep a certain algorithmic simplicity,
 * not considered to be rotated with respect to the body frame.
 * Rotation rates and acceleration, as well as the corresponding configuration parameters
 * therefore would need to be rotated to the body frame before integrating them in this filter.
 */
class PoseUKF : public pose_estimation::UnscentedKalmanFilter<PoseState>
{
public:
    struct PoseUKFParameter
    {
        Eigen::Vector3d imu_in_body;
        Eigen::Vector3d gyro_bias_offset;
        double gyro_bias_tau;
        Eigen::Vector3d acc_bias_offset;
        double acc_bias_tau;
        double inertia_tau;
        double lin_damping_tau;
        double quad_damping_tau;
        double water_velocity_tau; // time constant for water currents
        double water_velocity_limits; //long term 1 sigma bounds for currents
        double water_velocity_scale; // spatial scale for water current change in m/s / m
        double adcp_bias_tau;
        double atmospheric_pressure; // atmospheric pressure in pascal (N/m²)
        double water_density_tau; // long term 1 sigma bound for water density
    };

    MEASUREMENT(GeographicPosition, 2)
    MEASUREMENT(XY_Position, 2)
    MEASUREMENT(Z_Position, 1)
    MEASUREMENT(Pressure, 1)
    MEASUREMENT(RotationRate, 3)
    MEASUREMENT(Acceleration, 3)
    MEASUREMENT(Velocity, 3)
    MEASUREMENT(BodyEffortsMeasurement, 6)
    MEASUREMENT(WaterVelocityMeasurement, 2)
    MEASUREMENT(VisualFeatureMeasurement, 2)

public:
    PoseUKF(const State& initial_state, const Covariance& state_cov,
            const LocationConfiguration& location, const uwv_dynamic_model::UWVParameters& model_parameters,
            const PoseUKFParameter& filter_parameter);
    virtual ~PoseUKF() {}

    /* Latitude and Longitude in WGS 84 in radian.
     * Uncertainty expressed in m on earth surface */
    void integrateMeasurement(const GeographicPosition& geo_position,
                              const Eigen::Vector3d& gps_in_body = Eigen::Vector3d::Zero());

    /* 2D Position expressed in the navigation frame */
    void integrateMeasurement(const XY_Position& xy_position);

    /* Altitude of IMU expressed in the navigation frame */
    void integrateMeasurement(const Z_Position& z_position);

    /* Pressure in liquid in pascal (N/m²) */
    void integrateMeasurement(const Pressure& pressure,
                              const Eigen::Vector3d& pressure_sensor_in_imu = Eigen::Vector3d::Zero());

    /* Rotation rates of IMU expressed in the IMU frame */
    void integrateMeasurement(const RotationRate& rotation_rate);

    /* Accelerations of IMU expressed in the IMU frame */
    void integrateMeasurement(const Acceleration& acceleration);

    /* Velocities expressed in the IMU frame */
    void integrateMeasurement(const Velocity& velocity);

    /* Forces and torques in the body frame */
    void integrateMeasurement(const BodyEffortsMeasurement& body_efforts, bool only_affect_velocity = false);
   
    /* Water Velocities from ADCP expressed in the IMU frame */
    void integrateMeasurement(const WaterVelocityMeasurement& adcp_measurements, double cell_weighting);

    /**
     * The features (usually the four corners) of a visual marker in undistorted image coordinates.
     * |marker_features| and |feature_positions| musst be of equal size and order.
     * @param marker_features image coordinates of the features in the undistorted image.
     * @param feature_positions are the positions of the featues in the marker frame.
     */
    void integrateMeasurement(const std::vector< VisualFeatureMeasurement> &marker_features,
                              const std::vector<Eigen::Vector3d>& feature_positions,
                              const Eigen::Affine3d& marker_pose, const Eigen::Matrix<double,6,6> cov_marker_pose,
                              const CameraConfiguration& camera_config, const Eigen::Affine3d& camera_in_IMU);

    /* Returns rotation rate in IMU frame */
    RotationRate::Mu getRotationRate();

protected:
    void predictionStepImpl(double delta_t);


    boost::shared_ptr<uwv_dynamic_model::DynamicModel> dynamic_model;
    boost::shared_ptr<pose_estimation::GeographicProjection> projection;
    RotationRate::Mu rotation_rate;
    PoseUKFParameter filter_parameter;
    InertiaType::vectorized_type inertia_offset;
    LinDampingType::vectorized_type lin_damping_offset;
    QuadDampingType::vectorized_type quad_damping_offset;
    double water_density_offset;
    boost::shared_ptr<dynamic_model_svr::SVRThreeDOFModel> svrThreeDOFModel;
};

}

#endif
