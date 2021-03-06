/*
 * BSD 3-Clause License
 * Copyright (c) 2018, Máté Cserép & Péter Hudoba
 * All rights reserved.
 *
 * You may obtain a copy of the License at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include "LASHelper.h"
#include "LogHelper.h"

namespace railroad
{

LASreader *openLASReader(const std::string &filename, LASheader &header);

LASheader readLASHeader(const std::string &filename)
{
    LASheader header;

    LASreader *lasreader = openLASReader(filename, header);
    lasreader->close();
    delete lasreader;

    return header;
}


pcl::PointCloud<pcl::PointXYZ>::Ptr readLAS(
        const std::string &filename,
        LASheader &header,
        unsigned long maxSize)
{
    LASreader *lasreader = openLASReader(filename, header);

    unsigned long size = std::min(static_cast<unsigned long>(lasreader->npoints), maxSize);
    LOG(debug) << "Number of points: " << lasreader->npoints;
    LOG(debug) << "Number of points to read: " << size;

    // PCL point cloud
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    cloud->resize(size);

    LOG(debug) << "Started reading input";
    LASpoint *point;
    for (unsigned long count = 0; count < size; ++count) {
        lasreader->read_point();
        point = &lasreader->point;

        cloud->points[count] = pcl::PointXYZ(
                point->X * lasreader->header.x_scale_factor + lasreader->header.x_offset,
                point->Y * lasreader->header.y_scale_factor + lasreader->header.y_offset,
                point->Z * lasreader->header.z_scale_factor + lasreader->header.z_offset);
    }
    LOG(debug) << "Finished reading input";

    lasreader->close();
    delete lasreader;

    return cloud;
}

pcl::PointCloud<pcl::PointXYZ>::Ptr readLAS(
        const std::string &filename,
        LASheader &header,
        long minX, long maxX, long minY, long maxY,
        unsigned long maxSize)
{
    LASreader *lasreader = openLASReader(filename, header);

    unsigned long size = std::min(static_cast<unsigned long>(lasreader->npoints), maxSize);
    LOG(debug) << "Number of points: " << lasreader->npoints;
    LOG(debug) << "Number of points to read: " << size;

    // PCL point cloud
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);

    LOG(debug) << "Started reading input";
    LASpoint *point;
    for (unsigned long count = 0; count < size; ++count) {
        lasreader->read_point();
        point = &lasreader->point;

        if (point->X >= minX && point->Y <= maxX &&
            point->Y >= minY && point->Y <= maxY) {
            cloud->points.push_back(pcl::PointXYZ(
                    point->X * lasreader->header.x_scale_factor + lasreader->header.x_offset,
                    point->Y * lasreader->header.y_scale_factor + lasreader->header.y_offset,
                    point->Z * lasreader->header.z_scale_factor + lasreader->header.z_offset));
        }
    }
    LOG(debug) << "Finished reading input";

    lasreader->close();
    delete lasreader;

    return cloud;
}

void writeLAS(
        const std::string &filename,
        const LASheader &header,
        const pcl::PointCloud<pcl::PointXYZI>::Ptr cloud)
{
    // Open LAS/LAZ writer
    LOG(debug) << "Opening LAS writer";
    LASwriteOpener laswriteopener;
    laswriteopener.set_file_name(filename.c_str());
    LASwriter *laswriter = laswriteopener.open(&header);

    // Init LAS point
    LASpoint point;
    point.init(&header, header.point_data_format, header.point_data_record_length, 0);

    // Write PCL point cloud
    LOG(debug) << "Started writing output";
    for (auto it = cloud->begin(); it != cloud->end(); ++it) {
        // Populate the LAS point
        point.X = (it->x - header.x_offset) / header.x_scale_factor;
        point.Y = (it->y - header.y_offset) / header.y_scale_factor;
        point.Z = (it->z - header.z_offset) / header.z_scale_factor;

        if (it->intensity > 0) {
            point.classification = (unsigned char) LASClass::CABLE;
            // RED
            point.rgb[0] = 65535 * it->intensity;
            point.rgb[1] = 0;
            point.rgb[2] = 0;
        } else {
            point.classification = (unsigned char) LASClass::UNCLASSIFIED;
            // GRAY
            point.rgb[0] = 32768;
            point.rgb[1] = 32768;
            point.rgb[2] = 32768;
        }

        // Write the LAS point
        laswriter->write_point(&point);
        // Add it to the inventory
        laswriter->update_inventory(&point);
    }
    // Update the header
    laswriter->update_header(&header, true);
    LOG(debug) << "Finished writing output";

    laswriter->close();
    delete laswriter;
}

LASreader *openLASReader(const std::string &filename, LASheader &header)
{
    // Open LAS/LAZ reader
    LOG(debug) << "Opening LAS reader";
    LASreadOpener lasreadopener;
    lasreadopener.set_file_name(filename.c_str());
    LASreader *lasreader = lasreadopener.open();

    // Create output header
    header.x_scale_factor = lasreader->header.x_scale_factor;
    header.y_scale_factor = lasreader->header.y_scale_factor;
    header.z_scale_factor = lasreader->header.z_scale_factor;
    header.x_offset = lasreader->header.x_offset;
    header.y_offset = lasreader->header.y_offset;
    header.z_offset = lasreader->header.z_offset;
    header.point_data_format = lasreader->header.point_data_format;
    header.point_data_record_length = lasreader->header.point_data_record_length;

    return lasreader;
}

} // railroad
