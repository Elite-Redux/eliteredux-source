package er.defines

import er.Generator
import er.proto.Pocket
import java.io.OutputStreamWriter

object PocketEnumGenerator : Generator {
    override fun generate(writer: OutputStreamWriter) {
        val pockets = Pocket.entries.filter { it != Pocket.UNRECOGNIZED }
        writer.appendLine(
            """
            |#ifdef __assembly__
            |
            |${pockets.joinToString("\n") { "#define ${it.name} ${it.number}" }}
            |
            |#else
            |
            |typedef enum Pocket {
            |${pockets.joinToString("\n") { "    ${it.name} = ${it.number}," }}
            |} Pocket;
            |
            |#endif
            |""".trimMargin()
        )
    }
}