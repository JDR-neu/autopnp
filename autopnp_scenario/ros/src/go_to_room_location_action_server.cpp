#include <autopnp_scenario/go_to_room_location.h>

go_inside_of_the_room::go_inside_of_the_room(std::string name_of_the_action) :
		go_inside_of_the_room_action_server_(nh_, name_of_the_action, boost::bind(&go_inside_of_the_room::execute_go_to_room_location_action_server_, this, _1), false), action_name_(
				name_of_the_action)
{
	//Start action server
	go_inside_of_the_room_action_server_.start();
}

//execute move_base action and receives the goal in pixel
move_base_msgs::MoveBaseGoal go_inside_of_the_room::Move_in_pixel_(int x_coordinate_value, int y_coordinate_value)
{
	listener_.lookupTransform("/map", "/base_link", ros::Time(0), transform_);

	move_base_msgs::MoveBaseGoal move_base_msgs_obj;
	geometry_msgs::PoseStamped goal_to_move_base_action_server;

	double p = (x_coordinate_value * map_resolution_) + map_origin_.x;
	double q = (y_coordinate_value * map_resolution_) + map_origin_.y;

	double m = transform_.getOrigin().x();
	double n = transform_.getOrigin().y();

	double angel_around_z_axis = atan2((q - n), (p - m));

	goal_to_move_base_action_server.header.frame_id = "map";
	goal_to_move_base_action_server.header.stamp = ros::Time::now();
	goal_to_move_base_action_server.pose.position.x = p;
	goal_to_move_base_action_server.pose.position.y = q;
	tf::Quaternion quat = tf::createQuaternionFromYaw(angel_around_z_axis);
	tf::quaternionTFToMsg(quat, goal_to_move_base_action_server.pose.orientation);

	move_base_msgs_obj.target_pose = goal_to_move_base_action_server;

	return move_base_msgs_obj;
}

//always keep the robot in front in moving direction and receives the goal in pixel
move_base_msgs::MoveBaseGoal go_inside_of_the_room::stay_forward_(int x_coordinate_value, int y_coordinate_value)
{
	listener_.lookupTransform("/map", "/base_link", ros::Time(0), transform_);

	move_base_msgs::MoveBaseGoal move_base_msgs_obj;
	geometry_msgs::PoseStamped goal_to_move_base_action_server;

	double p = (x_coordinate_value * map_resolution_) + map_origin_.x;
	double q = (y_coordinate_value * map_resolution_) + map_origin_.y;

	double m = transform_.getOrigin().x();
	double n = transform_.getOrigin().y();

	double angel_around_z_axis = atan2((q - n), (p - m));

	goal_to_move_base_action_server.header.frame_id = "map";
	goal_to_move_base_action_server.header.stamp = ros::Time::now();
	goal_to_move_base_action_server.pose.position.x = m;
	goal_to_move_base_action_server.pose.position.y = n;
	tf::Quaternion quat = tf::createQuaternionFromYaw(angel_around_z_axis);
	tf::quaternionTFToMsg(quat, goal_to_move_base_action_server.pose.orientation);

	move_base_msgs_obj.target_pose = goal_to_move_base_action_server;

	return move_base_msgs_obj;
}

//this function will send the robot to room center position
std::string go_inside_of_the_room::go_to_room_center_location_(const cv::Mat &original_map_from_goal_definition)
{
	move_base_action_client_datatype move_base_client_obj("move_base", true);
	move_base_client_obj.waitForServer();
	move_base_client_obj.sendGoal(stay_forward_(goal_room_center_x_, goal_room_center_y_));
	move_base_client_obj.waitForResult();

	ROS_INFO("333333333333 go to location action server 333333333333");
	ROS_INFO("robot is going towards the room center.....");
	move_base_client_obj.waitForServer();
	move_base_client_obj.sendGoal(Move_in_pixel_(goal_room_center_x_, goal_room_center_y_));

	bool finished_before_timeout = move_base_client_obj.waitForResult();

	if (finished_before_timeout)
	{
		actionlib::SimpleClientGoalState state = move_base_client_obj.getState();
		ROS_INFO("Move Base Action for go to destination finished: %s", state.toString().c_str());
	}

	else
	{
		ROS_INFO("Move Base Action for go to destination did not finish before the time out.");
	}

	listener_.lookupTransform("/map", "/base_link", ros::Time(0), transform_);
	pose robot_location_in_meter(transform_.getOrigin().x(), transform_.getOrigin().y(), transform_.getRotation().z());
	robot_location_in_pixel_ = convert_from_meter_to_pixel_coordinates_<cv::Point>(robot_location_in_meter);

	ROS_INFO("333333333333 go to location action server 333333333333\n");

	if (original_map_from_goal_definition.at<unsigned char>(goal_room_center_y_, goal_room_center_x_)
			== original_map_from_goal_definition.at<unsigned char>(robot_location_in_pixel_))
	{
		feedback_about_robot_location_ = "True";
		return feedback_about_robot_location_;
	}
	else
	{
		feedback_about_robot_location_ = "False";
		return feedback_about_robot_location_;
	}
}

void go_inside_of_the_room::execute_go_to_room_location_action_server_(const autopnp_scenario::GoToRoomLocationGoalConstPtr &goal)
{
	ros::Rate looping_rate(1);

	//get the necessary room information from goal definition from client
	map_resolution_ = goal->map_resolution;
	map_origin_.x = goal->map_origin_x;
	map_origin_.y = goal->map_origin_y;

	goal_room_center_x_ = goal->room_center_position_x;
	goal_room_center_y_ = goal->room_center_position_y;

	//converting the map msg in cv format
	cv_bridge::CvImagePtr cv_ptr;
	cv_ptr = cv_bridge::toCvCopy(goal->input_img, sensor_msgs::image_encodings::MONO8);
	cv::Mat original_img;
	original_img = cv_ptr->image;

	std::string Result = go_to_room_center_location_(original_img);

	result_.output_flag = Result;

	looping_rate.sleep();

	go_inside_of_the_room_action_server_.setSucceeded(result_);
}

int main(int argc, char** argv)
{
	ros::init(argc, argv, "go_to_room_location");
	go_inside_of_the_room TL(ros::this_node::getName());
	ROS_INFO("go to room location action server is initialized.....");
	ros::spin();
	return 0;
}
