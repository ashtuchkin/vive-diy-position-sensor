cmake_minimum_required(VERSION 2.8.3)
project(vive_tracker)
find_package(catkin REQUIRED COMPONENTS
  rospy
  std_msgs
)
catkin_package(
  CATKIN_DEPENDS rospy std_msgs
)
catkin_install_python(PROGRAMS
  src/vive_tracker.py
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/vive_tracker
)

