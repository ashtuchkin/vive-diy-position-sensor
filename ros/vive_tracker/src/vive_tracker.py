#!/usr/bin/env python
import rospy
import time
from geometry_msgs.msg import PoseStamped
from std_msgs.msg import Time
import serial


def init():
   # print "init"
    global ser
    ret = ser.readline()
    if not ret.startswith('OBJ0'):
    	ser.write(b'continue')
    	time.sleep(0.5)
    	ret = ser.readline()
    	rospy.loginfo(ret)
    	if not ret.startswith('OBJ0'):
        	ser.write(b'o')

def vive_tracker():
    global ser
    rospy.init_node('vive_tracker', anonymous=True)
    rate = rospy.Rate(30) # 10hz
    try:
        ser = serial.Serial(port='/dev/ttyACM0', baudrate=57600, timeout=1.0)
        ret = ser.readline()
        rospy.loginfo(ret)
    	if not ret.startswith('OBJ0'):
             init()
    except:
        print "failed"
        raise
    #Define Publisher for topics
    pubPose = rospy.Publisher('/vive/pose', PoseStamped, queue_size=1 )

    while not rospy.is_shutdown():
        try:
            res=ser.readline()
            if res.startswith('OBJ0'):
                rawData = res.split("\t")
                if len(rawData) == 7:
                    poseStamped=PoseStamped()
                    poseStamped.header.frame_id = "/vive"
                    poseStamped.header.stamp = rospy.Time.now()
                    poseStamped.pose.position.x = float(rawData[3])*-1
                    poseStamped.pose.position.y = float(rawData[5])
                    poseStamped.pose.position.z = float(rawData[4])
 
                    #Publish Pose, ID and take a nap
                    pubPose.publish(poseStamped)
            
                    rate.sleep()
        except Exception as e:
            #You mad bro?
            rospy.loginfo(e)
            

if __name__ == '__main__':
    try:
        vive_tracker()
    except rospy.ROSInterruptException:
        cap.release()
