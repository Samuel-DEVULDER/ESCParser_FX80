#ifndef _FX80_FONT_H_
#define _FX80_FONT_H_

// https://minuszerodegrees.net/manuals/Epson%20FX-80%20Operation%20Manual.pdf

struct glyph {
	unsigned short ansi; // pdf/latin1 
	unsigned short data[9];
};

extern struct glyph *FontGlyph(unsigned int charset, unsigned char ch);

#endif // _FX80_FONT_H_