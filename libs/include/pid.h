#ifndef PID_H
#define PID_H

extern float g_Yaw, g_Pitch, g_Roll;     //eular

#define OUTTER_LOOP_KP 0 //0.257 * 0.83 0.255
#define OUTTER_LOOP_KI 0
#define OUTTER_LOOP_KD 0

#define INNER_LOOP_KP 0.03f
#define INNER_LOOP_KI 0
#define INNER_LOOP_KD 0

#define SUM_ERRO_MAX 900
#define SUM_ERRO_MIN -900

#define PID_IMAX 30
#define PID_IMIN -30
//	限幅
#define THROTTLE_MAX (unsigned short)3600	//2ms	-	top position of throttle
#define THROTTLE_MIN (unsigned short)1620	//0.9ms	-	bottom position of throttle


typedef struct {
    float InnerLast;			//保存内环旧值以便后向差分
    float OutterLast;			//保存外环旧值以便后向差分
    float *Feedback;			//反馈数据, 实时的角度数据
    float *Gyro;				//角速度
    float Error;				//误差值
    float p;					//比例项(内环外环共用)
    float i;					//积分项(仅用于外环)
    float d;					//微分项(内环外环共用)
    short output;				//PID输出, 用来修改PWM值, 2字节
    __IO uint16_t *Channel1;	//PWM输出, 通道1
    __IO uint16_t *Channel2;	//PWM输出, 通道2
} pid_st, *pid_pst;

void pid_SingleAxis(pid_pst package, float setPoint);

#endif
