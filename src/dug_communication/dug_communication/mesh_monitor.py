import rclpy
from rclpy.node import Node
from dug_msgs.msg import SwarmState

class MeshMonitor(Node):
    def __init__(self):
        super().__init__('mesh_monitor')
        
        self.peers = {}
        self.subscription = self.create_subscription(
            SwarmState,
            '/swarm/status',
            self.swarm_callback,
            10)
        
        self.timer = self.create_wall_timer(2.0, self.report_mesh_status)
        self.get_logger().info('Mesh Monitor Node Started')

    def swarm_callback(self, msg):
        self.peers[msg.uav_id] = self.get_clock().now()

    def report_mesh_status(self):
        now = self.get_clock().now()
        # Remove peers not seen in last 5 seconds
        active_peers = {pid: last_seen for pid, last_seen in self.peers.items() 
                       if (now - last_seen).nanoseconds / 1e9 < 5.0}
        
        self.peers = active_peers
        peer_count = len(self.peers)
        self.get_logger().info(f'Active Mesh Nodes: {peer_count} | Peer IDs: {list(self.peers.keys())}')

def main(args=None):
    rclpy.init(args=args)
    node = MeshMonitor()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
