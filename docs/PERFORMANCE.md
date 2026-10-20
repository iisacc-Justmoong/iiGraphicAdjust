# Filter workspace reuse

Box blur uses two premultiplied buffers and swaps their roles between axes.
The large-sigma Gaussian path reuses that workspace through all six existing
box passes. Peak storage remains two buffers, while repeated allocation and
zero-initialization of a complete image are eliminated. Sample order, edge
repetition, premultiplication, Gaussian radii and floating-point operations are
unchanged. Small-sigma convolution retains its zero-initialized accumulators.

Color halftone computes its three channel rotations once per invocation rather
than once per pixel. No persistent image cache or global mutable workspace is
introduced, so independent calls remain independent.

The blur oracle tests cover scalar box filtering, degenerate dimensions,
transparent pixels and chained Gaussian passes; the full filter suite checks
invalid parameters and the other users of the shared blur routines.
