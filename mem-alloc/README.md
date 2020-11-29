issue with `add_block_freeList()`
what I was trying to do here was this: if there's no list, then this block become the head and the tail of the list. if there is a list, then add this block to the tail. 
from there, check if the tag before it in the heap indicates a free block. call the block before A and this block B. if A is free (we are looking at A's end tag), then we update the tags for both blocks to have the size equal to the sum of their sizes. from there, point the B's front tag to A's front tag and point A's end tag to B's end tag. we do not have to update next/prev pointers here because we are merging B into A.
then check if the block adjacent after B is free (call it C). update the sizes of both. since we are merging C into B here, we need to update the actual node to be at the front of B's free space. so we point it there and also update C's prev/next to point there. we also point this moved node's prev/next at those blocks. finallly we point block C to block B and rearrange the tags like we did for A and B. I think I was doing too much pointing around here and got confused with using the structs instead of pure pointers. 

I would explain more here for the rest of my code but I spent a lot of time commenting things, and it might be easier to see the code alongside the comments anyway.


