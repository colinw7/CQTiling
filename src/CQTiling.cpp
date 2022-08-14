#include <CQTiling.h>
#include <CQPropertyTree.h>
#include <CQPropertyItem.h>
#include <CQPropertyEditor.h>
#include <QApplication>
#include <QSplitter>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QHelpEvent>
#include <QToolTip>
#include <cassert>
#include <iostream>

static QColor colors[] = {
  QColor("#FFFFFF"),  // 0
  QColor("#FFFFFF"),  // 1
  QColor("#FFFFFF"),  // 2
  QColor("#477984"),  // 3
  QColor("#EEAA4D"),  // 4
  QColor("#FF0000"),  // 5
  QColor("#C03C44"),  // 6
  QColor("#FFFFFF"),  // 7
  QColor("#756BB1"),  // 8
  QColor("#FFFFFF"),  // 9
  QColor("#FFFFFF"),  // 10
  QColor("#FFFFFF"),  // 11
  QColor("#2078B4"),  // 12
  QColor("#FFFFFF"),  // 13
};

using ModelUtil::range;
using ModelUtil::rangeBy;
using ModelUtil::scalePoint;

void drawText(QPainter *p, const QPointF &pos, const QString &text,
              const QColor &c=QColor("#FFFFFF"))
{
  p->save();

  p->setWorldMatrixEnabled(false);

  //auto font = p->font();

  //font.setPixelSize(14);

  //p->setFont(font);

  QFontMetrics fm(p->font());

  p->setPen(c);

  auto pos1 = p->transform().map(pos) +
    QPointF(-fm.width(text)/2.0, (fm.ascent() - fm.descent())/2.0);

  p->drawText(pos1, text);

  p->setWorldMatrixEnabled(true);

  p->restore();
}

//------

Model::
Model(Canvas *canvas) :
 canvas_(canvas), margin_(0.1), showSides_(false), bgColor_("#000000"),
 borderColor_("#313E4A"), borderWidth_(0.05), shapeQuadTree_(Rect(-100, -100, 100, 100))
{
}

Model::
~Model()
{
  for (auto shape : shapes_)
    delete shape;
}

void
Model::
reset()
{
  for (auto shape : shapes_)
    delete shape;

  shapes_.clear();

  shapeQuadTree_.reset();

  points_.clear();
}

void
Model::
setMargin(double m)
{
  margin_ = m;

  canvas_->update();
}

void
Model::
setShowSides(bool b)
{
  showSides_ = b;

  canvas_->update();
}

void
Model::
setBgColor(const QColor &c)
{
  bgColor_ = c;

  canvas_->update();
}

void
Model::
setBorderColor(const QColor &c)
{
  borderColor_ = c;

  canvas_->update();
}

void
Model::
setBorderWidth(double w)
{
  borderWidth_ = w;

  canvas_->update();
}

Shape *
Model::
createShape(int numSides)
{
  Shape *shape = new Shape(this, numSides, colors[numSides]);

  shape->setId(int(shapes_.size()));

  return shape;
}

void
Model::
storeShape(Shape *shape)
{
  shapes_.push_back(shape);

  shapeQuadTree_.add(shape);

  shape->updatePoly();

  for (auto point : shape->poly())
    points_.addData(point, shape);
}

int
Model::
addShape(int numSides)
{
  Shape *shape = createShape(numSides);

  storeShape(shape);

  updateShapeSides();

  return shape->id();
}

std::vector<int>
Model::
addShapesToSides(const std::vector<int> &shapeIds, const std::vector<int> &sideNums, int numSides)
{
  std::vector<int> shapeIds1;

  for (auto shapeId : shapeIds) {
    Shape *shape = getShape(shapeId);

    if (shape->fullyOccupied())
      continue;

    for (auto sideNum : sideNums) {
      Side &side = shape->side(sideNum);

      if (side.hasShapeSide())
        continue;

      //---

      Shape *shape1 = createShape(numSides);

      placeShape(shape, sideNum, shape1);

      Shape *shape2 = getShapeAtPos(shape1->pos());

      if (shape2 && shape2->numSides() == shape1->numSides()) {
        shape2->copyOver(shape1);

        delete shape1;

        shapeIds1.push_back(shape2->id());

        continue;
      }

      storeShape(shape1);

      shapeIds1.push_back(shape1->id());
    }
  }

  updateShapeSides();

  return shapeIds1;
}

void
Model::
placeShape(Shape *shape, int sideNum, Shape *shape1)
{
  Side &side = shape->side(sideNum);

  Side &side1 = shape1->side(0);

  double l = side.length()/side1.length();

  shape1->scale(l);

  double l2 = ModelUtil::dist(QPointF(0,0), side1.mid());

  shape1->translate(side.mid() + l2*side.vector(shape->pos()));

  // rotate shape1 to it's first side is at the same angle as adjacent side (mirrored)
  double a = M_PI + side.angle() - side1.angle();

  shape1->rotate(a);
}

void
Model::
repeat(int depth)
{
  std::vector<Shape *> repeatShapes = shapes_;

  Shape *repeatShape = repeatShapes.front();

  for (int i = 0; i < depth; ++i) {
    std::vector<int> shapeIds;

    std::vector<Shape *> shapes1 = shapes_;

    for (auto shape : shapes1) {
      if (shape->numSides() != repeatShape->numSides() || shape->fullyOccupied())
        continue;

      if (! ModelUtil::realEq(shape->angle(), repeatShape->angle()))
        continue;

      auto d = shape->pos() - repeatShape->pos();

      for (auto shape1 : repeatShapes) {
        Shape *shape2 = shape1->dup();

        shape2->setId(int(shapes_.size()));

        shape2->translate(d);

        Shape *shape3 = getShapeAtPos(shape2->pos());

        if (shape3) {
          delete shape2;

          continue;
        }

        storeShape(shape2);

        shapeIds.push_back(shape2->id());
      }
    }

    updateShapeSides();
  }
}

void
Model::
updateShapeSides()
{
  for (auto shape : shapes_)
    shape->updateSides();
}

QRectF
Model::
getBBox() const
{
  QRectF r(-1.0, -1.0, 1.0, 1.0);

  for (auto shape : shapes_)
    r |= shape->getBBox();

  return r;
}

Shape *
Model::
getShapeAtPos(const QPointF &p) const
{
  ShapeQuadTree::DataList shapes;

  shapeQuadTree_.getDataAtPoint(p.x(), p.y(), shapes);

  for (auto shape : shapes)
    if (shape->contains(p))
      return shape;

  return 0;
}

void
Model::
draw(QPainter *p)
{
  if (canvas_->dual()) {
    for (auto &point : points_)
      drawDual(p, point.first, point.second);
  }
  else {
    for (auto shape : shapes_)
      shape->draw(p);
  }
}

void
Model::
drawDual(QPainter *p, const QPointF &point, const std::set<Shape *> &shapes)
{
  p->setPen(QPen(QColor(255,0,0), 0.03));
  p->drawPoint(point);

  std::vector<QPointF> points;

  for (auto shape : shapes) {
    auto p = scalePoint(shape->pos(), 0.1, point);

    points.push_back(p);
  }

  if (points.size() > 2) {
    std::sort(points.begin(), points.end(), PointAngleCmp(point));

    QPainterPath path;

    for (auto point : points) {
      if (! path.elementCount())
        path.moveTo(point);
      else
        path.lineTo(point);
    }

    path.closeSubpath();

    if (borderWidth() > 0.0)
      p->setPen(QPen(borderColor(), borderWidth()));
    else
      p->setPen(QPen(QColor(0,0,0,0)));

    p->setBrush(QColor("#477984"));

    p->drawPath(path);
  }
}

//------

Shape::
Shape(Model *model, int numSides, const QColor &c) :
 model_(model), id_(0), numSides_(numSides), numOccupied_(0), c_(c), pos_(0, 0)
{
  addSides();
}

Shape::
~Shape()
{
  for (auto side : sides_)
    delete side;
}

Shape *
Shape::
dup() const
{
  Shape *shape = new Shape(model_, 0, c_);

  shape->id_       = 0;
  shape->numSides_ = numSides_;
  shape->pos_      = pos_;

  for (auto &side : sides_)
    shape->sides_.push_back(side->dup(shape));

  return shape;
}

void
Shape::
copyOver(Shape *shape)
{
  std::swap(sides_, shape->sides_);

  poly_.clear();
}

void
Shape::
addSides()
{
  double da = 2.0*M_PI/numSides();
  double a  = (! (numSides() % 2) ? -da/2.0 : 0.0);

  QPointF p1;

  for (auto i : range(numSides() + 1)) {
    double s = sin(a);
    double c = cos(a);

    auto p2 = QPointF(pos().x() + c, pos().y() + s);

    if (i > 0)
      sides_.push_back(new Side(this, i - 1, p1, p2));

    a += da;

    p1 = p2;
  }
}

void
Shape::
translate(const QPointF &p)
{
  pos_ += p;

  for (auto &side : sides_)
    side->move(p);

  poly_.clear();
}

void
Shape::
scale(double s)
{
  for (auto &side : sides_)
    side->scale(pos(), s);

  poly_.clear();
}

void
Shape::
rotate(double a)
{
  for (auto &side : sides_)
    side->rotate(pos(), a);

  poly_.clear();
}

QRectF
Shape::
getBBox() const
{
  QRectF r(pos_, QSizeF(0.01, 0.01));

  for (auto side : sides_)
    r |= side->getBBox();

  return r;
}

double
Shape::
angle() const
{
  double a = 1E50;

  for (auto side : sides_)
    a = std::min(a, side->angle());

  return a;
}

bool
Shape::
contains(const QPointF &p) const
{
  updatePoly();

  return ipoly_.containsPoint(p, Qt::OddEvenFill);
}

void
Shape::
updatePoly() const
{
  if (! poly_.empty()) return;

  QVector<QPointF> points, ipoints;

  for (auto side : sides_) {
    auto p = side->start();

    points.push_back(p);

    auto ip = scalePoint(p, 0.1, pos());

    ipoints.push_back(ip);
  }

  poly_  = QPolygonF(points);
  ipoly_ = QPolygonF(ipoints);
}

void
Shape::
updateSides()
{
  numOccupied_ = 0;

  for (auto side : sides_) {
    auto p = side->mid() + 0.01*side->vector(pos());

    Shape *shape = model_->getShapeAtPos(p);
    if (! shape) continue;

    side->setShapeSide(shape->id(), shape->getSide(p));

    ++numOccupied_;
  }
}

int
Shape::
getSide(const QPointF &p) const
{
  double minDist = 0;
  int    minSide = -1;

  for (auto side : sides_) {
    double d = ModelUtil::dist(p, side->mid());

    if (minSide < 0 || d < minDist) {
      minSide = side->num();
      minDist = d;
    }
  }

  return minSide;
}

QString
Shape::
tip() const
{
  auto str = QString("%1:").arg(id());

  for (auto side : sides_) {
    if (! side->hasShapeSide())
      str += QString(" %1").arg(side->num());
  }

  return str;
}

void
Shape::
draw(QPainter *p)
{
  double s = model_->margin();

  QPainterPath path;

  for (auto side : sides_) {
    if (! path.elementCount())
      path.moveTo(scalePoint(side->start(), s, pos()));

    path.lineTo(scalePoint(side->end(), s, pos()));
  }

  path.closeSubpath();

  if (model_->borderWidth() > 0.0)
    p->setPen(QPen(model_->borderColor(), model_->borderWidth()));
  else
    p->setPen(QPen(QColor(0,0,0,0)));

  p->setBrush(c_);

  p->drawPath(path);

  //---

  if (model_->showSides()) {
    p->setPen(QPen(QColor(255,0,0), 0.03));

    for (auto side : sides_) {
      if (side->hasShapeSide()) continue;

      //p->drawLine(side->start(), side->end());

      drawText(p, side->mid(), QString("%1").arg(side->num()));
    }

    drawText(p, pos(), QString("%1").arg(id()));
  }
}

//------

Canvas::
Canvas(QWidget *parent) :
 QWidget(parent), modelNum_(9), scale_(1.0), repeatCount_(0), dual_(false),
 printSize_(1024, 1024)
{
  model_ = new Model(this);

  addShapes(modelNum_);
}

Canvas::
~Canvas()
{
  delete model_;
}

void
Canvas::
addShapes(int id)
{
  model_->reset();

  if      (id == 0) {
    auto shapeId = model_->addShape(6);

    auto shapeIds1 = model_->addShapesToSides({shapeId}, range(6), 3);
    auto shapeIds2 = model_->addShapesToSides(shapeIds1, {1}     , 6);

    model_->repeat(repeatCount());
  }
  else if (id == 1) {
    auto shapeId = model_->addShape(12);

    auto shapeIds1 = model_->addShapesToSides({shapeId}, rangeBy(0, 12, 2), 6);
    auto shapeIds2 = model_->addShapesToSides({shapeId}, rangeBy(1, 12, 2), 4);
    auto shapeIds3 = model_->addShapesToSides(shapeIds2, {2}              , 12);

    model_->repeat(repeatCount());
  }
  else if (id == 2) {
    auto shapeId = model_->addShape(4);

    auto shapeIds1 = model_->addShapesToSides({shapeId}, range(4), 3);
    auto shapeIds2 = model_->addShapesToSides(shapeIds1, {1}     , 4);
    auto shapeIds3 = model_->addShapesToSides(shapeIds2, {2, 3}  , 3);
    auto shapeIds4 = model_->addShapesToSides(shapeIds3, {2}     , 4);

    model_->repeat(repeatCount());
  }
  else if (id == 3) {
    auto shapeId = model_->addShape(6);

    auto shapeIds1 = model_->addShapesToSides({shapeId}, range(6), 3);
    auto shapeIds2 = model_->addShapesToSides(shapeIds1, {1}     , 3);
    auto shapeIds3 = model_->addShapesToSides(shapeIds1, {2}     , 3);
    auto shapeIds4 = model_->addShapesToSides(shapeIds3, {1}     , 6);

    model_->repeat(repeatCount());
  }
  else if (id == 4) {
    auto shapeId = model_->addShape(8);

    auto shapeIds1 = model_->addShapesToSides({shapeId}, rangeBy(1, 8, 2), 4);
    auto shapeIds2 = model_->addShapesToSides(shapeIds1, {1}             , 8);

    model_->repeat(repeatCount());
  }
  else if (id == 5) {
    auto shapeId = model_->addShape(12);

    auto shapeIds1 = model_->addShapesToSides({shapeId}, rangeBy(0, 12, 2), 3);
    auto shapeIds2 = model_->addShapesToSides({shapeId}, rangeBy(1, 12, 2), 4);
    auto shapeIds3 = model_->addShapesToSides(shapeIds2, {1, 3}           , 3);
    auto shapeIds4 = model_->addShapesToSides(shapeIds2, {2}              , 12);

    model_->repeat(repeatCount());
  }
  else if (id == 6) {
    auto shapeId = model_->addShape(6);

    auto shapeIds1 = model_->addShapesToSides({shapeId}, range(6), 4);
    auto shapeIds2 = model_->addShapesToSides(shapeIds1, {1}     , 3);
    auto shapeIds3 = model_->addShapesToSides(shapeIds1, {2}     , 6);

    model_->repeat(repeatCount());
  }
  else if (id == 7) {
    auto shapeId = model_->addShape(4);

    auto shapeIds1 = model_->addShapesToSides({shapeId}, {0, 2}, 4); shapeIds1.push_back(0);
    auto shapeIds2 = model_->addShapesToSides(shapeIds1, {1, 3}, 3);
    auto shapeIds3 = model_->addShapesToSides(shapeIds2, {1   }, 3);
    auto shapeIds4 = model_->addShapesToSides(shapeIds3, {2   }, 4);

    model_->repeat(repeatCount());
  }
  else if (id == 8) {
    auto shapeId = model_->addShape(3);

    auto shapeIds1 = model_->addShapesToSides({shapeId}, range(3), 3);
    auto shapeIds2 = model_->addShapesToSides(shapeIds1, {1, 2}  , 3);

    model_->repeat(repeatCount());
  }
  else if (id == 9) {
    auto shapeId = model_->addShape(5);

    auto shapeIds1 = model_->addShapesToSides({shapeId}, range(5), 4);

    for (auto i : range(8)) {
      assert(i >= 0);
      auto shapeIds2 = model_->addShapesToSides(shapeIds1, {2}, 5);
           shapeIds1 = model_->addShapesToSides(shapeIds2, {2}, 4);
    }
  }
  else if (id == 10) {
    auto shapeId = model_->addShape(6);

    auto shapeIds1 = model_->addShapesToSides({shapeId}, range(6), 4);
    auto shapeIds2 = model_->addShapesToSides(shapeIds1, {2}     , 3);
    auto shapeIds3 = model_->addShapesToSides(shapeIds2, {1}     , 4);
    auto shapeIds4 = model_->addShapesToSides(shapeIds2, {2}     , 4);
    auto shapeIds5 = model_->addShapesToSides(shapeIds4, {2}     , 6);

    (void) model_->addShapesToSides(shapeIds1, {1}, 3);
    (void) model_->addShapesToSides(shapeIds3, {1}, 3);

    model_->repeat(repeatCount());
  }
  else if (id == 11) {
    auto shapeId = model_->addShape(8);

    auto shapeIds1 = model_->addShapesToSides({shapeId}, rangeBy(0, 8, 2), 6);
    auto shapeIds2 = model_->addShapesToSides(shapeIds1, {3}             , 8);

    model_->repeat(repeatCount());
  }
  else if (id == 12) {
    auto shapeId = model_->addShape(12);

    auto shapeIds1 = model_->addShapesToSides({shapeId}, rangeBy(0, 12, 2), 3);
    auto shapeIds2 = model_->addShapesToSides({shapeId}, rangeBy(1, 12, 2), 12);

    model_->repeat(repeatCount());
  }
}

void
Canvas::
setModelNum(int n)
{
  if (n == modelNum_) return;

  modelNum_ = n;

  addShapes(modelNum_);

  update();
}

void
Canvas::
setScale(double s)
{
  scale_ = s;

  update();
}

void
Canvas::
setRepeatCount(int n)
{
  if (n == repeatCount_) return;

  repeatCount_ = n;

  addShapes(modelNum_);

  update();
}

void
Canvas::
setDual(bool b)
{
  if (b == dual_) return;

  dual_ = b;

  update();
}

void
Canvas::
addPropeties(CQPropertyTree *tree)
{
  auto *iedit = new CQPropertyIntegerEditor;

  tree->addProperty("Canvas", this, "modelNum"   )->setEditorFactory(iedit);
  tree->addProperty("Canvas", this, "scale"      );
  tree->addProperty("Canvas", this, "repeatCount")->setEditorFactory(iedit);
  tree->addProperty("Canvas", this, "dual"       );
  tree->addProperty("Canvas", this, "printSize"  );

  tree->addProperty("Model" , model_, "margin"     );
  tree->addProperty("Model" , model_, "showSides"  );
  tree->addProperty("Model" , model_, "bgColor"    );
  tree->addProperty("Model" , model_, "borderColor");
  tree->addProperty("Model" , model_, "borderWidth");
}

void
Canvas::
resizeEvent(QResizeEvent *)
{
  w_ = width ();
  h_ = height();
}

void
Canvas::
paintEvent(QPaintEvent *)
{
  QPainter p(this);

  paint(&p);
}

void
Canvas::
paint(QPainter *p)
{
  w_ = p->device()->width ();
  h_ = p->device()->height();

  p->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

  p->fillRect(QRect(0, 0, w_, h_), QBrush(model_->bgColor()));

  auto r = model_->getBBox();

  double s = std::max(r.width(), r.height());

  transform_.reset();

  transform_.scale    (w_/s, h_/s);
  transform_.translate(s/2.0, s/2.0);
  transform_.scale    (scale(), -scale());

  p->setTransform(transform_);

  itransform_ = transform_.inverted();

  model_->draw(p);
}

void
Canvas::
mouseMoveEvent(QMouseEvent *)
{
}

bool
Canvas::
event(QEvent *e)
{
  if (e->type() == QEvent::ToolTip) {
    auto *helpEvent = static_cast<QHelpEvent *>(e);

    auto p = itransform_.map(QPointF(helpEvent->pos()));

    Shape *shape = model_->getShapeAtPos(p);

    if (shape) {
      auto rect = transform_.mapRect(shape->getBBox());

      QToolTip::showText(helpEvent->globalPos(), shape->tip(), this, rect.toRect());
    }

    return true;
  }

  return QWidget::event(e);
}

void
Canvas::
print()
{
  QImage image(printSize(), QImage::Format_ARGB32);

  QPainter p(&image);

  paint(&p);

  image.save("print.png");
}

//------

Dialog::
Dialog()
{
  auto *layout = new QHBoxLayout(this);
  layout->setMargin(2); layout->setSpacing(2);

  auto *splitter = new QSplitter;

  Canvas *canvas = new Canvas;

  canvas->setMinimumSize(QSize(800, 800));

  splitter->addWidget(canvas);

  auto *rframe = new QFrame;

  rframe->setMinimumWidth(275);

  auto *rlayout = new QVBoxLayout(rframe);
  rlayout->setMargin(2); rlayout->setSpacing(2);

  auto *tree = new CQPropertyTree;

  canvas->addPropeties(tree);

  rlayout->addWidget(tree);

  auto *buttonFrame = new QFrame;

  auto *buttonLayout = new QHBoxLayout(buttonFrame);
  buttonLayout->setMargin(2); buttonLayout->setSpacing(2);

  auto *printButton = new QPushButton("Print");

  connect(printButton, SIGNAL(clicked()), canvas, SLOT(print()));

  buttonLayout->addWidget(printButton);
  buttonLayout->addStretch(1);

  rlayout->addWidget(buttonFrame);

  splitter->addWidget(rframe);

  layout->addWidget(splitter);
}

//------

int
main(int argc, char **argv)
{
  QApplication app(argc, argv);

  Dialog *dialog = new Dialog;

  dialog->show();

  return app.exec();
}
