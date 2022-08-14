#ifndef POINT_SET_H
#define POINT_SET_H

#include <map>
#include <set>
#include <cmath>

template<class POINT, class DATA>
class PointData {
 public:
  class PointCmp {
   public:
    PointCmp(double tol=1E-6) :
     tol_(tol) {
    }

    bool operator()(const POINT &p1, const POINT &p2) const {
      return (less(p1.x(), p2.x()) || (equal(p1.x(), p2.x()) && less(p1.y(), p2.y())));
    }

   private:
    bool less(double a, double b) const {
      return ! equal(a, b) && a < b;
    }

    bool equal(double a, double b) const {
      return (fabs(a - b) < tol_);
    }

   private:
    double tol_;
  };

 private:
  typedef std::set<DATA>                     DataSet;
  typedef std::map<POINT, DataSet, PointCmp> Points;

 public:
  typedef typename Points::iterator       iterator;
  typedef typename Points::const_iterator const_iterator;

  PointData(double tol=1E-6) :
   points_(PointCmp(tol)) {
  }

  iterator begin() { return points_.begin(); }
  iterator end  () { return points_.end  (); }

  const_iterator begin() const { return points_.begin(); }
  const_iterator end  () const { return points_.begin(); }

  void clear() {
    points_.clear();
  }

  void addData(const POINT &point, const DATA &data) {
    auto p = points_.find(point);

    if (p == points_.end())
      p = points_.insert(p, typename Points::value_type(point, DataSet()));

    (*p).second.insert(data);
  }

  bool hasData(const POINT &point) const {
    return (points_.find(point) != points_.end());
  }

  const DATA &getData(const POINT &point) const {
    auto p = points_.find(point);

    assert(p != points_.end());

    return (*p).second;
  }

 private:
  Points points_;
};

#endif
