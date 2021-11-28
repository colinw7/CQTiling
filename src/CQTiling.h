#ifndef CQTiling_H
#define CQTiling_H

#include <QWidget>
#include <CQuadTree.h>
#include <PointSet.h>

class CQPropertyTree;

class QPainter;

class Canvas;
class Shape;
class Side;

class PointAngleCmp {
 public:
  PointAngleCmp(const QPointF &c) :
   c_(c) {
  }

  bool operator()(const QPointF &p1, const QPointF &p2) {
    double a1 = atan2(p1.y() - c_.y(), p1.x() - c_.x());
    double a2 = atan2(p2.y() - c_.y(), p2.x() - c_.x());

    return (a1 < a2);
  }

 private:
  QPointF c_;
};

class Rect : public QRectF {
 public:
  Rect(const QRectF &r=QRectF()) :
   QRectF(r) {
  }

  Rect(double l, double b, double r, double t) :
   QRectF(l, b, r - l, t - b) {
  }

  double getLeft  () const { return left  (); }
  double getBottom() const { return top   (); }
  double getRight () const { return right (); }
  double getTop   () const { return bottom(); }
};

typedef CQuadTree<Shape,Rect> ShapeQuadTree;

namespace ModelUtil {

inline double hypot(const QPointF &v) {
  return std::hypot(v.x(), v.y());
}

inline double dist(const QPointF &p1, const QPointF &p2) {
  return hypot(p2 - p1);
}

inline bool realEq(double r1, double r2) {
  return fabs(r2 - r1) < 1E-3;
}

inline QPointF scalePoint(const QPointF &p, double s, const QPointF &o=QPointF(0,0)) {
  QPointF v = p - o;
  double  l = hypot(v);

  return o + (v - s*v/l);
}

inline std::vector<int> range(int n, int i=0) {
  std::vector<int> v(n);

  std::iota(v.begin(), v.end(), i);

  return v;
}

inline std::vector<int> rangeBy(int i1, int i2, int s=1) {
  std::vector<int> v;

  for (int i = i1, j = 0; i < i2; i += s, ++j)
    v.push_back(i);

  return v;
}

}

//---

class Model : public QObject {
  Q_OBJECT

  Q_PROPERTY(double margin      READ margin      WRITE setMargin     )
  Q_PROPERTY(bool   showSides   READ showSides   WRITE setShowSides  )
  Q_PROPERTY(QColor bgColor     READ bgColor     WRITE setBgColor    )
  Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor)
  Q_PROPERTY(double borderWidth READ borderWidth WRITE setBorderWidth)

 public:
  Model(Canvas *canvas);
 ~Model();

  double margin() const { return margin_; }
  void setMargin(double m);

  bool showSides() const { return showSides_; }
  void setShowSides(bool b);

  const QColor &bgColor() const { return bgColor_; }
  void setBgColor(const QColor &c);

  const QColor &borderColor() const { return borderColor_; }
  void setBorderColor(const QColor &c);

  double borderWidth() const { return borderWidth_; }
  void setBorderWidth(double w);

  void reset();

  int addShape(int numSides);

  std::vector<int> addShapesToSides(const std::vector<int> &shapeIds,
                                    const std::vector<int> &sideNums, int numSides);

  void repeat(int depth);

  QRectF getBBox() const;

  Shape *getShape(int shapeId) const { return shapes_[shapeId]; }

  Shape *getShapeAtPos(const QPointF &p) const;

  void draw(QPainter *p);

  void drawDual(QPainter *p, const QPointF &point, const std::set<Shape *> &shape);

 private:
  Shape *createShape(int numSides);

  void storeShape(Shape *shape);

  void updateShapeSides();

  void placeShape(Shape *shape, int sideNum, Shape *shape1);

  void addShapeAtPos(Shape *shape);

 private:
  typedef std::vector<Shape *>        Shapes;
  typedef std::vector<Shape *>        PosShapes;
  typedef PointData<QPointF, Shape *> Points;

  Canvas*       canvas_;
  double        margin_;
  bool          showSides_;
  QColor        bgColor_;
  QColor        borderColor_;
  double        borderWidth_;
  Shapes        shapes_;
  PosShapes     posShapes_;
  ShapeQuadTree shapeQuadTree_;
  Points        points_;
};

//---

typedef std::pair<QPointF,QPointF> SideVector;

//---

struct ShapeSide {
  int shapeId;
  int sideNum;

  ShapeSide(int shapeId1=-1, int sideNum1=-1) :
   shapeId(shapeId1), sideNum(sideNum1) {
  }

  bool isValid() const { return shapeId >= 0; }
};

//---

class Side {
 public:
  Side(Shape *shape, int num=0, const QPointF &start=QPointF(), const QPointF &end=QPointF()) :
   shape_(shape), num_(num), start_(start), end_(end), shapeSide_(-1,-1) {
  }

  Shape *shape() const { return shape_; }

  void setNum(int num) { num_ = num; }
  int num() const { return num_; }

  const QPointF &start() const { return start_; }
  const QPointF &end  () const { return end_  ; }

  Side *dup(Shape *shape) const {
    Side *side = new Side(shape, num_, start_, end_);

    return side;
  }

  bool hasShapeSide() const { return shapeSide_.isValid(); }

  const ShapeSide &shapeSide() const { return shapeSide_; }

  void setShapeSide(int shapeId, int sideNum) {
    shapeSide_ = ShapeSide(shapeId, sideNum);
  }

  double length() const {
    return ModelUtil::dist(start_, end_);
  }

  double angle() const {
    QPointF d = end_ - start_;

    return atan2(d.y(), d.x());
  }

  QPointF mid() const {
    return (start_ + end_)/2.0;
  }

  QPointF vector(const QPointF &pos) const {
    QPointF pv = mid() - pos;

    return pv/ModelUtil::hypot(pv);
  }

  void move(const QPointF &p) {
    start_ += p;
    end_   += p;
  }

  void scale(const QPointF &o, double s) {
    start_ = s*(start_ - o) + o;
    end_   = s*(end_   - o) + o;
  }

  void rotate(const QPointF &o, double a) {
    double s = sin(a);
    double c = cos(a);

    QPointF d1 = start_ - o;
    QPointF d2 = end_   - o;

    start_ = QPointF(d1.x()*c - d1.y()*s, d1.x()*s + d1.y()*c) + o;
    end_   = QPointF(d2.x()*c - d2.y()*s, d2.x()*s + d2.y()*c) + o;
  }

  QRectF getBBox() const {
    double x1 = std::min(start_.x(), end_.x());
    double y1 = std::min(start_.y(), end_.y());
    double x2 = std::max(start_.x(), end_.x());
    double y2 = std::max(start_.y(), end_.y());

    return QRectF(x1, y1, x2 - x1, y2 - y1);
  }

 private:
  Shape     *shape_;
  int        num_;
  QPointF    start_;
  QPointF    end_;
  ShapeSide  shapeSide_;
};

//---

class Shape : public QObject {
  Q_OBJECT

 public:
  typedef std::vector<Side *> Sides;

 public:
  Shape(Model *model, int numSides, const QColor &c);
 ~Shape();

  void setId(int id) { id_ = id; }
  int id() const { return id_; }

  int numSides() const { return numSides_; }

  const Sides &sides() const { return sides_; }

  const Side &side(int sideNum) const { return *sides_[sideNum]; }
  Side &side(int sideNum) { return *sides_[sideNum]; }

  const QColor &color() const { return c_; }

  const QPointF &pos() const { return pos_; }

  const QPolygonF &poly() const { return poly_; }

  Shape *dup() const;

  void copyOver(Shape *shape);

  void translate(const QPointF &p);

  void scale(double s);

  void rotate(double a);

  QRectF getBBox() const;

  double angle() const;

  bool contains(const QPointF &p) const;

  void updatePoly() const;

  void updateSides();

  int getSide(const QPointF &p) const;

  int numOccupied() const { return numOccupied_; }

  bool fullyOccupied() const { return numOccupied_ == numSides_; }

  QString tip() const;

  void draw(QPainter *p);

 private:
  void addSides();

 private:
  Model*            model_;
  int               id_;
  int               numSides_;
  Sides             sides_;
  int               numOccupied_;
  QColor            c_;
  QPointF           pos_;
  mutable QPolygonF poly_;
  mutable QPolygonF ipoly_;
};

//---

class Dialog : public QWidget {
  Q_OBJECT

 public:
  Dialog();
};

class Canvas : public QWidget {
  Q_OBJECT

  Q_PROPERTY(int    modelNum    READ modelNum    WRITE setModelNum   )
  Q_PROPERTY(double scale       READ scale       WRITE setScale      )
  Q_PROPERTY(int    repeatCount READ repeatCount WRITE setRepeatCount)
  Q_PROPERTY(bool   dual        READ dual        WRITE setDual       )
  Q_PROPERTY(QSize  printSize   READ printSize   WRITE setPrintSize  )

 public:
  Canvas(QWidget *parent=0);
 ~Canvas();

  int modelNum() const { return modelNum_; }
  void setModelNum(int n);

  double scale() const { return scale_; }
  void setScale(double s);

  int repeatCount() const { return repeatCount_; }
  void setRepeatCount(int n);

  bool dual() const { return dual_; }
  void setDual(bool b);

  const QSize &printSize() const { return printSize_; }
  void setPrintSize(const QSize &s) { printSize_ = s; }

  void addShapes(int modelNum);

  void addPropeties(CQPropertyTree *tree);

  void paint(QPainter *p);

 private:
  void resizeEvent(QResizeEvent *) override;

  void paintEvent(QPaintEvent *) override;

  void mouseMoveEvent(QMouseEvent *e) override;

  bool event(QEvent *e) override;

 public slots:
  void print();

 private:
  Model*     model_;
  int        w_, h_;
  int        modelNum_;
  double     scale_;
  int        repeatCount_;
  bool       dual_;
  QSize      printSize_;
  QTransform transform_;
  QTransform itransform_;
};

#endif
