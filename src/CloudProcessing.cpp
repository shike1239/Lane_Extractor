#include <lane_extractor/CloudProcessing.h>

namespace lane_extractor
{
    CloudProcessing::CloudProcessing()
    : cloud(new pcl::PointCloud<pcl::PointXYZI>)
    , cloud_filtered(new pcl::PointCloud<pcl::PointXYZI>)
    , Intesity_Cloud(new pcl::PointCloud<pcl::PointXYZI>)
    , leaf_size(1.0)
    {

        map_pub = nh.advertise<sensor_msgs::PointCloud2> ("Point_map", 10);
        lane_pub = nh.advertise<sensor_msgs::PointCloud2> ("lane_pub", 10);
        cloud_filtered_pub = nh.advertise<sensor_msgs::PointCloud2> ("cloud_filtered_pub", 10);
        marker_pub = nh.advertise<visualization_msgs::Marker>("visualization_marker", 1);

        nh.getParam("/file_path", file_path);
        CloudLoader();

       // MapDownsampling(false);
        map_publish();
    }

    void CloudProcessing::CloudLoader(){
            pcl::io::loadPCDFile<pcl::PointXYZI> (file_path.c_str(), *cloud);
            ROS_INFO("Cloud Loading Complete!");
    }

    void CloudProcessing::passthrough()
    {
        pcl::PassThrough<pcl::PointXYZI> pass;
        pass.setInputCloud(cloud);
        // pass.setFilterFieldName ("x"); // X..
        // pass.setFilterLimits (-150.0, 150.0);
        // pass.filter(*cloud);
            
        // pass.setFilterFieldName ("y"); // Y..
        // pass.setFilterLimits (-100.0, 300.0);
        // pass.filter(*cloud);

        pass.setFilterFieldName ("z");
        pass.setFilterLimits (-1.0, 1.0);
        pass.filter(*cloud);

        ROS_INFO("Z axis Passthrough Complete!");
    }

    void CloudProcessing::VoxelGridFilter(){
            pcl::VoxelGrid<pcl::PointXYZI> voxel;
            voxel.setInputCloud(cloud);
            voxel.setLeafSize(leaf_size,leaf_size,leaf_size);
            voxel.filter(*cloud);

            ROS_INFO("%fM VoxelGridFilter Complete!",leaf_size);
    }

    void CloudProcessing::MapDownsampling(bool voxel){
              passthrough();         
              if(voxel){
               VoxelGridFilter();
              }
              ROS_INFO("downsampling Complete!");
    }
    void CloudProcessing::fromMsgToCloud(const sensor_msgs::PointCloud2 &msg){
        sensor_msgs::PointCloud2 c_msg = msg;
        c_msg.header.frame_id = "map";
        c_msg.fields[3].name = "intensity";
        pcl::fromROSMsg(c_msg, *cloud);
    }

    void CloudProcessing::map_publish(){
                pcl::toROSMsg(*cloud, cloud_msg);
                cloud_msg.header.frame_id = "map";
                map_pub.publish(cloud_msg);
    }

    void CloudProcessing::lane_publish(){
                pcl::toROSMsg(*Intesity_Cloud, Intensity_msg);
                Intensity_msg.header.frame_id="map";
                lane_pub.publish(Intensity_msg);
    }

    void CloudProcessing::cloud_filtered_publish(){
                pcl::toROSMsg(*cloud_filtered, cloud_filtered_msg);
                cloud_filtered_msg.header.frame_id="map";
                cloud_filtered_pub.publish(cloud_filtered_msg);
    }

    void CloudProcessing::marker_publish(tf::Transform &pose){

        uint32_t shape = visualization_msgs::Marker::ARROW;

        visualization_msgs::Marker marker;
        // Set the frame ID and timestamp.  See the TF tutorials for information on these.
        marker.header.frame_id = "/marker_frame";
        marker.header.stamp = ros::Time::now();

        // Set the namespace and id for this marker.  This serves to create a unique ID
        // Any marker sent with the same namespace and id will overwrite the old one
        marker.ns = "basic_shapes";
        marker.id = 0;

        // Set the marker type.  Initially this is CUBE, and cycles between that and SPHERE, ARROW, and CYLINDER
        marker.type = shape;

        // Set the marker action.  Options are ADD, DELETE, and new in ROS Indigo: 3 (DELETEALL)
        marker.action = visualization_msgs::Marker::ADD;

        // Set the pose of the marker.  This is a full 6DOF pose relative to the frame/time specified in the header
        marker.pose.position.x = pose.getOrigin().getX();
        marker.pose.position.y = pose.getOrigin().getY();
        marker.pose.position.z = pose.getOrigin().getZ();
        marker.pose.orientation.x = pose.getRotation().x();
        marker.pose.orientation.y = pose.getRotation().y();
        marker.pose.orientation.z = pose.getRotation().z();
        marker.pose.orientation.w = pose.getRotation().w();

        // Set the scale of the marker -- 1x1x1 here means 1m on a side
        marker.scale.x = 1.0;
        marker.scale.y = 1.0;
        marker.scale.z = 1.0;

        // Set the color -- be sure to set alpha to something non-zero!
        marker.color.r = 0.0f;
        marker.color.g = 1.0f;
        marker.color.b = 0.0f;
        marker.color.a = 1.0;

        marker.lifetime = ros::Duration();
        marker_pub.publish(marker);
}
    //Functions for storing found roads
    bool CloudProcessing::CloudSaver(int num)
    {
        std::stringstream ss;
        ss << num;
        std::string str = ss.str();
        char *name = new char[str.size()+1];
        std::strcpy(name,str.c_str());
        strcat(name,"_lane.pcd");
        pcl::PCDWriter writer;
        Intesity_Cloud->width = Intesity_Cloud->points.size();
        Intesity_Cloud->height = 1;
        Intesity_Cloud->resize(Intesity_Cloud->width*Intesity_Cloud->height);
        if(writer.writeASCII(name,*Intesity_Cloud))
            return true;
        else{
            Intesity_Cloud->points.clear();
            return false;
        }
    }

}