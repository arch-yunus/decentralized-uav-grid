#!/bin/bash
# Decentralized UAV Grid (DUG) Environment Setup Script

set -e

echo "Setting up Decentralized UAV Grid (DUG) Environment..."

# Update and install basic dependencies
sudo apt-get update
sudo apt-get install -y \
    python3-pip \
    python3-colcon-common-extensions \
    ros-humble-desktop \
    ros-humble-mavros \
    ros-humble-mavros-msgs \
    ros-humble-gazebo-ros-pkgs \
    batman-adv-dkms \
    batctl

# Install Python dependencies
pip3 install mavproxy pymavlink

echo "Installing ROS2 dependencies using rosdep..."
if [ -f "/etc/ros/rosdep/sources.list.d/20-default.list" ]; then
    sudo rosdep init || true
fi
rosdep update
rosdep install --from-paths src --ignore-src -r -y

echo "Environment setup complete!"
echo "Please source your ROS2 installation: source /opt/ros/humble/setup.bash"
