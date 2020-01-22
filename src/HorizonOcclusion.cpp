#include "tntn/geometrix.h"
#include "glm/glm.hpp"

namespace tntn {

//
// https://cesiumjs.org/2013/05/09/Computing-the-horizon-occlusion-point
//
double llh_ecef_radiusX = 6378137.0;
double llh_ecef_radiusY = 6378137.0;
double llh_ecef_radiusZ = 6356752.3142451793;
double llh_ecef_rX = 1.0 / llh_ecef_radiusX;
double llh_ecef_rY = 1.0 / llh_ecef_radiusY;
double llh_ecef_rZ = 1.0 / llh_ecef_radiusZ;

double magSquared(Vertex &v) {
  return (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
}

double mag(Vertex &v) {
  return std::sqrt(magSquared(v));
}

Vertex cross(Vertex &v, Vertex &other) {
  return Vertex(
    (v.y * other.z) - (other.y * v.z), 
    (v.z * other.x) - (other.z * v.x), 
    (v.x * other.y) - (other.x * v.y)
  );
}

double dot(Vertex &v, Vertex &other) {
  return (v.x * other.x) + (v.y * other.y) + (v.z * other.z);
}

double ocp_computeMagnitude(Vertex &position, Vertex &sphereCenter) {
  double magnitudeSquared = magSquared(position);
  double magnitude = std::sqrt(magnitudeSquared);
  Vertex direction = position * (1.0 / magnitude);

  // For the purpose of this computation, points below the ellipsoid
  // are considered to be on it instead.
  magnitudeSquared = std::fmax(1.0, magnitudeSquared);
  magnitude = std::fmax(1.0, magnitude);

  double cosAlpha = dot(direction, sphereCenter);
  Vertex sinAlphaTmp = cross(direction, sphereCenter);
  double sinAlpha = mag(sinAlphaTmp);
  double cosBeta = 1.0 / magnitude;
  double sinBeta = std::sqrt(magnitudeSquared - 1.0) * cosBeta;

  return 1.0 / (cosAlpha * cosBeta - sinAlpha * sinBeta);
}

Vertex ocp_fromPoints(SimpleRange<const Vertex*> &points, Vertex &boundingSphereCenter) {
  double max_magnitude = -std::numeric_limits<double>::infinity();

  // Bring coordinates to ellipsoid scaled coordinates
  const Vertex &center = boundingSphereCenter;
  Vertex scaledCenter = Vertex(center.x * llh_ecef_rX, center.y * llh_ecef_rY, center.z * llh_ecef_rZ);

  for (const Vertex* p = points.begin; p != points.end; p++) {
    Vertex scaledPoint = Vertex(p->x * llh_ecef_rX, p->y * llh_ecef_rY, p->z * llh_ecef_rZ);

    double magnitude = ocp_computeMagnitude(scaledPoint, scaledCenter);
    if (magnitude > max_magnitude) max_magnitude = magnitude;
  }
  return scaledCenter * max_magnitude;
}

}
