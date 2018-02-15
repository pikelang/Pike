
#pike 7.6

inherit ADT.Heap;

//! This method has been renamed to @expr{pop@} in Pike 7.6.
//! @deprecated pop
mixed top() { return pop(); }
