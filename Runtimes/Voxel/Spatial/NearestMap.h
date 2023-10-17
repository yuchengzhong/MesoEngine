// Meso Engine 2024
#include <iostream>
#include <vector>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <glm/glm.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
typedef bg::model::point<float, 3, bg::cs::cartesian> Point;

template<typename DataType>
class TNearestMap 
{
private:
    std::vector<DataType> Data;
    typedef std::pair<Point, uint32_t> ValueType;
    bgi::rtree<ValueType, bgi::quadratic<16>> RTree;

public:
    void Insert(const glm::vec3& Vec, const DataType& Data_) 
    {
        Data.push_back(Data_);
        RTree.insert(std::make_pair(Point(Vec.x, Vec.y, Vec.z), (uint32_t)Data.size() - 1));
    }
    void Insert(const glm::vec3& Vec, DataType&& Data_)
    {
        Data.push_back(std::move(Data_));
        RTree.insert(std::make_pair(Point(Vec.x, Vec.y, Vec.z), (uint32_t)Data.size() - 1));
    }

    DataType& Query(const glm::vec3& QueryPoint_)
    {
        std::vector<ValueType> Result;
        Point QueryPoint = Point(QueryPoint_.x, QueryPoint_.y, QueryPoint_.z);
        RTree.query(bgi::nearest(QueryPoint, 1), std::back_inserter(Result));
        if (!Result.empty())
        {
            uint32_t DataIndex = Result[0].second;
            if (DataIndex < (uint32_t)Data.size())
            {
                return Data[DataIndex];
            }
            throw std::runtime_error("RTree.query result invalid");
        }
        throw std::runtime_error("No nearest neighbor found");
    }
    DataType Query(const glm::vec3& QueryPoint_) const
    {
        std::vector<ValueType> Result;
        Point QueryPoint = Point(QueryPoint_.x, QueryPoint_.y, QueryPoint_.z);
        RTree.query(bgi::nearest(QueryPoint, 1), std::back_inserter(Result));
        if (!Result.empty())
        {
            uint32_t DataIndex = Result[0].second;
            if (DataIndex < (uint32_t)Data.size())
            {
                return Data[DataIndex];
            }
            throw std::runtime_error("RTree.query result invalid");
        }
        throw std::runtime_error("No nearest neighbor found");
    }
};