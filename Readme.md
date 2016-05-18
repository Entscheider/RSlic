# Overview
RSlic is a library written in C++ for computing Superpixel and Supervoxel using OpenCV. It based on the Slic-Algorithmus (http://ivrg.epfl.ch/research/superpixels).

# Features
- Compute Superpixel and Supervoxel
- Support for the zero parameter version of the SLIC algorithm (SLICO)
- Support for custom metrics
- Ability to save any iteration while computing (e.g. SuperPixelGUI makes use of it)

# Dependency
To build RSlic you need:
- OpenCV
- CMake
- optional: ccmake or cmake-gui for a configuration interface
- C++11 Compiler (e.g. gcc or clang)
- Qt4 or Qt5 for GUI-Apps

# Installing
Go into the source directory and run:

```
$ mkdir build && cd build
$ cmake ..
$ make
$ make install
```

Or run *ccmake* if you want to configure somethings before building. (E.g. you can disable thread usage. Or prefer Qt4 over Qt5)

# Create a project
## CMakeLists
Create a CMakeLists.txt for your project. If your project has the name myproj, the CMakeLists.txt should contains something like:

```
 find_package(RSlic REQUIRED)
include_directories(${RSLIC_INCLUDE_DIRS})
add_executable(myproj main.cpp ...)
target_link_libraries(myproj ${RSLIC_LIB})
```

By the way: Your project will (and have to) use C++11 now.

## Including the files
If you would like to use Superpixel the simplest way would be to import RSlic/RSlic2H.h:

```cpp
#include <RSlic/RSlic2H.h>
```

and if you want to use Supervoxel:

```cpp
#include <RSlic/RSlic3H.h>
```
Of course you can use Superpixel and Supervoxel at the same time.

## Simple introduction RSlic
Executing the Slic-Algorithmus is very simple. Let's say you have a gray image img, want n Superpixel with a special stiffness. Then:

```cpp
#include <RSlic/RSlic2H.h>
using namespace RSlic::Pixel;
....
Mat grad = buildGrad(img); //Build the gradient for the algorithm
int w = m.cols;
int h = m.rows;
int step = sqrt(w * h * 1.0 / n); //Calc the step-width
Slic2P slic = Slic2::initialize(img,grad,step,stiffness); //Initialize with the parameter
for (int i=0; i < 10; i++){ //Do 10 Iterations
  slic = slic->iterate<distanceGray>();
  //slic = slic->iterateZero<distanceGray>(); //For the zero parameter version
}
slic = slic->finalize<distanceGray>();
```

Now you can work with *slic*. Mainly you want to use
*slic->getClusters()*. See the documentation for more details.

You're not sure if you have a gray image or a colorful one?

```cpp
#include <RSlic/RSlic2H.h>
using namespace RSlic::Pixel;
....
Mat grad = buildGrad(img); //Build the gradient for the algorithms
int w = m.cols;
int h = m.rows;
int step = sqrt(w * h * 1.0 / n); //Calc the step-width
Slic2P slic = Slic2::initialize(img,grad,step,stiffness); //Initialize with the parameter
for (int i=0; i < 10; i++){ //Do 10 Iterations
  slic = iteratingHelper(slic, img.type());
  // slic = iteratingHelper(slic, img.type(), true); //For the zero parameter version
}
slic = slic->finalize<distanceGray>();
  ```

You think that's not simple enough? OK, an easier version:
```cpp
#include <RSlic/RSlic2H.h>
using namespace RSlic::Pixel;
....
//10-Iterations
Slic2P slic = shutUpAndTakeMyMoney(img, n, hardness, false, 10);
//10-Iterations with Slico
//Slic2P slic = shutUpAndTakeMyMoney(img, n, hardness, true, 10);
```

Note that Slic is often intended to be applied on top of LAB images. So you may want to convert your image before using the functions above. (Use OpenCV for this)

# 3rd Party Code
This library uses the ThreaPool file from https://github.com/progschj/ThreadPool. 
The RSlic library uses threads if 'PARALLEL' is enabled in CMakeCache.

# License
BSD license
