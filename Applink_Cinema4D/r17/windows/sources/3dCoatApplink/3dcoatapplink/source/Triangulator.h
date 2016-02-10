#ifndef TRIANGULATOR_H
#define TRIANGULATOR_H

class Triangulator
{
public:
  virtual void           addPoint(double x,double y,double z) = 0; // add a point to the contour
  virtual void           addPoint(float x,float y,float z) = 0; // add a point to the contour

  virtual unsigned int * triangulate(unsigned int &tcount,double epsilon=0.00000000001) = 0; // triangulate using the provided epsilon

  virtual void           reset(void) = 0; // reset contour

  virtual void           getPoint(unsigned int index,float &x,float &y,float &z) const = 0; // retrieve one of the points in the contour.
  virtual void           getPoint(unsigned int index,double &x,double &y,double &z) const = 0; // retrieve one of the points in the contour.

protected:
  virtual ~Triangulator(void) { };
};

Triangulator * createTriangulator(void);
void           releaseTriangulator(Triangulator *t);

#endif
