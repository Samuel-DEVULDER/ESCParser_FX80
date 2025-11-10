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


//////////////////////////////////////////////////////////////////////


EscInterpreter::EscInterpreter(std::istream& input, OutputDriver& output) :
    m_input(input), m_output(output)
{
    m_marginleft = 96;  // 96/720 inch = 9.6 points
    m_margintop = 160;  // 160/720 inch = 16 points
    m_endofpage = false;
    m_fontsp = m_fontdo = m_fontfe = m_fontks = m_fontel = m_fontun = false;
    m_superscript = m_subscript = false;
    m_charset = 0;

    PrinterReset();
}

unsigned char EscInterpreter::GetNextByte()
{
    if (m_input.eof())
        return 0;
    unsigned char result = (unsigned char) m_input.get();
    return result;
}

void EscInterpreter::PrinterReset()
{
    m_x = m_y = 0;
    m_printmode = false;

    //TODO: Configure the modes using DIP switches.
    m_fontsp = m_fontdo = m_fontfe = m_fontks = m_fontel = m_fontun = false;
    m_shifty = 720 / 6;  // 6 lines/inch
    UpdateShiftX();
    m_limitright = m_shiftx * 80;  //TODO
    m_limitbottom = 720 * 11;  // 11 inches = 66 lines

    m_superscript = m_subscript = false;
    m_charset = 0;
}

// Update the value of m_shiftx according to the selected font
void EscInterpreter::UpdateShiftX()
{
    m_shiftx = 720 / 10;  // Normal spacing
    if (m_fontel)
        m_shiftx = 720 / 12;  // Elite
    else if (m_fontks)
        m_shiftx = 720 / 17;  // Condensed

    if (m_fontsp)  // Spaced font
        m_shiftx *= 2;
}

void EscInterpreter::ShiftY(int shifty)
{
    m_y += shifty;

    if (m_y >= m_limitbottom)  // Proceed to the next page if needed
        NextPage();
}

void EscInterpreter::NextPage()
{
    m_endofpage = true;
    m_x = m_y = 0;
}

// Interpret the next token
bool EscInterpreter::InterpretNext()
{
    if (IsEndOfFile()) return false;
    m_endofpage = false;

    unsigned char ch = GetNextByte();
    if (IsEndOfFile()) return false;

    switch (ch)
    {
    case 0/*NUL*/: case 7/*BEL*/: case 17/*DC1*/: case 19/*DC3*/: case 127/*DEL*/:
        break; // Ignored codes
    case 24/*CAN*/:
        NextPage();
        return false; //End of page
    case 8/*BS*/: // Backspace - move back 1 character
        m_x -= m_shiftx;  if (m_x < 0) m_x = 0;
        break;
    case 9/*HT*/: // Horizontal tab - a special case is implemented
        //NOTE: resetting tab positions is ignored
        m_x += m_shiftx * 8;
        m_x = (m_x / (m_shiftx * 8)) * (m_shiftx * 8);
        break;
    case 10/*LF*/: // Line Feed - move to the next line
        ShiftY(m_shifty);
        return !m_endofpage;
    case 11/*VT*/: //Vertical Tab - in this specific case, it satisfies the description.
	    // NOTE: Resetting tab positions is ignored
        m_x = 0;  ShiftY(m_shifty);
        return !m_endofpage;
    case 12/*FF*/: // Form Feed - !!! to be completed
        NextPage();
        return false;
    case 13/*CR*/: // Carriage Return - carriage return
        m_x = 0;
        break;
    case 14/*SO*/: // Enable expanded font
        m_fontsp = true;
        UpdateShiftX();
        break;
    case 15/*SI*/: // Enable compressed font (17.1 characters per inch)
        m_fontks = true;
        UpdateShiftX();
        break;
    case 18/*DC2*/: // Disable compressed font
        m_fontks = false;
        UpdateShiftX();
        break;
    case 20/*DC4*/: // Disable expanded font
        m_fontsp = false;
        UpdateShiftX();
        break;
    case 27/*ESC*/:  // Expanded Function Codes
        return InterpretEscape();

        /* otherwise "print" the character */
    default:
        PrintCharacter(ch);
        m_x += m_shiftx;
        break;
    }

    if (m_x >= m_limitright)  // If the line length is exceeded, automatically move to the next line
    {
        m_x = 0;
        ShiftY(m_shifty);  // Proceed to the next line; probably also to the next page
    }

    return !m_endofpage;
}

// Interpret Escape sequence
bool EscInterpreter::InterpretEscape()
{
    unsigned char ch = GetNextByte();
    switch (ch)
    {
    case 'U': // Printing in one or two directions
        GetNextByte();  // Ignore
        break;
    case 'x': // Select quality
        {
            unsigned char ss = GetNextByte();
            m_printmode = (ss != 0 && ss != '0');
        }
        break;

        // Character pitch function group
    case 'P':  // Enable "pica" font
        m_fontel = false;
        UpdateShiftX();
        break;
    case 'M':  // Enable "elite" font (12 characters per inch)
        m_fontel = true;
        UpdateShiftX();
        break;
    case 15/*SI*/:  // Enable compressed font
        m_fontks = true;
        UpdateShiftX();
        break;

    case '0':  // Set interval to 1/8"
        m_shifty = 720 / 8;
        break;
    case '1':  // Set interval to 7/72"
        m_shifty = 720 * 7 / 72;
        break;
    case '2':
        m_shifty = 720 / 6; /* set line spacing to 1/6 inch */
        break;
    case 'A':   /* text line spacing */
        m_shifty = (720 * (int)GetNextByte() / 60);
        break;
    case '3':   /* graphics line spacing */
        m_shifty = (720 * (int)GetNextByte() / 180);
        break;
    case 'J': /* variable line spacing */
        ShiftY((int)GetNextByte() * 720 / 180);
        return !m_endofpage;

    case 'C': // PageLength - ignore
        if (GetNextByte() == 0)
            GetNextByte();
        break;
    case 'N': // Skip perforation - ignore
        GetNextByte();
        break;
    case 'O': break;
    case 'B': // Set vertical tabs - ignore ???
        while (GetNextByte() != 0);
        break;
    case '/':
        GetNextByte();
        break;
    case 'D': // Set horizontal tabs - ignore ???
        while (GetNextByte() != 0);
        break;
    case 'Q': // Set right margin - ignore ???
        {
            int n = (int)GetNextByte();
            if (n > 0 && m_shiftx * n <= 720 * 8)  // Not less than one character and not more than the usable width of the format (8 inches)
                m_limitright = m_shiftx * n;
            break;
        }

    case 'K': /* 8-bit single density graphics */
        printGR9(12);  // 72 / 1.2 = 60
        break;
    case 'L': /* 8-bit double density graphics */
        printGR9(6);  // 72 / 0.6 = 120
        break;
    case 'Y': /* 8-bit double-speed double-density graphics */
        printGR9(6, true);  // 72 / 0.6 = 120
        break;
    case 'Z': /* 8-bit quadple-density graphics */
        printGR9(3, true);  // 72 / 0.3 = 240
        break;
    case '*': /* Bit Image Graphics Mode */
        switch (GetNextByte())
        {
        case 0: /* same as ESC K, Normal 60 dpi */
            printGR9(12);  // 72 / 1.2 = 60
            break;
        case 1: /* same as ESC L, Double 120 dpi */
            printGR9(6);  // 72 / 0.6 = 120
            break;
        case 2: /* same as ESC Y, Double speed 120 dpi */
            printGR9(6, true);  // 72 / 0.6 = 120
            break;
        case 3: /* same as ESC Z, Quadruple 240 dpi */
            printGR9(3, true);  // 72 / 0.3 = 240
            break;
        case 4: /* CRT 1, Semi-double 80 dpi */
            printGR9(9);  // 72 / 0.9 = 80
            break;
        case 5: /* Plotter 72 dpi */
            printGR9(10);  // 72 / 1.0 = 72
            break;
        case 6: /* CRT 2, 90 dpi */
            printGR9(8);  // 72 / 0.8 = 90
            break;
        case 7: /* Double Plotter 144 pdi */
            printGR9(5);  // 72 / 0.5 = 144
            break;
        case 32:  /* High-resolution for ESC K */
            printGR24(2 * 6);
            break;
        case 33:  /* High-resolution for ESC L */
            printGR24(6);
            break;
        case 38:  /* CRT 3 */
            printGR24(2 * 4);
            break;
        case 39:  /* High-resolution triple-density */
            printGR24(2 * 2);
            break;
        case 40:  /* high-resolution hex-density */
            printGR24(2);
            break;
        }
        break;
        /* reassign bit image command ??? */
    case '?': break;
        /* download - ignore (???) */
    case '&': break; /* this command downloads character sets to the printer */
    case '%': break; /* select/deselect download character code */
    case ':': /* this command copies the internal character set into the download area */
        GetNextByte();  GetNextByte();  GetNextByte();
        break;
    case 'R': /* international character set - ignore (???) */
        m_charset = GetNextByte();
        break;
        /* MSB control - ignore (???) */
    case '#': break; /* clear most significant bit */
    case '=': break; /* clear most significant bit */
    case '>': break; /* set most significant bit */
        /* print table control */
    case '6': break; /* select upper character set */
    case '7': break; /* select lower character set */
        /* home head */
    case '<':
        m_x = 0;    /* repositions the print head to the left most column */
        break;
    case 14/*SO*/: // Enable expanded font
        m_fontsp = true;
        UpdateShiftX();
        break;
        /* inter character space */
    case 32/*SP*/:
        GetNextByte();
        break;
        /* absolute dot position */
    case '$':
        m_x = GetNextByte();
        m_x += 256 * (int)GetNextByte();
        m_x = (int)((int)m_x * 720 / 60);
        break;
        /* relative dot position */
    case '\\':
        {
            int shift = GetNextByte();  shift += 256 * (int)GetNextByte();
            m_x += (int)((int)shift * 720 / (m_printmode ? 180 : 120));
            /* !!! Take into account the LQ or DRAFT mode */
        }
        break;

        /* CHARACTER CONTROL CODES */
    case 'E': // Enable bold font
        m_fontfe = true;
        UpdateShiftX();
        break;
    case 'F': // Disable bold font
        m_fontfe = false;
        UpdateShiftX();
        break;
    case 'G':  // Enable double printing
        m_fontdo = true;
        break;
    case 'H':  // Disable double printing
        m_fontdo = false;
        m_superscript = m_subscript = false;
        break;
    case '-': // Underline
        {
            unsigned char ss = GetNextByte();
            m_fontun = (ss != 0 && ss != '0');
        }
        break;

    case 'S': // Enable printing in the upper or lower part of the line
        {
            unsigned char ss = GetNextByte();
            m_superscript = (ss == 0 || ss == '0');
            m_subscript = (ss == 1 || ss == '1');
        }
        break;
    case 'T': // Disable printing in the upper or lower part of the line
        m_superscript = m_subscript = false;
        break;
    case 'W': // Enable or disable expanded font
        {
            unsigned char ss = GetNextByte();
            m_fontsp = (ss != 0 && ss != '0');
            UpdateShiftX();
        }
        break;
    case '!': // Font type selection
        {
            unsigned char fontbits = GetNextByte();
            m_fontel = (fontbits & 1) != 0;
            m_fontks = ((fontbits & 4) != 0) && !m_fontel;
            m_fontfe = ((fontbits & 8) != 0) && !m_fontel;
            m_fontdo = (fontbits & 16) != 0;
            m_fontsp = (fontbits & 32) != 0;
            UpdateShiftX();
        }
        break;
        /* italic print */
    case '4': /* set italics */
        break;
    case '5': /* clear itelics */
        break;
        /* character table */
    case 't': /* select character table ??? */
        GetNextByte(); /* ignore */
        break;
        /* double height */
    case 'w': /* select double height !!! */
        GetNextByte();
        break;

        /* SYSTEM CONTROL CODES */
        /* reset */
    case '@':
        PrinterReset();
        break;
        /* cut sheet feeder control */
    case 25/*EM*/:
        GetNextByte(); /* ??? - ignore */
        break;
    }

    return !m_endofpage;
}

void EscInterpreter::printGR9(int dx, bool dblspeed)
{
    int width = GetNextByte();  // Number of data "chunks" for the image
    width += 256 * (int)GetNextByte();

    // Read and output data
    unsigned char lastfbyte = 0;
    for (; width > 0; width--)
    {
        unsigned char fbyte = GetNextByte();
        if (dblspeed)  // In high-speed mode, ignore consecutive strikes
        {
            fbyte &= ~lastfbyte;
            lastfbyte = fbyte;
        }
        unsigned char mask = 0x80;
        for (int i = 0; i < 8; i++)
        {
            if (fbyte & mask)
            {
                DrawStrike(float(m_x), float(m_y + i * 12));
                /* 12 corresponds to 1/60 inch... In reality, the distance between needles in
                9-pin dot matrix printers = 1/72 inch, but when emulating on a 24-pin printer, 1/60 is used */
            }
            mask >>= 1;
        }
        m_x += dx;
    }
}

void EscInterpreter::printGR24(int dx)
{
    int width = GetNextByte(); // Number of data "chunks" for the image
    width += 256 * (int)GetNextByte();

    // Read and output data
    for (; width > 0; width--)
    {
        for (unsigned char n = 0; n < 3; n++)
        {
            unsigned char fbyte = GetNextByte();
            unsigned char mask = 0x80;
            for (int i = 0; i < 8; i++)
            {
                if (fbyte & mask)
                {
                    DrawStrike(float(m_x), float((m_y + (n * 4 * 8/*èãë*/) + i * 4)));
                    /* 4 corresponds to 1/180 inch - the distance between needles in 24-pin dot matrix printers */
                }
                mask >>= 1;
            }
        }
        m_x += dx;
    }
}

void EscInterpreter::PrintCharacter(unsigned char ch)
{
    if (ch < 32) return;
    if (ch < 160 && ch > 126) return;

    // Calculate the character generator symbol based on the current character set
    int charset = m_charset ^ (ch > 128 ? 1 : 0);
    ch &= 0x7f;
    int symbol = ch;
    if (ch >= (unsigned char)'@' && charset != 0)
        symbol += 68;

    // Get the address of the character in the character generator
    const unsigned short* pchardata = RobotronFont + int(symbol - 32) * 9;

    float step = float(m_shiftx) / 11.0f;  // Horizontal step
    float y = float(m_y);
    if (m_subscript) y += 4 * 12;

    // Loop for printing the character line by line
    unsigned short prevdata = 0;
    for (int line = 0; line < 9; line++)
    {
        unsigned short data = pchardata[line];

        // Special handling for superscript and subscript characters
        if ((m_superscript || m_subscript))
        {
            if ((line & 1) == 0)
            {
                prevdata = data;
                continue;
            }
            else
            {
                data |= prevdata;  // Combine two lines of the character into one
            }
        }

        for (int col = 0; col < 9; col++)  // Loop for printing the dots of the line
        {
            unsigned short bit = (data >> col) & 1;
            if (m_fontun && line == 8) bit = 1;
            if (!bit) continue;

            DrawStrike(m_x + col * step, y);
            if (m_fontsp)
                DrawStrike(m_x + (col + 1.0f) * step, y);

            //TODO: Take into account m_fontfe (bold font)
        }

        y += 12;  // 12 corresponds to 1/60 inch
    }

    // For m_fontun, add the last point
    if (m_fontun)
        DrawStrike(m_x + 9.0f * step, float(m_y + 8 * 12));
}

void EscInterpreter::DrawStrike(float x, float y)
{
    float cx = float(m_marginleft) + x;
    float cy = float(m_margintop) + y;
    //TODO: Take into account m_fontdo in the point radius
    float cr = 6.0f;

    m_output.WriteStrike(cx, cy, cr);
}


//////////////////////////////////////////////////////////////////////
