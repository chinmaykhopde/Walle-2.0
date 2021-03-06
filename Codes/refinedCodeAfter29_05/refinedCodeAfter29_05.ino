/**
	Self-Balancing Bot
	Demonstrates the working of Balancing Algorithm. 

	For more information visit http://sra.vjti.info/

	This code is in the public domain.

	modified 29 May, 2017
	by Society Of Robotics And Automation, VJTI.
*/

#include <MPU.h>
#include <FILTER.h>
#include <SRA16.h>

//PID Params
#define KP_CONST 1
#define KI_CONST 0
#define KD_CONST 0.01

//Filter Params
struct raw_data raw_values;
struct initial_data	init_data;

float angleY = 0;

struct pid_controller{
	int   kp;
	int   ki;
	float kd;
	float error;
	float derivative;
	float prop; 
	float prev_error;
	long  cumulative_error;
};

pid_controller balancing_pid_ctrl = {102, 0, 0.1, 0, 0, 0, 0, 0};

void setup()
{
	/*	Begin Serial Communication between SRA-Board and CP2102	*/
	Serial.begin(9600);

	/*	Belongs to the MPU module	
		Wakes up the MPU with particular settings	*/
	start_mpu();                                                      

	/*	Belongs to the MPU module
		Gets the initial values of the angular velocity using the gyroscope and
		angle using the accelerometer, with respect to which new angles will be calculated	*/
	calibrate_mpu(&init_data);         

	/*	Initializes the motors and PWM (PWM1)	*/
	bot_motion_init(); 

	/*	Belongs to the sra16 module
		Initializes switches on the SRA-Board	*/
	switch_init();                                                  

	//  HARD CODED VALUES
	init_data.acce_angle	= 0.25;
	init_data.gyro_reading	= -15;

	/*	Used for tuning values of kp, and kd using the available Potentiometers	*/
	tune_pid_pot(&balancing_pid_ctrl);  
}

void loop()
{
	/*  This function belongs to the MPU modulue,
		it reads raw values of the accelerometer and gyroscope's registers from the MPU	*/	
	read_raw_values_mpu(&raw_values); 
	
	/*  This function belongs to the Filter Module and it calculates the current angle about the Y-Axis.
		This marks the connection between the MPU module and the Filter module	*/
	calc_angle(&angleY, raw_values, init_data, COMPLEMENTARY);                
	
	/*	This function calculates the error using the angle about Y-Axis i.e. angleY
		The value of angleY is obtained from the calc angle function
		This marks the connection between the Filter module and the PID controller	*/
	calc_error_using_pid(&balancing_pid_ctrl, angleY);    
	
	/*	This function gives instructions to the motor to balance according
		to the error obtained from the previous calc_error_using_pid() function
		This marks the connection between the PID controller anf the Balancing algorithm	*/	
	balance(balancing_pid_ctrl.error); 

	print_data();
}

void print_data()
{
//    Serial.print(raw_values.raw_acce_x);Serial.print("\t");
//    Serial.print(raw_values.raw_acce_y);Serial.print("\t");
//    Serial.print(raw_values.raw_acce_z);Serial.print("\t");
//    Serial.print(raw_values.temp);Serial.print("\t");
//    Serial.print(raw_values.raw_gyro_x);Serial.print("\t");
//    Serial.print(raw_values.raw_gyro_y);Serial.print("\t");
//    Serial.print(raw_values.raw_gyro_z);Serial.print("\t");
    Serial.print(angleY);Serial.print("\t");

//    Serial.print(balancing_pid_ctrl.prop);Serial.print("\t");
//    Serial.print(balancing_pid_ctrl.derivative);Serial.print("\t");
//    Serial.print(balancing_pid_ctrl.cumulative_error);Serial.print("\t");
//    Serial.print(balancing_pid_ctrl.error);Serial.print("\t");
//    Serial.print(balancing_pid_ctrl.prev_error);Serial.print("\t");
	  Serial.println();
}

//	SETTING MOTOR SPEEDS
void left_motorspeed(int a)
{
	set_pwm1a(a); 
}

void right_motorspeed(int b)
{
    set_pwm1b(b); 
}

//	CALCULATION OF ERROR USING PID
void calc_error_using_pid(struct pid_controller* pid, float theta)
{
    pid->prop              = theta;                                                  //  WE PLACE THE MPU SUCH THAT THE BOT TILTS ABOUT Y-AXIS
    pid->error             = pid->kp * pid->prop + pid->ki * pid->cumulative_error + pid->kd * pid->derivative;
    pid->derivative        = pid->error - pid->prev_error;
    pid->cumulative_error += pid->error;
    pid->prev_error        = pid->error;
}

//	BALANCING FUNCTION
void balance(float error)
{
    static long abs_error = abs(error);

	if(error > 0) {
		bot_backward();
	}
	else if(error < 0) {
		bot_forward();
	}
	else bot_brake();

	if(abs_error > 399) {
		abs_error = 399;
	}

    left_motorspeed(abs_error);
    right_motorspeed(abs_error);
}

//	PID TUNING WITH POT
void tune_pid_pot(struct pid_controller* pid)
{
	Serial.println("TUNING STARTED");
	short done_tuning = 0;

	while(!done_tuning) { 
		pid->kp = adc_start(4) * KP_CONST;
		pid->kd = adc_start(5) * KD_CONST;

		Serial.print(done_tuning);	Serial.print("\t");
		Serial.print("kp:\t");		Serial.print(pid->kp);
		Serial.print("\tkd:\t");	Serial.print(pid->kd, 4);
		Serial.println();

		if(pressed_switch0()) {
			done_tuning = 1;
		}
	}
}

