
#include <ros/ros.h>
#include<iostream>
#include "JY901.h"
#include "rs232.h"
#include "odomIMU_msgs/odomIMU.h"

using namespace std;

ros::Publisher odomIMU_pub;

enum dataType_ {

    STIME = 0,
    SACC,
    SGYRO,
    SANGLE,
    SMAG,
    SDSTATUS,
    SPRESS,
    SLONLAT,
    SGPSV,
    SQ,
    ENCODE0CNT,
    ENCODE1CNT
};


typedef struct IMU_{
    struct STime stcTime; //0
    struct SAcc stcAcc; // 1
    struct SGyro stcGyro; // 2
    struct SAngle stcAngle; // 3
    struct SMag stcMag; // 4
    struct SDStatus stcDStatus; // 5
    struct SPress stcPress; // 6
    struct SLonLat stcLonLat; // 7
    struct SGPSV stcGPSV; // 8
    struct SQ stcQ; // 9

}imuData;

typedef struct SensorData_{
    char head[4];
    imuData imu;
    int Encode0Cnt; // 10
    int Encode1Cnt; // 11

}SensorData;

static unsigned char ucRxBuffer[250];
static unsigned char ucRxCnt = 0;

int parseSensorData(char Byte){

    ucRxBuffer[ucRxCnt++] = Byte;
    if(ucRxCnt < 4){
        return 0;
    }

    if(ucRxBuffer[0] != 0x50 &&
    ucRxBuffer[1] != 0x51 &&
    ucRxBuffer[2] != 0x52 &&
    ucRxBuffer[3] != 0x53){
        ucRxCnt --;
        memmove(ucRxBuffer , &ucRxBuffer[1] , ucRxCnt);
        return 0;
    }

    if(ucRxCnt < sizeof(SensorData)){
        return 0;
    }
    else{
        ucRxCnt = 0;
    }

    SensorData *p = (SensorData *)ucRxBuffer;
    odomIMU_msgs::odomIMU msg;
	msg.header.frame_id = "odomIMU_frame_info";
	msg.header.stamp = ros::Time::now();
	msg.gyro[0] = p->imu.stcGyro.w[0];
	msg.gyro[1] = p->imu.stcGyro.w[1];
	msg.gyro[2] = p->imu.stcGyro.w[2];	

	msg.acc[0] = p->imu.stcAcc.a[0];
	msg.acc[1] = p->imu.stcAcc.a[1];
	msg.acc[2] = p->imu.stcAcc.a[2];

	msg.angle[0] = p->imu.stcAngle.Angle[0];
	msg.angle[1] = p->imu.stcAngle.Angle[1];
	msg.angle[2] = p->imu.stcAngle.Angle[2];
	
	msg.encode0Cnt = p->Encode0Cnt;
	msg.encode1Cnt = p->Encode1Cnt;

	odomIMU_pub.publish(msg);

    printf("stcTime:Y%04d M%04d D%04d h%04d m%04d s%04d ms%04d \n" , p->imu.stcTime.ucYear , p->imu.stcTime.ucMonth,
        p->imu.stcTime.ucDay , p->imu.stcTime.ucHour , p->imu.stcTime.ucMinute, p->imu.stcTime.ucSecond,
        p->imu.stcTime.usMiliSecond);

    printf("stcAcc:a%02x T%02x\n" , p->imu.stcAcc.a , p->imu.stcAcc.T);
    printf("stcGyro:w%02x T%02x\n" , p->imu.stcGyro.w , p->imu.stcGyro.T);
    printf("stcAngle:Angle%02x T%02x\n" , p->imu.stcAngle.Angle , p->imu.stcAngle.T);
    printf("stcMag:h%02x T%02x\n" , p->imu.stcMag.h , p->imu.stcMag.T);
    printf("stcDStatus:a%02x T%02x\n" , p->imu.stcDStatus.sDStatus);
    printf("stcPress:a%02x T%02x\n" , p->imu.stcPress.lAltitude , p->imu.stcPress.lPressure);
    printf("stcLonLat:a%02x T%02x\n" , p->imu.stcLonLat.lLat , p->imu.stcLonLat.lLon);
    printf("stcGPSV:%02x %02x %02x\n" , p->imu.stcGPSV.lGPSVelocity , p->imu.stcGPSV.sGPSHeight, p->imu.stcGPSV.sGPSYaw);
    printf("stcQ:%02x\n" , p->imu.stcQ.q);
    printf("Encode0Cnt:%02x\n" , p->Encode0Cnt);
    printf("Encode1Cnt:%02x\n" , p->Encode1Cnt);


}

int main(int argc, char **argv){

    ros::init(argc, argv, "mapBuilder");
    ros::start();
	
	ros::NodeHandle nh;
	odomIMU_pub = nh.advertise<odomIMU_msgs::odomIMU>("odomIMU_frame" , 100);

    int i,n,
    cport_nr = 0, //
    bdrate = 921600;
    
    unsigned char buf[4096];

    char mode[] = {'8' , 'N' , '1' , 0};

    if(RS232_OpenComport(cport_nr , bdrate , mode)){
        printf("Can not open comport\n");
        return 0;
    } 

    while(ros::ok()){
        
        n = RS232_PollComport(cport_nr , buf , 4095);
        if(n>0){
            buf[n] = 0;
            for(int i=0;i<n;i++){
                parseSensorData(buf[i]);
            }
        }

        //usleep(100000);
		ros::spinOnce();
    }
	ros::shutdown();

    return 0;
}