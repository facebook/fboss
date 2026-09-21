HW tests intents
=================

This document describes at a high level what behavior each group of HW tests 
test. It should be used as a reference for learning about the expected behavior 
of FBOSS system. It is not intended to be a architecture doc.

In Pause discard tests
-----------------------
- Basic expectation here is that a incoming pause frame increments both the
  inDiscard and inPause port counters regardless of whether the port has
  flow control enabled or not. 
- FBOSS then computes a derived inDiscard counter by subtracting the inPause
  from inDiscard. In the field its very confusing to see pause frames 
  contribute to in discard counts.

