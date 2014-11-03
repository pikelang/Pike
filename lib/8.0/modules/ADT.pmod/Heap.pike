#pike 8.1

inherit ADT.Heap;

//! Removes and returns the item on top of the heap,
//! which also is the smallest value in the heap.
//! @deprecated pop
__deprecated__ mixed top() { return pop(); }

//! Returns the number of elements in the heap.
//! @deprecated lfun::_sizeof
__deprecated__ int size() { return _sizeof(); }
