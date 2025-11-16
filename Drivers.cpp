/*  This file is part of UKNCBTL.
    UKNCBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    UKNCBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
UKNCBTL. If not, see <http://www.gnu.org/licenses/>. */

#include "ESCParser.h"
#include <iomanip>

#include "zlib/zlib.h"

//////////////////////////////////////////////////////////////////////
// TxtChunk
static void printOver(unsigned short& c1, unsigned short c2) {
	char c;
	if(c1==32) {
	} else if(c1==(c='`') || (c2==c && (c2=c1))) {
		switch(c2) {
			case 'A': c2 = 192; break;
			case 'E': c2 = 200; break;
			case 'I': c2 = 204; break;
			case 'O': c2 = 210; break;
			case 'U': c2 = 217; break;
			case 'a': c2 = 224; break;
			case 'e': c2 = 232; break;
			case 'i': c2 = 236; break;
			case 'o': c2 = 242; break;
			case 'u': c2 = 249; break;
		}
	} else if(c1==(c='\'') || (c2==c && (c2=c1))) {
		switch(c2) {
			case 'A': c2 = 193; break;
			case 'E': c2 = 201; break;
			case 'I': c2 = 205; break;
			case 'O': c2 = 211; break;
			case 'U': c2 = 218; break;
			case 'Y': c2 = 221; break;
			case 'a': c2 = 225; break;
			case 'e': c2 = 233; break;
			case 'i': c2 = 237; break;
			case 'o': c2 = 243; break;
			case 'u': c2 = 250; break;
			case 'y': c2 = 253; break;
		}
	} else if(c1==(c='^') || (c2==c && (c2=c1))) {
		switch(c2) {
			case 'A': c2 = 194; break;
			case 'E': c2 = 202; break;
			case 'I': c2 = 206; break;
			case 'O': c2 = 212; break;
			case 'U': c2 = 219; break;
			case 'a': c2 = 226; break;
			case 'e': c2 = 234; break;
			case 'i': c2 = 238; break;
			case 'o': c2 = 244; break;
			case 'u': c2 = 251; break;
		}
	} else if(c1==(c='~') || (c2==c && (c2=c1))) {
		switch(c2) {
			case 'A': c2 = 195; break;
			case 'N': c2 = 209; break;
			case 'O': c2 = 213; break;
			case 'a': c2 = 227; break;
			case 'n': c2 = 241; break;
			case 'o': c2 = 245; break;
		}
	} else if(c1==(c='"') || (c2==c && (c2=c1))) {
		switch(c2) {
			case 'A': c2 = 196; break;
			case 'E': c2 = 203; break;
			case 'I': c2 = 207; break;
			case 'O': c2 = 214; break;
			case 'U': c2 = 220; break;
			case 'a': c2 = 228; break;
			case 'e': c2 = 235; break;
			case 'i': c2 = 239; break;
			case 'o': c2 = 246; break;
			case 'u': c2 = 252; break;
			case 'y': c2 = 255; break;
		}
	} else if(c1==(c=',') || (c2==c && (c2=c1))) {
		switch(c2) {
			case 'C': c2 = 199; break;
			case 'c': c2 = 231; break;
		}
	} else if(32<c1 && c1<128 && c2>=128) c2 = c1;
	c1 = c2;
}

#define UNK "_"

static std::string ascii[256] = {
	"[NUL]","[SOH]","[STX]","[ETX]","[EOT]","[ENQ]","[ACK]","[BEL]",
	"[BS]","[HT]","[LF]","[VT]","[FF]","[CR]","[SO]","[SI]",
	"[DLE]","[DC1]","[DC2]","[DC3]","[DC4]","[NAK]","[SYN]","[ETB]",
	"[CAN]","[EM]","[SUB]","[ESC]","[FS]","[GS]","[RS]","[US]",
	" ","!","\"","#","$","%","&","'","(",")","*","+",",","-",".",",",
	"0","1","2","3","4","5","6","7","8","9",":",";","<","=",">","?",
	"@","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O",
	"P","Q","R","S","T","U","V","W","X","Y","Z","[","\\","]","^","_",
	"`","a","b","c","d","e","f","g","h","i","j","k","l","m","n","o",
	"p","q","r","s","t","u","v","w","x","y","z","{","|","}","~","[DEL]",
	"EUR",UNK,",","f",",,","...","+","++","^","o/oo","S","<","OE",UNK,"Z",UNK,
	"?","`","'","\"","\"","*","-","--","~","TM","s",">","oe",UNK,"z","Y",
	UNK,"!","c","L","o","Y","|","S","\"","(C)","a","<<","~","-","(R)","-",
	"o","+/-","2","3","'","u","P",".",",","1","o",">>","1/4","1/2","3/4","?",
	"A","A","A","A","A","A","AE","C","E","E","E","E","I","I","I","I",
	"D","N","O","O","O","O","O","x","O","U","U","U","U","Y","Th","ss",
	"a","a","a","a","a","a","ae","c","e","e","e","e","i","i","i","i",
	"d","n","o","o","o","o","o","/","o","u","u","u","u","y","th","y"
};

TxtChunk &TxtChunk::appendAscii(std::string &out) {
	for(int i=0, m=size(); i<m; ++i) {
		unsigned short c = m_buf[i];
		out.append(c<256 ? ascii[c] : "_");
	}
	return *this;
}

TxtChunk &TxtChunk::appendWinAnsi(std::string &out) {
	char buf[5];
	for(int i=0, m=size(); i<m; ++i) {
		unsigned short c = m_buf[i];
		sprintf(buf, 31<c && c<128 ? c=='(' || c=='\\' || c==')' ? 
					"\\%c" : "%c" : "\\%03o", c&255);
		out.append(buf);
	}
	return *this;
}

bool TxtChunk::canSet(int x, int y, int w, int h) {
	if(m_w == 0 || m_h == 0) return true;
	if(m_w != w || m_h != h) return false;
	if(m_y != y)             return false;
	if(m_x >  x)             return false;
	size_t pos = (x - m_x)/m_w;
	return pos <= m_buf.size();
}

void TxtChunk::set(unsigned short ch, int x, int y, int w, int h) {
	if(m_w==0 || m_h==0) {
		m_x = x; m_y = y;
		m_w = w; m_h = h;
		m_buf.clear();
	}
	size_t pos = (x - m_x)/m_w;
	while(pos >= m_buf.size()) m_buf.push_back(32);
	printOver(m_buf[pos], ch);
}

//////////////////////////////////////////////////////////////////////
// txt driver

void OutputDriverTxt::WriteEnding()
{
	std::string str;
	m_txt.trim().appendAscii(str).clear();
	m_output << str;
}

void OutputDriverTxt::WriteChar(unsigned short ch, int x, int y, int w, int h) 
{
	if(!m_txt.canSet(x,y,w,h)) {
		bool eol = y != m_txt.getY();
		WriteEnding();
		if(eol) m_output << std::endl;
	}
	m_txt.set(ch,x,y,w,h);
}

//////////////////////////////////////////////////////////////////////
// SVG driver

//NOTE: The most recent SVG standard is 1.2 tiny. Multipage support appears in 1.2 full.
// So, currently SVG does not have multipage support, and browsers can't interpret multipage SVGs.

void OutputDriverSvg::WriteBeginning(int pagestotal)
{
    m_output << "<?xml version=\"1.0\"?>" << std::endl;
    m_output << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.0\">" << std::endl;
}

void OutputDriverSvg::WriteEnding()
{
    m_output << "</svg>" << std::endl;
}

void OutputDriverSvg::WriteStrike(float x, float y, float r)
{
    float cx = x / 10.0f;
    float cy = y / 10.0f;
    float cr = r / 10.0f;
    m_output << "<circle cx=\"" << cx << "\" cy=\"" << cy << "\" r=\"" << cr << "\" />" << std::endl;
}


//////////////////////////////////////////////////////////////////////
// PostScript driver

void OutputDriverPostScript::WriteBeginning(int pagestotal)
{
    m_output << "%!PS-Adobe-2.0" << std::endl;
    m_output << "%%Creator: ESCParser" << std::endl;
    m_output << "%%Pages: " << pagestotal << std::endl;

    // PS procedure used to simplify WriteStrike output
    m_output << "/dotxyr { newpath 0 360 arc fill } def" << std::endl;
}

void OutputDriverPostScript::WriteEnding()
{
    m_output << "%%EOF" << std::endl;
}

void OutputDriverPostScript::WritePageBeginning(int pageno)
{
    m_output << "%%Page: " << pageno << " " << pageno << std::endl;
    m_output << "0 850 translate 1 -1 scale" << std::endl;
    m_output << "0 setgray" << std::endl;
}

void OutputDriverPostScript::WritePageEnding()
{
    m_output << "showpage" << std::endl;
}

void OutputDriverPostScript::WriteStrike(float x, float y, float r)
{
    float cx = x / 10.0f;
    float cy = y / 10.0f;
    float cr = r / 10.0f;

    char buffer[24];
    sprintf_s(buffer, sizeof(buffer), "%.2f %.2f %.1f", cx, cy, cr);
    m_output << buffer << " dotxyr" << std::endl;
}

//////////////////////////////////////////////////////////////////////
// PDF driver

// See below
void ascii85_encode_tuple(const unsigned char* src, char* dst);

const float PdfPageSizeX = 595.0f;  // A4 210mm / 25.4 * 72, rounded
const float PdfPageSizeY = 842.0f;  // A4 297mm / 25.4 * 72, rounded

void OutputDriverPdf::WriteBeginning(int pagestotal)
{
    xref.push_back(PdfXrefItem(0, 65535, 'f'));
    m_output << "%PDF-1.3" << std::endl;

    xref.push_back(PdfXrefItem(m_output.tellp(), 0, 'n'));
    m_output << "1 0 obj <<";
    m_output << "/Producer (ESCParser utility by Nikita Zimin)";
    m_output << ">>" << std::endl << "endobj" << std::endl;

    xref.push_back(PdfXrefItem(m_output.tellp(), 0, 'n'));
    m_output << "2 0 obj <</Type /Catalog /Pages 3 0 R>>" << std::endl;
    m_output << "endobj" << std::endl;

    xref.push_back(PdfXrefItem(m_output.tellp(), 0, 'n'));
    m_output << "3 0 obj <</Type /Pages /Kids [";
    for (int i = 0; i < pagestotal; i++)
    {
        if (i > 0)
            m_output << " ";
        m_output << i * 3 + 4 << " 0 R";  // Page objects: 4, 7, 10, etc.
    }
    m_output << "] /Count " << pagestotal << ">>" << std::endl;
    m_output << "endobj" << std::endl;
}

void OutputDriverPdf::WriteEnding()
{
    std::streamoff startxref = m_output.tellp();
    m_output << "xref" << std::endl;
    m_output << "0 " << xref.size() << std::endl;
    for (std::vector<PdfXrefItem>::iterator it = xref.begin(); it != xref.end(); ++it)
    {
        m_output << std::setw(10) << std::setfill('0') << (*it).offset << " ";
        m_output << std::setw(5) << (*it).size << " ";
        m_output << (*it).flag << std::endl;
    }

    m_output << "trailer" << std::endl;
    m_output << "<</Size " << xref.size() << " /Root 2 0 R /Info 1 0 R>>" << std::endl;
    m_output << "startxref" << std::endl;
    m_output << startxref << std::endl;
    m_output << "%%EOF" << std::endl;
}

void OutputDriverPdf::WritePageBeginning(int pageno)
{
    xref.push_back(PdfXrefItem(m_output.tellp(), 0, 'n'));
    int objnopage   = pageno * 3 + 1;  // 4, 7, 10, etc.
    int objnofont   = pageno * 3 + 2;  // 5, 8, 11, etc.
    int objnostream = pageno * 3 + 3;  // 6, 9, 12, etc.
    m_output << objnopage << " 0 obj<</Type /Page /Parent 3 0 R ";
    m_output << "/MediaBox [0 0 " << PdfPageSizeX << " " << PdfPageSizeY << "] ";  // Page bounds
    m_output << "/Contents " << objnostream << " 0 R ";
    m_output << "/Resources << /Font << /F1 "<<objnofont<<" 0 R >> >>" << std::endl;  // Resources is required key
    m_output << ">> endobj" << std::endl;

	xref.push_back(PdfXrefItem(m_output.tellp(), 0, 'n'));
    m_output << objnofont << " 0 obj << /Type /Font /Subtype /Type1 /BaseFont /Courier >>" << std::endl;
    m_output << "endobj" << std::endl;

    xref.push_back(PdfXrefItem(m_output.tellp(), 0, 'n'));
    m_output << objnostream << " 0 obj<<";

    strikesize = 0.0f;
    pagebuf.clear();
    pagebuf.append("1 J");  // Round cap
	m_txtbuf.clear();
	m_txt.clear();
}

static void addPdfBT(std::string &str, TxtChunk &txt) {
	if(txt.trim().size()) {
		char buf[64];
		float cx = txt.getX() / 10.0f;
		float cy = PdfPageSizeY - txt.getY() / 10.0f;
		float cw = txt.getW() / 10.0f;
		float ch = txt.getH() / 10.0f;

		// sprintf(buf, " BT /F1 %g Tf %g 100.0 Tz %g %g Tm 0 Tr (",
			// 12.0f, 60.0f, cx-1, cy-7.5);
		
		sprintf(buf, " %g 0 0 %g %g %g Tm (",
				(cw*100)/60, ch*.559f, cx-1, cy-.559*ch + 1.5);
		
		// sprintf(buf, " BT /F1 9 Tf %g %g Td (", cx, cy);
		str.append(buf);
		txt.appendWinAnsi(str);
		str.append(") Tj");
	}
	txt.clear();
}

void OutputDriverPdf::WritePageEnding()
{
	addPdfBT(m_txtbuf, m_txt);
	pagebuf.append("\nBT /F1 1 Tf 3 Tr");
	pagebuf.append(m_txtbuf); 
	pagebuf.append(" ET");
	m_txtbuf.clear();
	
    // Preparing for inflate
    size_t outsize = pagebuf.length() + pagebuf.length() / 2 + 200;
    outsize = (outsize + 3) / 4 * 4;  // Make sure we have 4-byte aligned size
    Bytef* zbuffer = new Bytef[outsize];  memset(zbuffer, 0, outsize);
    z_stream zstrm;  memset(&zstrm, 0, sizeof(zstrm));
    zstrm.avail_in = pagebuf.length();
    zstrm.avail_out = outsize;
    zstrm.next_in = (Bytef*) pagebuf.c_str();
    zstrm.next_out = zbuffer;
    // Trying to inflate
    bool inflatedok = false;
    int rsti = deflateInit(&zstrm, Z_DEFAULT_COMPRESSION);
    if (rsti == Z_OK)
    {
        int rst2 = deflate(&zstrm, Z_FINISH);
        if (rst2 >= 0)
            inflatedok = true;
    }
    size_t inflatesize = zstrm.total_out;

    if (inflatedok)
    {
        std::string pagebufz;
        char buffer[6];  memset(buffer, 0, sizeof(buffer));
        for (size_t i = 0; i < inflatesize; i += 4)
        {
            unsigned char * bytes = zbuffer + i;
            ascii85_encode_tuple(bytes, buffer);
            pagebufz.append(buffer);
        }
        pagebufz.append("~>");

        m_output << "/Length " << pagebufz.length() << " /Filter [/ASCII85Decode /FlateDecode]>>stream" << std::endl;
        m_output << pagebufz.c_str() << std::endl;
        m_output << "endstream" << std::endl << "endobj" << std::endl;
    }
    else
    {
        m_output << "/Length " << pagebuf.length() << ">>stream" << std::endl;
        m_output << pagebuf.c_str() << std::endl;
        m_output << "endstream" << std::endl << "endobj" << std::endl;
    }

    deflateEnd(&zstrm);
    delete[] zbuffer;  zbuffer = 0;
}

void OutputDriverPdf::WriteChar(unsigned short ch, int x, int y, int w, int h) 
{
	if(!m_txt.canSet(x,y,w,h)) addPdfBT(m_txtbuf, m_txt);
	m_txt.set(ch,x,y,w,h);
}

void OutputDriverPdf::WriteStrike(float x, float y, float r)
{
    char buffer[80];

    if (strikesize != r / 5.0f)
    {
        strikesize = r / 5.0f;
        sprintf_s(buffer, sizeof(buffer), " %g w", strikesize);  // Line width
        pagebuf.append(buffer);
    }

    float cx = x / 10.0f;
    float cy = PdfPageSizeY - y / 10.0f;

    sprintf_s(buffer, sizeof(buffer), " %g %g m %g %g l s", cx, cy, cx, cy);
    pagebuf.append(buffer);
}

//////////////////////////////////////////////////////////////////////
// ASCII85 encoding for PDF

typedef unsigned int  uint32_t;
// make sure uint32_t is 32-bit
typedef char Z85_uint32_t_static_assert[(sizeof(uint32_t) * 8 == 32) * 2 - 1];

#define DIV85_MAGIC 3233857729ULL
// make sure magic constant is 64-bit
typedef char Z85_div85_magic_static_assert[(sizeof(DIV85_MAGIC) * 8 == 64) * 2 - 1];

#define DIV85(number) ((uint32_t)((DIV85_MAGIC * (number)) >> 32) >> 6)

static const char* base85 =
    "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstu";

void ascii85_encode_tuple(const unsigned char* src, char* dst)
{
    // unpack big-endian frame
    uint32_t value = (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];

    if (value == 0)  // Special case for zero
    {
        dst[0] = 'z';
        dst[2] = dst[3] = dst[4] = dst[5] = 0;
    }
    else
    {
        uint32_t value2;
        value2 = DIV85(value); dst[4] = base85[value - value2 * 85]; value = value2;
        value2 = DIV85(value); dst[3] = base85[value - value2 * 85]; value = value2;
        value2 = DIV85(value); dst[2] = base85[value - value2 * 85]; value = value2;
        value2 = DIV85(value); dst[1] = base85[value - value2 * 85];
        dst[0] = base85[value2];
    }
}

//////////////////////////////////////////////////////////////////////
