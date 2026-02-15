# Performance monitor overlay and font system improvements

Added a modular performance monitor bar at the top of the viewport showing FPS, ping, and frame time with color-coded status. Hovering over the frame time expands a panel with a line graph of frame time history and per-stage profiling breakdown. Merged FrameTiming into the performance monitor system.

Fixed a bug in the font system where per-call font size overrides via TextStyle permanently mutated the global renderer font size, breaking all subsequent text rendering. Font size is now passed per-call through the renderer interface with no global state mutation. SFML's internal glyph caching handles lazy loading per character size.

Tuned the line graph to use 100ms averaged samples for smooth but responsive updates, with a dynamic Y-axis that scales in 5ms increments starting from zero.
