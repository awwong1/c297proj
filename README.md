c297proj: Mouse Maze Simulation
===============================

A quick walkthrough:
--------------------

The maze is made up of points placed equi-distant across the board.
The point struct has x and y coordinates.  The board is circular, like
in pacman.  The points are stored in the point_array variable.  Points
are numbered sequentially from the top left corner to the bottom right
corner.

Walls may be placed between any two points, excepting those on the far
right column or the bottom row.  This is to preserve the circular
nature of the board.  The wall struct has two points.  
Walls are stored in the wall_array according to box number: 
      
  - The board has "boxes" in which the mouse and cheese can reside.
    The box number is the same as the top left hand corner's number.
  - For every box, there can be two walls - the top and right walls.
  - wall_array holds two elements for each box. The two elements are
    0/1 values representing the presence of top and right walls for
    each box.

The entity struct contains the previous and current position for the
mouse and cheese.  The position is represented by the point number for
the top left hand corner of the box in which the entity resides.

A mapping of the board is provided in grid_visual.pdf.