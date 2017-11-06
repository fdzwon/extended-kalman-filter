#include "FusionEKF.h"
#include "tools.h"
#include "Eigen/Dense"
#include <iostream>
#include <cmath>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/*
 * Constructor.
 */
FusionEKF::FusionEKF() {
  is_initialized_ = false;

  previous_timestamp_ = 0;

  // initializing matrices
  R_laser_ = MatrixXd(2, 2);
  R_radar_ = MatrixXd(3, 3);
  H_laser_ = MatrixXd(2, 4);
  Hj_ = MatrixXd(3, 4);

  //measurement covariance matrix - laser
  R_laser_ << 0.0225, 0,
        0, 0.0225;

  //measurement covariance matrix - radar
  R_radar_ << 0.09, 0, 0,
        0, 0.0009, 0,
        0, 0, 0.09;

  /*
    * Set the process and measurement noises
  */

  H_laser_ << 1, 0, 0, 0,
	  0, 1, 0, 0;

  Hj_ << 1, 1, 0, 0,
	  1, 1, 0, 0,
	  1, 1, 1, 1, 1;

}

/**
* Destructor.
*/
FusionEKF::~FusionEKF() {}

void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) {


  /*****************************************************************************
   *  Initialization
   ****************************************************************************/
  if (!is_initialized_) {
    /**
      * Initialize the state ekf_.x_ with the first measurement.
      * Create the covariance matrix.
      * convert radar from polar to cartesian coordinates.
    */
    // first measurement
    cout << "EKF: " << endl;
    ekf_.x_ = VectorXd(4);
    ekf_.x_ << 1, 1, 1, 1;

	ekf_.F_ = MatrixXd(4, 4);
	ekf_.F_ << 1, 0, 1, 0,
		0, 1, 0, 1,
		0, 0, 1, 0,
		0, 0, 0, 1;

	ekf_.P_ = MatrixXd(4, 4);
	ekf_.P_ << 1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1000, 0,
		0, 0, 0, 1000;

	ekf_.Q_ = MatrixXd(4, 4);
	ekf_.Q_ << 1, 0, 1, 0,
		0, 1, 0, 1,
		1, 0, 1, 0,
		0, 1, 0, 1;

    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
      /**
      Convert radar from polar to cartesian coordinates and initialize state.
      */
		const double pi = 3.1415926535897;
		double rho = measurement_pack.raw_measurements_(0);
		double phi = measurement_pack.raw_measurements_(1);
		double dot_rho = measurement_pack.raw_measurements_(2);
		double px = rho * cos(phi);
		double py = rho *sin(phi);
		ekf_.x_(0) = px;
		ekf_.x_(1) = py;
    }
    else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
      /**
      Initialize state.
      */
		ekf_.x_(0) = measurement_pack.raw_measurements_(0);
		ekf_.x_(1) = measurement_pack.raw_measurements_(1);

    }


    // done initializing, no need to predict or update
    is_initialized_ = true;
    return;
  }

  /*****************************************************************************
   *  Prediction
   ****************************************************************************/

  /**
     * Update the state transition matrix F according to the new elapsed time.
      - Time is measured in seconds.
     * Update the process noise covariance matrix.
     * Use noise_ax = 9 and noise_ay = 9 for your Q matrix.
   */
  double dt = (previous_timestamp_ - measurement_pack.timestamp_) / 1000000.0;
  previous_timestamp_ = measurement_pack.timestamp_;

  double dt_sq = dt*dt;
  float noise_ax = 9.0;
  float noise_ay = 9.0;
  ekf_.F_(0, 2) = dt;
  ekf_.F_(1, 3) = dt;
  
  ekf_.Q_(0, 0) = (dt_sq*dt_sq) / 4.0 * noise_ax;
  ekf_.Q_(0, 2) = (dt_sq*dt) / 2.0 * noise_ax;
  ekf_.Q_(1, 1) = (dt_sq*dt_sq) / 4.0 * noise_ay;
  ekf_.Q_(1, 3) = (dt_sq*dt) / 2.0 * noise_ay;
  ekf_.Q_(2, 0) = (dt_sq*dt) / 2.0 * noise_ax;
  ekf_.Q_(2, 2) = (dt_sq*noise_ax);
  ekf_.Q_(3, 1) = (dt_sq*dt) / 2.0 * noise_ay;
  ekf_.Q_(2, 2) = (dt_sq*noise_ay);



  ekf_.Predict();

  /*****************************************************************************
   *  Update
   ****************************************************************************/

  /**
     * Use the sensor type to perform the update step.
     * Update the state and covariance matrices.
   */

  if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
    // Radar updates
	  Hj_ = tools.CalculateJacobian(ekf_.x_);
	  ekf_.H_ = Hj_;
	  ekf_.R_ = R_radar_;
	  ekf_.UpdateEKF(measurement_pack.raw_measurements_);
  } else {
    // Laser updates
	  ekf_.H_ = H_laser_;
	  ekf_.R_ = R_laser_;
	  ekf_.Update(measurement_pack.raw_measurements_);
  }

  // print the output
  cout << "x_ = " << ekf_.x_ << endl;
  cout << "P_ = " << ekf_.P_ << endl;
}
