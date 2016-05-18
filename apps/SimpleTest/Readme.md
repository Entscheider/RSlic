#SimpleTest

Little application for showing pictures with Superpixel.

Parameters:

- -c Number of Superpixel (optional)
- -m Stiffness (optional)
- -i Number of iterations (optional)
- -0 Use Slico. Ignores -i (optional)
- -t Number of threads to be used.
- -h Show help
- Filename of the picture

For the window following keys are accepted:

- q Close the Window
- -b Set contour black
- -w Set contour white

For example:

- `./SimpleTest -c 400 -m 40 -i 10 pic.png` 
- `./SimpleTest -0 pic.png`
- `./SimpleTest pic.png`
