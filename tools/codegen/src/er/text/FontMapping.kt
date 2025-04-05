package er.text

import java.io.File

object FontMapping {
    enum class Font(val id: Int, val widths: FontWidths = FontWidths(id)) {
        SMALL(0),
        NORMAL(1),
        SHORT(2),
        SHORT_COPY_1(3),
        SHORT_COPY_2(4),
        SHORT_COPY_3(5),
        BRAILLE(6),
        NARROW(7),
        SMALL_NARROW(8),
        BOLD(9),
    }

    class FontWidths(val id: Int) {
        val entries by lazy {
            File("../../graphics/fonts/font${id}_latin_widths.inc").readLines()
                .flatMap { """\d+""".toRegex().findAll(it) }.map {
                    it.value.toInt()
                }
        }
    }

    private fun Char.width(font: Font, acceptNewline: Boolean = false): Int {
        if (acceptNewline && this == '\n') return 0
        val index = checkNotNull(CHAR_INDICES[this]) { "No index present for character $this" }
        return font.widths.entries[index]
    }

    fun breakString(string: String, font: Font, maxLength: Int, maxLines: Int) = buildString {
        var curLength = 0
        string.split("""((?= )|(?<=-)|(?=\n))""".toRegex()).forEach { piece ->
            val pieceLength = piece.sumOf { it.width(font, true) }
            if (piece.startsWith("\n") ||
                curLength + pieceLength > maxLength
            ) {
                append("\\n")
                curLength = pieceLength
                append(piece.trim())
            } else {
                append(piece)
                curLength += pieceLength
            }
        }
    }.also { result ->
        check(result.split("\\n").size <= maxLines) {
            "String $result exceeds max line length of $maxLines."
        }
    }

    private val CHAR_INDICES = mapOf(
        ' ' to 0x00,
        'À' to 0x01,
        'Á' to 0x02,
        'Â' to 0x03,
        'Ç' to 0x04,
        'È' to 0x05,
        'É' to 0x06,
        'Ê' to 0x07,
        'Ë' to 0x08,
        'Ì' to 0x09,
        'Î' to 0x0B,
        'Ï' to 0x0C,
        'Ò' to 0x0D,
        'Ó' to 0x0E,
        'Ô' to 0x0F,
        'Œ' to 0x10,
        'Ù' to 0x11,
        'Ú' to 0x12,
        'Û' to 0x13,
        'Ñ' to 0x14,
        'ß' to 0x15,
        'à' to 0x16,
        'á' to 0x17,
        'ç' to 0x19,
        'è' to 0x1A,
        'é' to 0x1B,
        'ê' to 0x1C,
        'ë' to 0x1D,
        'ì' to 0x1E,
        'î' to 0x20,
        'ï' to 0x21,
        'ò' to 0x22,
        'ó' to 0x23,
        'ô' to 0x24,
        'œ' to 0x25,
        'ù' to 0x26,
        'ú' to 0x27,
        'û' to 0x28,
        'ñ' to 0x29,
        'º' to 0x2A,
        'ª' to 0x2B,
        '&' to 0x2D,
        '+' to 0x2E,
        '=' to 0x35,
        ';' to 0x36,
        '¿' to 0x51,
        '¡' to 0x52,
        'Í' to 0x5A,
        '%' to 0x5B,
        '(' to 0x5C,
        ')' to 0x5D,
        'â' to 0x68,
        'í' to 0x6F,
        '<' to 0x85,
        '>' to 0x86,
        '0' to 0xA1,
        '1' to 0xA2,
        '2' to 0xA3,
        '3' to 0xA4,
        '4' to 0xA5,
        '5' to 0xA6,
        '6' to 0xA7,
        '7' to 0xA8,
        '8' to 0xA9,
        '9' to 0xAA,
        '!' to 0xAB,
        '?' to 0xAC,
        '.' to 0xAD,
        '-' to 0xAE,
        '·' to 0xAF,
        '…' to 0xB0,
        '“' to 0xB1,
        '”' to 0xB2,
        '‘' to 0xB3,
        '\'' to 0xB4,
        '♂' to 0xB5,
        '♀' to 0xB6,
        '¥' to 0xB7,
        ',' to 0xB8,
        '×' to 0xB9,
        '/' to 0xBA,
        'A' to 0xBB,
        'B' to 0xBC,
        'C' to 0xBD,
        'D' to 0xBE,
        'E' to 0xBF,
        'F' to 0xC0,
        'G' to 0xC1,
        'H' to 0xC2,
        'I' to 0xC3,
        'J' to 0xC4,
        'K' to 0xC5,
        'L' to 0xC6,
        'M' to 0xC7,
        'N' to 0xC8,
        'O' to 0xC9,
        'P' to 0xCA,
        'Q' to 0xCB,
        'R' to 0xCC,
        'S' to 0xCD,
        'T' to 0xCE,
        'U' to 0xCF,
        'V' to 0xD0,
        'W' to 0xD1,
        'X' to 0xD2,
        'Y' to 0xD3,
        'Z' to 0xD4,
        'a' to 0xD5,
        'b' to 0xD6,
        'c' to 0xD7,
        'd' to 0xD8,
        'e' to 0xD9,
        'f' to 0xDA,
        'g' to 0xDB,
        'h' to 0xDC,
        'i' to 0xDD,
        'j' to 0xDE,
        'k' to 0xDF,
        'l' to 0xE0,
        'm' to 0xE1,
        'n' to 0xE2,
        'o' to 0xE3,
        'p' to 0xE4,
        'q' to 0xE5,
        'r' to 0xE6,
        's' to 0xE7,
        't' to 0xE8,
        'u' to 0xE9,
        'v' to 0xEA,
        'w' to 0xEB,
        'x' to 0xEC,
        'y' to 0xED,
        'z' to 0xEE,
        '▶' to 0xEF,
        ':' to 0xF0,
        'Ä' to 0xF1,
        'Ö' to 0xF2,
        'Ü' to 0xF3,
        'ä' to 0xF4,
        'ö' to 0xF5,
        'ü' to 0xF6,
    )
}