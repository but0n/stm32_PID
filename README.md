# stm32_PID	[![Build Status](https://travis-ci.org/but0n/stm32_PID.svg?branch=master)](https://travis-ci.org/but0n/stm32_PID)

串级PID

单极PID适合线性系统，当输出量和被控制量呈线性关系时单极PID能获得较好的效果，但是四轴不是线性系统，现代学者认为，四轴通常可以简化为一个二阶阻尼系统。为什么四轴不是线性系统呢？首先，输出的电压和电机转速不是呈正比的，其次，螺旋桨转速和升力是平方倍关系，故单极PID在四轴上很难取得很好效果，能飞，但是不好飞。
为了解决这个问题，我们提出了串级PID这个解决方法。

> 数字 PID 控制的实现必须用数值逼近的方法.
>
> 当采样周期相当短时, 用求和代替积分, 用后向差分代替微分, 使模拟 PID 离散化变为差分方程.

> 数字 PID 算法总体分为位置型控制算法和增量型控制算法.


## 伪代码
> 外环PID

- 计算被控轴(*Pitch, Roll, Yaw*)角度误差 *(实际角度 - 期望角度)*

* *积分项*

	- 对误差结果进行累加 *(用求和代替积分)*
	- 对累加结果进行积分限幅

* *微分项*

	- 使用当前的角度值减去上一次的角度值 *(后向差分)*

* *外环输出*

	- 比例系数 **`P`** x 当前误差 + 积分系数 **`I`** x 积分项 + 微分系数 **`D`** x 积分项
- 保存上一次角度值

> 内环PD

* *比例项*

	- 外环输出 + 该轴角速度 * 3.5
* *微分项*

	- 该轴角速度 - 上一次该轴角速度 *(后向差分)*

* *内环输出*

	- 比例系数`p` x 比例项 + 微分系数`d` x 微分项

- 存储角速度

## Code

先定义结构体以及结构体指针, 把运算过程使用的大量变量封装起来, 在专属函数中对其解析和利用

``` c
typedef struct {
    float InnerLast;		//保存内环旧值以便后向差分
    float OutterLast;		//保存外环旧值以便后向差分
    float *Feedback;		//反馈数据, 实时的角度数据
    float *Gyro;				//角速度
    float Error;				//误差值
    float p;					//比例项(内环外环共用)
    float i;					//积分项(仅用于外环)
    float d;					//微分项(内环外环共用)
    short output;			//PID输出, 用来修改PWM值, 2字节
    __IO uint16_t *Channel1;//PWM输出, 通道1
    __IO uint16_t *Channel2;//PWM输出, 通道2
} pid_st, *pid_pst;
```


``` c
void pid_SingleAxis(pid_pst temp, float setPoint) {
    temp->Error = *temp->Feedback - setPoint;
//Outter Loop PID
    temp->i += temp->Error;
    if (temp->i > PID_IMAX) temp->i = PID_IMAX;
    else if (temp->i < PID_IMIN) temp->i = PID_IMIN;

    temp->d = *temp->Feedback - temp->OutterLast;

    temp->output = (short)(OUTTER_LOOP_KP * (temp->Error) + OUTTER_LOOP_KI * temp->i + OUTTER_LOOP_KD * temp->d);
    temp->OutterLast = *temp->Feedback; //Save Old Data
//Inner Loop PD
    temp->p = temp->output + *temp->Gyro;
    temp->d = *temp->Gyro - temp->InnerLast;
    temp->output = (short)(INNER_LOOP_KP * temp->p + INNER_LOOP_KD * temp->d);

    if (*temp->Channel1+temp->output > THROTTLE_MAX) *temp->Channel1 = THROTTLE_MAX;
    else if (*temp->Channel1+temp->output < THROTTLE_MIN) *temp->Channel1 = THROTTLE_MIN;
    else *temp->Channel1 += (short)temp->output;

    if (*temp->Channel2-temp->output > THROTTLE_MAX) *temp->Channel2 = THROTTLE_MAX;
    else if (*temp->Channel2-temp->output < THROTTLE_MIN) *temp->Channel2 = THROTTLE_MIN;
    else *temp->Channel2 -= (short)temp->output;

    temp->InnerLast = *temp->Gyro;
}
```

## 调参

1. 要在飞机的起飞油门基础上进行PID参数的调整，否则“烤四轴”的时候调试稳定了，飞起来很可能又会晃荡。

2. 内环的参数最为关键！理想的内环参数能够很好地跟随打舵（角速度控制模式下的打舵）控制量。在平衡位置附近（正负30度左右），舵量突加，飞机快速响应；舵量     回中，飞机立刻停止运动（几乎没有回弹和震荡）。

	2.1. 首先改变程序，将角度外环去掉，将打舵量作为内环的期望（角速度模式，在APM中叫ACRO模式，在大疆中叫手动模式）。

	2.2. 加上P，P太小，不能修正角速度误差表现为很“软”倾斜后难以修正，打舵响应也差。P太大，在平衡位置容易震荡，打舵回中或给干扰（用手突加干扰）时会震荡。合适的P能较好的对打舵进行响应，又不太会震荡，但是舵量回中后会回弹好几下才能停止（没有D）。

	2.3. 加上D，D的效果十分明显，加快打舵响应，最大的作用是能很好地抑制舵量回中后的震荡，可谓立竿见影。太大的D会在横滚俯仰混控时表现出来（尽管在“烤四轴”时的表现可能很好），具体表现是四轴抓在手里推油门会抽搐。如果这样，只能回到“烤四轴”降低D，同时P也只能跟着降低。D调整完后可以再次加大P值，以能够跟随打舵为判断标准。

	2.4. 加上I，会发现手感变得柔和了些。由于笔者“烤四轴”的装置中四轴的重心高于旋转轴，这决定了在四轴偏离水平位置后会有重力分量使得四轴会继续偏离平衡位置。I的作用就可以使得在一定角度范围内（30度左右）可以修正重力带来的影响。表现打舵使得飞机偏离平衡位置，舵量回中后飞机立刻停止转动，若没有I或太小，飞机会由于重力继续转动。

3. 角度外环只有一个参数P。将外环加上（在APM中叫Stabilize模式，在大疆中叫姿态模式）。打舵会对应到期望的角度。P的参数比较简单。太小，打舵不灵敏，太大，打舵回中易震荡。以合适的打舵反应速度为准。

4. 至此，“烤四轴”效果应该会很好了，但是两个轴混控的效果如何还不一定，有可能会抽（两个轴的控制量叠加起来，特别是较大的D，会引起抽搐）。如果抽了，降低PD的值，I基本不用变。

5. 加上偏航的修正参数后（直接给双环参数，角度外环P和横滚差不多，内环P比横滚大些，I和横滚差不多，D可以先不加），拿在手上试过修正和打舵方向正确后可以试飞了（试飞很危险！！！！选择在宽敞、无风的室内，1米的高度（高度太低会有地面效应干扰，太高不容易看清姿态且容易摔坏），避开人群的地方比较适合，如有意外情况，立刻关闭油门！！！

	5.1. 试飞时主要观察这么几个方面的情况，一般经过调整的参数在平衡位置不会大幅度震荡，需要观察：

	5.2. 在平衡位置有没有小幅度震荡（可能是由于机架震动太大导致姿态解算错误造成。也可能是角速度内环D的波动过大，前者可以加强减震措施，传感器下贴上3M胶，必要时在两层3M泡沫胶中夹上“减震板”，注意：铁磁性的减震板会干扰磁力计读数；后者可以尝试降低D项滤波的截止频率）。

	5.3. 观察打舵响应的速度和舵量回中后飞机的回复速度。

	5.4. 各个方向（记得测试右前，左后等方向）大舵量突加输入并回中时是否会引起震荡。如有，尝试减小内环PD也可能是由于“右前”等混控方向上的舵量太大造成。

6. 横滚和俯仰调好后就可以调整偏航的参数了。合适参数的判断标准和之前一样，打舵快速响应，舵量回中飞机立刻停止转动（参数D的作用）。

### 参考
> [1] 王松,李新军,梁建宏,蒲立. 微小型机器人嵌入式自驾仪设计[J] 北京航空航天大学学报,
2005,(07) .

> [2] 叶青;熊茂华;预测-PID 串级控制在发酵过程中的应用[J] 微计算机信息,2006,(22),73-75.


> [3] 陈欣,杨一栋,张民. 一种无人机姿态智 PID 控制研究[J] 南京航空航天大学学报,2003,(06) .


> [4] 许民新，王律. 无人驾驶飞机机载传感器小型化研究与应用[J]. 数据采集与处理.


> [5] 四旋翼飞行器Quadrotor飞控之 PID调节（参考APM程序） via [CSDN](http://blog.csdn.net/super_mice/article/details/38436723)
