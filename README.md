# FYDP
Capstone Project - Intelligent Manufacturing Inspection

Clone FYDP repository in your documents, and move Robotic_Arm directory (and all its contents) to your catkin workspace (catkin_ws).

Run catkin_make

Use the following command to run simulation of the robotic arm in Gazebo:

roslaunch robotic_arm display.launch model:='$(find robotic_arm)/urdf/robotic_arm.urdf'
