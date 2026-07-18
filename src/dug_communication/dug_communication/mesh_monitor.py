import rclpy
from rclpy.node import Node
from dug_msgs.msg import SwarmState
import hashlib
import time

class MeshMonitor(Node):
    def __init__(self):
        super().__init__('mesh_monitor')
        
        self.peers = {}
        self.subscription = self.create_subscription(
            SwarmState,
            '/swarm/status',
            self.swarm_callback,
            10)
        
        self.secret_key = "dug_secure_key_2026" # Simulated shared secret
        self.trust_levels = {} # node_id -> float (0.0 to 1.0)
        
        self.timer = self.create_wall_timer(2.0, self.report_mesh_status)
        self.get_logger().info('Mesh Monitor Node Started')

    def swarm_callback(self, msg):
        is_trusted = self.verify_handshake(msg.uav_id, msg.signature)
        
        if is_trusted:
            self.peers[msg.uav_id] = {
                'last_seen': self.get_clock().now(),
                'trusted': True
            }
        else:
            self.get_logger().warning(f'Untrusted node attempted to join mesh: ID {msg.uav_id} (Signature: {msg.signature})')

    def verify_handshake(self, uav_id, signature):
        # Generate expected HMAC for the current time window (10s) and neighboring windows
        time_window = int(time.time() / 10)
        for offset in [0, -1, 1]:
            data = f"{uav_id}:{time_window + offset}:{self.secret_key}"
            expected_hash = hashlib.sha256(data.encode()).hexdigest()
            if expected_hash == signature:
                return True
        return False

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
