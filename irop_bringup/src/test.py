import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry
from geometry_msgs.msg import TwistWithCovarianceStamped

class OdomToTwistConverter(Node):
    def __init__(self):
        super().__init__('odom_to_twist_converter')
        
        self.subscription = self.create_subscription(
            Odometry,
            '/odom',
            self.odom_callback,
            10
        )
        
        self.publisher = self.create_publisher(
            TwistWithCovarianceStamped,
            '/twist_with_covariance',
            10
        )
        
        self.get_logger().info('Odom -> Twist 변환기가 정상 작동 시작')

    def odom_callback(self, msg):
        twist_msg = TwistWithCovarianceStamped()
        
        twist_msg.header.stamp = msg.header.stamp
        twist_msg.header.frame_id = msg.child_frame_id 
        
        twist_msg.twist = msg.twist
        
        self.publisher.publish(twist_msg)

def main(args=None):
    rclpy.init(args=args)
    node = OdomToTwistConverter()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info('변환기 종료')
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()