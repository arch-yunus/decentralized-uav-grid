import rclpy
from rclpy.node import Node
from geometry_msgs.msg import PoseStamped
import random
import math

class VSLAMNode(Node):
    def __init__(self):
        super().__init__('vslam_node')
        
        self.publisher_ = self.create_publisher(PoseStamped, 'vslam_pose', 10)
        self.timer = self.create_wall_timer(0.1, self.publish_vslam_pose) # 10Hz
        
        # Internal state for simulation
        self.current_x = 0.0
        self.current_y = 0.0
        self.current_z = 0.0
        self.drift_x = 0.0
        self.drift_y = 0.0
        
        self.get_logger().info('VSLAM Simulation Node Started')

    def publish_vslam_pose(self):
        msg = PoseStamped()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = 'odom'
        
        # Simulate slight drift over time
        self.drift_x += random.uniform(-0.01, 0.01)
        self.drift_y += random.uniform(-0.01, 0.01)
        
        # Loop closure simulation: occasionally reset drift
        if random.random() < 0.01:
            self.get_logger().info('VSLAM: Loop Closure Detected! Correcting drift.')
            self.drift_x *= 0.1
            self.drift_y *= 0.1
            
        msg.pose.position.x = self.current_x + self.drift_x
        msg.pose.position.y = self.current_y + self.drift_y
        msg.pose.position.z = self.current_z
        
        # Simple orientation (always facing "forward" for now)
        msg.pose.orientation.w = 1.0
        
        self.publisher_.publish(msg)

def main(args=None):
    rclpy.init(args=args)
    node = VSLAMNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
