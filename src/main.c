#include "stm32f10x.h"
#include "pid.h"

#define PID_TEST

//	Output (e.g PWM Register)
#define MOTOR1 outputRegister_1
#define MOTOR2 outputRegister_2

__IO uint16_t outputRegister_1;
__IO uint16_t outputRegister_2;

//	Input (e.g Source data from IMU)

float gyroInput;
float g_Roll;



pid_st g_pid_roll = {
	.InnerLast  = 0,
	.OutterLast = 0,
	.Feedback   = &g_Roll,
	.i          = 0,
	.Channel1   = &MOTOR1,
	.Channel2   = &MOTOR2,
	.Gyro       = &gyroInput,
};


int main() {

	#ifdef PID_TEST
		while(1) {
			pid_SingleAxis(&g_pid_roll, 0);
		}
	#endif

	while(1);
}
