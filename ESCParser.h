/*  This file is part of UKNCBTL.
    UKNCBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    UKNCBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
UKNCBTL. If not, see <http://www.gnu.org/licenses/>. */

#ifndef _ESCPARSER_H_
#define _ESCPARSER_H_

#include <iostream>
#include <vector>

#ifndef WIN32
#include <string.h>
#define sprintf_s snprintf
#define _stricmp  strcasecmp
#endif

extern unsigned short RobotronFont[];

//////////////////////////////////////////////////////////////////////
// Txt chunk
class TxtChunk
{
protected:
    std::vector<unsigned short> m_buf; // utf-16
	int m_x = 0, m_y = 0, m_w = 0, m_h = 0;
	
public:
    TxtChunk() : m_buf() { }
    ~TxtChunk() { }

	size_t size() {return m_buf.size();}
		
	bool canSet(int x, int y, int w, int h);
	void set(unsigned short ch, int x, int y, int w, int h);
	TxtChunk &appendAscii(std::string &s); // 7 bits
	TxtChunk &appendWinAnsi(std::string &s); // 8 bit
	// TODO : unicode 16
	
	unsigned short get(size_t pos) {
		return pos<m_buf.size() ? m_buf[pos] : 32;
	}
	
	int getX() {return m_x;}
	int getY() {return m_y;}
	int getW() {return m_w;}
	int getH() {return m_h;}
	
	TxtChunk &clear() {
		m_x = m_y = m_w = m_h = 0;
		m_buf.clear();
		return *this;
	}
	
	TxtChunk &trim() {
		size_t i = size();
		while(i && m_buf[i-1]==32) --i;
		m_buf.resize(i);
		return *this;
	}
	
};


//////////////////////////////////////////////////////////////////////
// Output drivers

enum
{
    OUTPUT_DRIVER_UNKNOWN = 0,
    OUTPUT_DRIVER_SVG = 1,
    OUTPUT_DRIVER_POSTSCRIPT = 2,
    OUTPUT_DRIVER_PDF = 3,
	OUTPUT_DRIVER_TXT = 4
};

// Base abstract class for output drivers
class OutputDriver
{
protected:
    std::ostream& m_output;

public:
    OutputDriver(std::ostream& output) : m_output(output) { }
    virtual ~OutputDriver() { }

public:
    // Write beginning of the document
    virtual void WriteBeginning(int pagestotal) { }  // Overwrite if needed
    // Write ending of the document
    virtual void WriteEnding() { }  // Overwrite if needed
    // Write beginning of the page
    virtual void WritePageBeginning(int pageno) { }  // Overwrite if needed
    // Write ending of the page
    virtual void WritePageEnding() { }  // Overwrite if needed
    // Write strike by one pin
    virtual void WriteStrike(float x, float y, float r) = 0;  // Always overwrite
	// Write a character
	virtual void WriteChar(unsigned short ch, int x, int y, int w, int h) { }
};

// Stub driver, does nothing
class OutputDriverStub : public OutputDriver
{
public:
    OutputDriverStub(std::ostream& output) : OutputDriver(output) { };

public:
    virtual void WriteStrike(float x, float y, float r) { }
};

// Dumb driver, just print text
class OutputDriverTxt : public OutputDriverStub
{
protected:
    TxtChunk m_txt;
public:
    OutputDriverTxt(std::ostream& output) : OutputDriverStub(output), m_txt() { };
	virtual void WriteEnding();
	virtual void WriteChar(unsigned short ch, int x, int y, int w, int h);
};


// SVG driver, for one-page output only
class OutputDriverSvg : public OutputDriver
{
public:
    OutputDriverSvg(std::ostream& output) : OutputDriver(output) { };

public:
    virtual void WriteBeginning(int pagestotal);
    virtual void WriteEnding();
    virtual void WriteStrike(float x, float y, float r);
};

// PostScript driver with multipage support
class OutputDriverPostScript : public OutputDriver
{
public:
    OutputDriverPostScript(std::ostream& output) : OutputDriver(output) { };

public:
    virtual void WriteBeginning(int pagestotal);
    virtual void WriteEnding();
    virtual void WritePageBeginning(int pageno);
    virtual void WritePageEnding();
    virtual void WriteStrike(float x, float y, float r);
};


struct PdfXrefItem
{
    std::streamoff offset;
    int size;
    char flag;
public:
    PdfXrefItem(std::streamoff anoffset, int asize, char aflag)
    {
        offset = anoffset;
        size = asize;
        flag = aflag;
    }
};
// PDF driver with multipage support
class OutputDriverPdf : public OutputDriver
{
public:
    OutputDriverPdf(std::ostream& output) : OutputDriver(output) { strikesize = 0.1f; };

public:
    virtual void WriteBeginning(int pagestotal);
    virtual void WriteEnding();
    virtual void WritePageBeginning(int pageno);
    virtual void WritePageEnding();
    virtual void WriteStrike(float x, float y, float r);
	virtual void WriteChar(unsigned short ch, int x, int y, int w, int h);

private:
    std::vector<PdfXrefItem> xref;
    std::string pagebuf;
    float strikesize;

protected:	
	std::string m_txtbuf;
    TxtChunk m_txt;
};


//////////////////////////////////////////////////////////////////////
// ESC/P interpreter

class EscInterpreter
{
private:  // Input and output
    std::istream& m_input;
    OutputDriver& m_output;

private:  // Current state
    // Units for all the int values are equal to 1/10 point = 1/720 inch
    int  m_x, m_y;      // Current position
    int  m_marginleft, m_margintop;
    int  m_limitright;
    int  m_limitbottom;
    int  m_shiftx, m_shifty;  // Shift for text printout
    bool m_printmode;   // false - DRAFT, true - LQ
    bool m_endofpage;
    bool m_fontsp;      // Spaced fond
    bool m_fontdo;      // Double printing
    bool m_fontfe;      // Bold font
    bool m_fontks;      // Compressed font
    bool m_fontel;      // "Elite" font
    bool m_fontun;      // Underline
    bool m_superscript; // Super-script
    bool m_subscript;   // Sub-script
    unsigned char m_charset;  // Character set number

public:
    // Constructor
    EscInterpreter(std::istream& input, OutputDriver& output);
    // Interpret next character or escape sequense
    bool InterpretNext();
    // Interpret escape sequence
    bool InterpretEscape();
    // is the end of input stream reached
    bool IsEndOfFile() const { return m_input.eof(); }

protected:
    // Retrieve a next byte from the input
    unsigned char GetNextByte();
    // Update m_shiftx according to current font settings
    void UpdateShiftX();
    // Increment m_y by shifty; proceed to the next page if needed
    void ShiftY(int shifty);
    // End the current page
    void NextPage();
    // Reset the printer settings
    void PrinterReset();
    // Print graphics
    void printGR9(int dx, bool dblspeed = false);
    // Print graphics
    void printGR24(int dx);
    // Print the symbol using current charset
    void PrintCharacter(unsigned char ch);
    // Draw strike made by one pin
    void DrawStrike(float x, float y);
};


//////////////////////////////////////////////////////////////////////
#endif // _ESCPARSER_H_
