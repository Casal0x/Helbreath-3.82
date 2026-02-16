# Magic casting hardening and warning cleanup

Added server-side viewport clamping to prevent off-screen spell casting and tightened auto-aim range from 10 tiles to 2 tiles to close an exploit where players could target just outside the victim's viewport.

Fixed all implicit truncation and type conversion warnings across server and client code.
