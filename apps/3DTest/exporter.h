#ifndef EXPORTER_H
#define EXPORTER_H

#include <Voxel/RSlic3.h>
#include <vector>

void exportPLY(const std::vector<Vec3i> &vertex, const std::string &filename, RSlic::Voxel::MovieCacheP img, const string &comments = string());

std::vector<Vec3i> contourVertex(RSlic::Voxel::Slic3P p, const Mat & mask = Mat());


void showNr(RSlic::Voxel::Slic3P p, int i, bool tIgnore);

void showFirst(RSlic::Voxel::Slic3P p);

#endif // EXPORTER_H
