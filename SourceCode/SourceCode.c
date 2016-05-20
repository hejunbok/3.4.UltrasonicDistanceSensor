/******************************************************************************
 * Ultrasonic Range Finder Operation
 *
 * Description:
 *
 * An implementation which triggers an ultrasonic range finder with a digital pulse
 *
 * lasting less than 1ms and determines the distance between the sensor and
 *
 * the obstacle based on the echo pulse generated by the ultrasonic transceiver.
 *
 * The distance is measured based on the time between the transmission of
 *
 * the pulse to the detection of the first wave front arriving
 *
 * back at the sensor.
 *
 * Author:
 *  		Siddharth Ramkrishnan (sxr4316@rit.edu)	: 04.25.2016
 *
 *****************************************************************************/

#include <sys/neutrino.h>
#include <sys/syspage.h>
#include <sys/netmgr.h>
#include <sys/mman.h>
#include <hw/inout.h>
#include <pthread.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>

static int				EventCounter = 0;

static uintptr_t		PortControl, SensorTrigger, SensorData;

static double			StartTime, EndTime, EchoDuration;

static double			Distance, MaxDistance = 0, MinDistance = 1000 ;

static char				ControlData, WriteData;

#define	REALTIME(Clock)	(((double)Clock)/ CLOCKS_PER_SEC)

void HardwareInitialization(void)
{
// Provide Access to restricted Hardware Registers
	ThreadCtl( _NTO_TCTL_IO, NULL );

// Declare Port A as Output and Port B as input
	ControlData		=	0x02;

	WriteData		=	0x00;

//	Map the hardware registers corresponding to Port A, Port B, and Control registers.
	SensorTrigger	=	mmap_device_io(1,0x288);

	PortControl		=	mmap_device_io(1,0x28B);

	SensorData		=	mmap_device_io(1,0x289);

	(void)out8(PortControl,ControlData);

	(void)out8(SensorTrigger,WriteData);
}

void TimerInitialization();

void PulseTriggerAction()
{

//	Oscillate between on and off for every 99ms and 1ms respectively
	if(WriteData==0x00)
	{
		WriteData			=		0xFF	;
	}
	else
	{
		WriteData			=		0x00	;
	}

	(void)out8(SensorTrigger,WriteData);

// Initialize the Timer to retrigger the function after the required on/off time
	TimerInitialization();
}

void TimerInitialization()
{
    // timer being used
    static timer_t timer;

    // structure needed for timer_create
    static struct sigevent   event;
    // structure needed to attach a signal to a function
    static struct sigaction  action;
    // structure that sets the start and interval time for the timer
    static struct itimerspec time_info;

    // itimerspec:
    //      it_value is used for the first tick
    //      it_interval is used for the interval between ticks
    if(WriteData == 0x00)
    {
    	time_info.it_value.tv_nsec    =  99000000;
    }else
    {
    	time_info.it_value.tv_nsec    =   1000000;
    }

    time_info.it_interval.tv_nsec = 0;

    // this is used to bind the action to the function
    // you want called ever timer tick
    action.sa_sigaction = &PulseTriggerAction;

    // this is used to queue signals
    action.sa_flags = SA_SIGINFO;

    // this function takes a signal and binds it to your action
    // the last param is null because that is for another sigaction
    // struct that you want to modify
    sigaction(SIGUSR1, &action, NULL);

    // SIGUSR1 is an unused signal provided for you to use

    // this macro takes the signal you bound to your action,
    // and binds it to your event
    SIGEV_SIGNAL_INIT(&event, SIGUSR1);

    // now your function is bound to an action
    // which is bound to a signal
    // which is bound to your event

    // now you can create the timer using your event
    timer_create(CLOCK_REALTIME, &event, &timer);

    // and then start the timer, using the time_info you set up
    timer_settime(timer, 0, &time_info, 0);
}

int main()
{
	EventCounter	=	0	;

	HardwareInitialization();

	TimerInitialization();

	printf("\n \r Ultrasonic Sensor Operation \n \r");

	while(EventCounter<500)
	{

//	Wait for rising edge of the echo line
		while((in8(SensorData)&0x01)==0x00)	;

//	Capture the Start Time of EchoLine
		StartTime	 =	ClockCycles()	;

//	Wait for falling edge of the echo line
		while((in8(SensorData)&0x01)==0x01)	;

//	Capture the End Time of EchoLine
		EndTime		 =	ClockCycles()	;

		EchoDuration = REALTIME(EndTime-StartTime);

//	Check for validity of the measured Signal Duration
		if((EchoDuration>0.00002)&&(EchoDuration < 0.018))
		{
			Distance =	EchoDuration * 13544.08 / 2	;

			if(Distance > MaxDistance)
				MaxDistance = Distance;

			if(Distance < MinDistance)
				MinDistance = Distance;

			printf("\r Echo Duration : %f s Measured Distance : %3.0f inches", EchoDuration, Distance);
		} else
		{
			printf("\r Echo Duration : %f s Measured Distance :	 ********** ", EchoDuration);
		}

		EventCounter = EventCounter + 1	;
	}

	printf("\n \r Maximum Measured Distance	:	%3.0f inches", MaxDistance);

	printf("\n \r Minimum Measured Distance	:	%3.0f inches", MinDistance);

	printf("\n End of Ulltrasonic sensor operation \n");

	return 0;
}
