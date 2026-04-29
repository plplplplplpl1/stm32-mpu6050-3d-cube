#include "stm32f10x.h"
#include "MyI2C.h"
#include "MPU6050_Reg.h"

#define MPU6050_ADDRESS		0xD0		//MPU6050的I2C从机地址

/**
  * 函    数：MPU6050写寄存器
  * 参    数：RegAddress 寄存器地址，范围：参考MPU6050手册的寄存器描述
  * 参    数：Data 要写入寄存器的数据，范围：0x00~0xFF
  * 返 回 值：无
  */
void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data)
{
	MyI2C_Start();						//I2C起始
	MyI2C_SendByte(MPU6050_ADDRESS);	//发送从机地址，读写位为0，表示即将写入
	MyI2C_ReceiveAck();					//接收应答
	MyI2C_SendByte(RegAddress);			//发送寄存器地址
	MyI2C_ReceiveAck();					//接收应答
	MyI2C_SendByte(Data);				//发送要写入寄存器的数据
	MyI2C_ReceiveAck();					//接收应答
	MyI2C_Stop();						//I2C终止
}

/**
  * 函    数：MPU6050读寄存器
  * 参    数：RegAddress 寄存器地址，范围：参考MPU6050手册的寄存器描述
  * 返 回 值：读取寄存器的数据，范围：0x00~0xFF
  */
uint8_t MPU6050_ReadReg(uint8_t RegAddress)
{
	uint8_t Data;

	MyI2C_Start();						//I2C起始
	MyI2C_SendByte(MPU6050_ADDRESS);	//发送从机地址，读写位为0，表示即将写入
	MyI2C_ReceiveAck();					//接收应答
	MyI2C_SendByte(RegAddress);			//发送寄存器地址
	MyI2C_ReceiveAck();					//接收应答

	MyI2C_Start();						//I2C重复起始
	MyI2C_SendByte(MPU6050_ADDRESS | 0x01);	//发送从机地址，读写位为1，表示即将读取
	MyI2C_ReceiveAck();					//接收应答
	Data = MyI2C_ReceiveByte();			//接收指定寄存器的数据
	MyI2C_SendAck(1);					//发送应答，给从机非应答，终止从机的数据输出
	MyI2C_Stop();						//I2C终止

	return Data;
}

/**
  * 函    数：MPU6050连续读多个寄存器（burst/auto-increment模式）
  * 参    数：RegAddress 起始寄存器地址
  * 参    数：pBuffer 存放读取数据的缓冲区
  * 参    数：len 要读取的字节数
  * 返 回 值：无
  * 注意事项：MPU6050内部地址自动递增，只需一次I2C事务即可读取连续寄存器
  */
static void MPU6050_ReadBurst(uint8_t RegAddress, uint8_t *pBuffer, uint8_t len)
{
	uint8_t i;

	MyI2C_Start();						//I2C起始
	MyI2C_SendByte(MPU6050_ADDRESS);	//发送从机地址，读写位为0（写）
	MyI2C_ReceiveAck();					//接收应答
	MyI2C_SendByte(RegAddress);			//发送起始寄存器地址
	MyI2C_ReceiveAck();					//接收应答

	MyI2C_Start();						//I2C重复起始
	MyI2C_SendByte(MPU6050_ADDRESS | 0x01);	//发送从机地址，读写位为1（读）
	MyI2C_ReceiveAck();					//接收应答

	for (i = 0; i < len; i++)
	{
		pBuffer[i] = MyI2C_ReceiveByte();	//接收一个字节
		/* 前len-1字节发应答(0)，最后一字节发非应答(1) */
		MyI2C_SendAck(i == len - 1 ? 1 : 0);
	}
	MyI2C_Stop();						//I2C终止
}

/**
  * 函    数：MPU6050初始化
  * 参    数：无
  * 返 回 值：无
  */
void MPU6050_Init(void)
{
	MyI2C_Init();									//先初始化底层的I2C

	/*MPU6050寄存器初始化，需要对照MPU6050手册的寄存器描述配置，此处仅配置了部分重要的寄存器*/
	MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);		//电源管理寄存器1，取消睡眠模式，选择时钟源为X轴陀螺仪
	MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);		//电源管理寄存器2，保持默认值0，所有轴均不待机
	MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x09);		//采样率分频寄存器，配置采样率
	MPU6050_WriteReg(MPU6050_CONFIG, 0x06);			//配置寄存器，配置DLPF
	MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);	//陀螺仪配置寄存器，选择满量程为±2000°/s
	MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18);	//加速度计配置寄存器，选择满量程为±16g
}

/**
  * 函    数：MPU6050获取ID号
  * 参    数：无
  * 返 回 值：MPU6050的ID号
  */
uint8_t MPU6050_GetID(void)
{
	return MPU6050_ReadReg(MPU6050_WHO_AM_I);		//返回WHO_AM_I寄存器的值
}

/**
  * 函    数：MPU6050获取数据
  * 参    数：AccX AccY AccZ 加速度计X、Y、Z轴的数据，使用输出参数的形式返回，范围：-32768~32767
  * 参    数：GyroX GyroY GyroZ 陀螺仪X、Y、Z轴的数据，使用输出参数的形式返回，范围：-32768~32767
  * 返 回 值：无
  * 注意事项：使用burst读取模式，一次I2C事务读取所有6轴+温度共14字节，效率远高于逐寄存器读取
  */
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ,
						int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
	uint8_t buf[14];

	/*
	 * 寄存器布局（地址连续，支持auto-increment）：
	 *   0x3B-0x40: AccX/Y/Z (6字节)
	 *   0x41-0x42: 温度     (2字节，跳过)
	 *   0x43-0x48: GyroX/Y/Z (6字节)
	 * 一次burst读取14字节，只需1次I2C事务
	 */
	MPU6050_ReadBurst(MPU6050_ACCEL_XOUT_H, buf, 14);

	*AccX  = (int16_t)((buf[0]  << 8) | buf[1]);
	*AccY  = (int16_t)((buf[2]  << 8) | buf[3]);
	*AccZ  = (int16_t)((buf[4]  << 8) | buf[5]);
	/* buf[6..7] = 温度数据（本驱动不使用） */
	*GyroX = (int16_t)((buf[8]  << 8) | buf[9]);
	*GyroY = (int16_t)((buf[10] << 8) | buf[11]);
	*GyroZ = (int16_t)((buf[12] << 8) | buf[13]);
}
