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
        # Zero-Trust Handshake Simulation
        # In a real scenario, we would check cryptographic signatures here
        is_trusted = self.verify_handshake(msg.uav_id)
        
        if is_trusted:
            self.peers[msg.uav_id] = {
                'last_seen': self.get_clock().now(),
                'trusted': True
            }
        else:
            self.get_logger().warning(f'Untrusted node attempted to join mesh: ID {msg.uav_id}')

    def verify_handshake(self, uav_id):
        # Simulation: only IDs less than 100 are trusted
        return uav_id < 100

    def report_mesh_status(self):
        now = self.get_clock().now()
        # Remove peers not seen in last 5 seconds
        active_peers = {pid: data for pid, data in self.peers.items() 
                       if (now - data['last_seen']).nanoseconds / 1e9 < 5.0}
        
        self.peers = active_peers
        peer_count = len(self.peers)
        trusted_count = sum(1 for data in self.peers.values() if data['trusted'])
        
        self.get_logger().info(f'Mesh Status | Nodes: {peer_count} | Trusted: {trusted_count} | IDs: {list(self.peers.keys())}')

def main(args=None):
    rclpy.init(args=args)
    node = MeshMonitor()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
