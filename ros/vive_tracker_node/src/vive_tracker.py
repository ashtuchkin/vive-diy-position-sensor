#!/usr/bin/env python
import rospy
import time
import rospkg
from geometry_msgs.msg import PoseStamped
from std_msgs.msg import Int16, Time
from tf.msg import tfMessage
import numpy as np
import cv2
import cv2.aruco as aruco
import serial


def init():
   # print "init"
    global ser
    ser.write(b'continue')
    time.sleep(0.5)


def vive_tracker():
    global ser
    rospy.init_node('vive_tracker', anonymous=True)
    rate = rospy.Rate(30) # 10hz
    portname = rospy.get_param('~port')
    rospy.loginfo('trying to connect to %s', portname)
    try:
        ser = serial.Serial(port=portname, baudrate=56700, timeout=1.0)
        ret = ser.readline()

    if not ret.startswith('OBJ0'):
             init()

    except:
        print "failed"
        rospy.logfatal('cannot connect to port %s', portname)
        raise
    #Define Publisher for topics
    pubPose = rospy.Publisher('/vive/pose', PoseStamped, queue_size=1 )

    while not rospy.is_shutdown():
        try:
            res=ser.readline()
            if res.startswith('OBJ0'):
                rospy.loginfo(res)
                rawData = res.split(",")
                if len(rawData) == 7:
                    # Double check it is pos data
                    if rawData[0]=='POS' and rawData[2]>=0:
                        confidence = int(rawData[2])
                        if confidence >= conf_thresh:
                            poseStamped=PoseStamped()
                            poseStamped.header.frame_id = "/vive"
                            poseStamped.header.stamp = rospy.Time.now()
                            poseStamped.pose.position.x = float(rawData[3])
                            poseStamped.pose.position.y = float(rawData[5])
                            poseStamped.pose.position.z = float(rawData[4])
                            poseStamped.pose.orientation.x = rawData[3]
                            poseStamped.pose.orientation.y = rawData[5]
                            poseStamped.pose.orientation.z = rawData[4]   
                            poseStamped.pose.orientation.w = rawData[6]

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