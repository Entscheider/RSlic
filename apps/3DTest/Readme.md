# 3DTest

3DTest is a program which creates a point cloud from several images with Supervoxel.
The export format is ply.

Additionally, a window appears which shows the result in pictures.
The following keys help navigate through these pictures:

- a Go a picture backward
- s Go a picture forward
- t Toggle between showing contour in z-direction
- d Show some debug information
- q Close the window

The following arguments are allowed on the command line:

- -c Number of Supervoxel (optional)
- -m Stiffness (optional)
- -t Number of threads to be used (optional)
- -0 Use Slico (optional)
- -h Show some help and exit
- -o Output filename for the point cloud. (optional). The points cloud will only exported if a filename is set.
- pictures

Some command can like this:

`
./3DTest -c 39000 -m 40 -i 10 -o out.ply pic001.tif pic002.tif
`

`
./3DTest -0 -o out.ply img/*.tif
`

or without exporting:

`
./3DTest img/*.tif
`
